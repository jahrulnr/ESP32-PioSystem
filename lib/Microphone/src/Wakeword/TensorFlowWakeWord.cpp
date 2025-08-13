#include "TensorFlowWakeWord.h"
#include "hey_esp_model_data.h"
#include <cmath>
#include <cstring>
#include "tensorflow/lite/version.h"

// Static allocation for TensorFlow objects to avoid memory allocation issues
static tflite::MicroErrorReporter tf_error_reporter;
static tflite::ops::micro::AllOpsResolver tf_resolver;

TensorFlowWakeWord::TensorFlowWakeWord() 
    : error_reporter_(&tf_error_reporter)
    , model_(nullptr)
    , resolver_(&tf_resolver)
    , tensor_allocator_(nullptr)
    , interpreter_(nullptr)
    , tensor_arena_(nullptr)
    , audio_buffer_(nullptr)
    , audio_buffer_pos_(0)
    , wake_word_detected_(false)
    , confidence_(0.0f)
    , last_detection_time_(0) {
}

TensorFlowWakeWord::~TensorFlowWakeWord() {
    if (tensor_arena_) {
        #ifdef BOARD_HAS_PSRAM
        free(tensor_arena_);  // ps_malloc uses regular free()
        #else
        free(tensor_arena_);
        #endif
    }
    if (audio_buffer_) {
        free(audio_buffer_);
    }
    // Don't delete static objects
    delete tensor_allocator_;
    delete interpreter_;
}

bool TensorFlowWakeWord::initialize() {
    Serial.println("Initializing TensorFlow Lite Wake Word Detector...");
    
    // Check available heap
    size_t free_heap = ESP.getFreeHeap();
    #ifdef BOARD_HAS_PSRAM
    size_t free_psram = ESP.getFreePsram();
    Serial.printf("Free heap before TF initialization: %zu bytes\n", free_heap);
    Serial.printf("Free PSRAM before TF initialization: %zu bytes\n", free_psram);
    
    if (free_psram < kTensorArenaSize + 10000) {  // Need tensor arena + 10KB buffer
        Serial.println("ERROR: Insufficient PSRAM for TensorFlow Lite");
        return false;
    }
    #else
    Serial.printf("Free heap before TF initialization: %zu bytes\n", free_heap);
    
    if (free_heap < kTensorArenaSize + 15000) {  // Need tensor arena + 15KB buffer
        Serial.println("ERROR: Insufficient memory for TensorFlow Lite");
        return false;
    }
    #endif
    
    // Load the model from PROGMEM
    model_ = tflite::GetModel(g_hey_esp_model_data);
    if (model_->version() != TFLITE_SCHEMA_VERSION) {
        Serial.printf("ERROR: Model schema version %d != supported version %d\n", 
                     model_->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }
    
    // Allocate tensor arena (use PSRAM if available)
    #ifdef BOARD_HAS_PSRAM
    tensor_arena_ = (uint8_t*)ps_malloc(kTensorArenaSize);
    Serial.printf("Allocating %d bytes for tensor arena in PSRAM\n", kTensorArenaSize);
    #else
    tensor_arena_ = (uint8_t*)malloc(kTensorArenaSize);
    Serial.printf("Allocating %d bytes for tensor arena in DRAM\n", kTensorArenaSize);
    #endif
    
    if (!tensor_arena_) {
        Serial.println("ERROR: Failed to allocate tensor arena");
        return false;
    }
    
    // Create tensor allocator
    tensor_allocator_ = new tflite::SimpleTensorAllocator(tensor_arena_, kTensorArenaSize);
    if (!tensor_allocator_) {
        Serial.println("ERROR: Failed to create tensor allocator");
        return false;
    }
    
    // Create interpreter
    interpreter_ = new tflite::MicroInterpreter(model_, *resolver_, tensor_allocator_, error_reporter_);
    if (!interpreter_) {
        Serial.println("ERROR: Failed to create interpreter");
        return false;
    }
    
    // Verify input tensor dimensions
    TfLiteTensor* input = interpreter_->input(0);
    if ((input->dims->size != 3) || 
        (input->dims->data[0] != 1) ||
        (input->dims->data[1] != kFeatureSliceCount) ||
        (input->dims->data[2] != kFeatureSliceSize) ||
        (input->type != kTfLiteFloat32)) {  // Expect 3D tensor [16, 96, 1] float32
        Serial.println("ERROR: Input tensor has wrong dimensions");
        Serial.printf("Expected: [%d, %d, 1] float32, size=3\n", kFeatureSliceCount, kFeatureSliceSize);
        Serial.printf("Got: [%d, %d, %d] type=%d, size=%d\n", 
                     input->dims->data[0], input->dims->data[1], 
                     input->dims->data[2], input->type, input->dims->size);
        return false;
    }
    
    // Allocate audio buffer
    audio_buffer_ = (int16_t*)malloc(kAudioBufferSize * sizeof(int16_t));
    if (!audio_buffer_) {
        Serial.println("ERROR: Failed to allocate audio buffer");
        return false;
    }
    
    // Initialize micro features
    TfLiteStatus features_status = InitializeMicroFeatures(error_reporter_);
    if (features_status != kTfLiteOk) {
        Serial.println("ERROR: Failed to initialize micro features");
        return false;
    }
    
    Serial.printf("TensorFlow Lite initialized successfully!\n");
    Serial.printf("Model input: [%d, %d, %d, %d]\n", 
                 input->dims->data[0], input->dims->data[1], 
                 input->dims->data[2], input->dims->data[3]);
    Serial.printf("Free heap after initialization: %zu bytes\n", ESP.getFreeHeap());
    
    return true;
}

bool TensorFlowWakeWord::processAudio(int16_t* audio_data, size_t audio_length) {
    if (!interpreter_ || !audio_buffer_) {
        return false;
    }
    
    // Add new audio data to buffer
    for (size_t i = 0; i < audio_length && audio_buffer_pos_ < kAudioBufferSize; i++) {
        audio_buffer_[audio_buffer_pos_++] = audio_data[i];
    }
    
    // Process when we have enough samples (512 samples = 32ms at 16kHz)
    if (audio_buffer_pos_ >= kMaxAudioSampleSize) {
        // Get input tensor
        TfLiteTensor* input = interpreter_->input(0);
        
        // Create temporary buffer for uint8 features
        uint8_t features_buffer[kFeatureElementCount];
        
        // Extract features using TensorFlow micro frontend
        size_t num_samples_read;
        TfLiteStatus feature_status = GenerateMicroFeatures(
            error_reporter_, 
            audio_buffer_, 
            kMaxAudioSampleSize,
            kFeatureElementCount,
            features_buffer,  // Use temporary uint8 buffer
            &num_samples_read
        );
        
        if (feature_status != kTfLiteOk) {
            Serial.println("ERROR: Feature extraction failed");
            return false;
        }
        
        // Convert uint8 features to float32 for model input
        for (int i = 0; i < kFeatureElementCount; i++) {
            input->data.f[i] = features_buffer[i] / 255.0f;  // Normalize to 0.0-1.0
        }
        
        // Run inference
        TfLiteStatus invoke_status = interpreter_->Invoke();
        if (invoke_status != kTfLiteOk) {
            Serial.println("ERROR: Inference failed");
            return false;
        }
        
        // Process output
        TfLiteTensor* output = interpreter_->output(0);
        
        // Check output type and handle accordingly
        float hey_esp_score, silence_score, unknown_score;
        if (output->type == kTfLiteFloat32) {
            // Float32 output (0.0 - 1.0 range)
            hey_esp_score = output->data.f[kHeyEspIndex];
            silence_score = output->data.f[kSilenceIndex];
            unknown_score = output->data.f[kUnknownIndex];
            confidence_ = hey_esp_score;  // Already in 0.0-1.0 range
        } else if (output->type == kTfLiteUInt8) {
            // Uint8 output (0 - 255 range)
            hey_esp_score = output->data.uint8[kHeyEspIndex] / 255.0f;
            silence_score = output->data.uint8[kSilenceIndex] / 255.0f;
            unknown_score = output->data.uint8[kUnknownIndex] / 255.0f;
            confidence_ = hey_esp_score;
        } else {
            Serial.printf("ERROR: Unsupported output tensor type: %d\n", output->type);
            return false;
        }
        
        // Check for wake word with threshold and cooldown
        unsigned long current_time = millis();
        if (confidence_ > 0.6f && // 60% confidence threshold
            (current_time - last_detection_time_) > kDetectionCooldownMs) {
            wake_word_detected_ = true;
            last_detection_time_ = current_time;
            
            Serial.printf("WAKE WORD DETECTED! Confidence: %.2f%% (HeyESP:%.2f, Silence:%.2f, Unknown:%.2f)\n", 
                         confidence_ * 100.0f, hey_esp_score, silence_score, unknown_score);
        }
        
        // Shift audio buffer (keep last 25% for overlap)
        size_t keep_samples = kMaxAudioSampleSize / 4;
        memmove(audio_buffer_, audio_buffer_ + kMaxAudioSampleSize - keep_samples, 
                keep_samples * sizeof(int16_t));
        audio_buffer_pos_ = keep_samples;
        
        return true;
    }
    
    return false;
}

bool TensorFlowWakeWord::isWakeWordDetected() {
    bool detected = wake_word_detected_;
    wake_word_detected_ = false; // Clear flag after reading
    return detected;
}

float TensorFlowWakeWord::getConfidence() {
    return confidence_;
}

void TensorFlowWakeWord::reset() {
    wake_word_detected_ = false;
    confidence_ = 0.0f;
    audio_buffer_pos_ = 0;
    if (audio_buffer_) {
        memset(audio_buffer_, 0, kAudioBufferSize * sizeof(int16_t));
    }
}
