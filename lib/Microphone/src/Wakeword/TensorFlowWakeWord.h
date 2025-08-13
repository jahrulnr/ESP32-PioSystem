#ifndef TENSORFLOW_WAKE_WORD_H
#define TENSORFLOW_WAKE_WORD_H

#include <Arduino.h>
#include "tensorflow/lite/experimental/micro/micro_error_reporter.h"
#include "tensorflow/lite/experimental/micro/micro_interpreter.h"
#include "tensorflow/lite/experimental/micro/simple_tensor_allocator.h"
#include "tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/experimental/micro/examples/micro_speech/micro_features/micro_features_generator.h"

// HAI-specific model settings to match hey_esp.tflite requirements
// These are the expected dimensions based on the error: [1, 49, 40, 1]
constexpr int kFeatureSliceSize = 96;     // Width of spectrogram 
constexpr int kFeatureSliceCount = 16;    // Height of spectrogram (time slices)
constexpr int kFeatureElementCount = (kFeatureSliceSize * kFeatureSliceCount); // 1960 elements

// Audio processing settings
constexpr int kMaxAudioSampleSize = 512;  // 32ms at 16kHz
constexpr int kAudioSampleFrequency = 16000;

// Model categories for hey_esp wake word model
constexpr int kSilenceIndex = 0;
constexpr int kUnknownIndex = 1;
constexpr int kHeyEspIndex = 2;
constexpr int kTensorFlowCategoryCount = 3;

// Model settings for hey_esp.tflite (using micro speech settings)
constexpr int kTensorArenaSize = 30 * 1024;  // 30KB for ESP32-S3 (increased from 10KB)

class TensorFlowWakeWord {
public:
    TensorFlowWakeWord();
    ~TensorFlowWakeWord();
    
    bool initialize();
    bool processAudio(int16_t* audio_data, size_t audio_length);
    bool isWakeWordDetected();
    float getConfidence();
    void reset();

private:
    // TensorFlow Lite components
    tflite::MicroErrorReporter* error_reporter_;
    const tflite::Model* model_;
    tflite::ops::micro::AllOpsResolver* resolver_;
    tflite::SimpleTensorAllocator* tensor_allocator_;
    tflite::MicroInterpreter* interpreter_;
    
    // Memory management
    uint8_t* tensor_arena_;
    
    // Audio processing
    int16_t* audio_buffer_;
    size_t audio_buffer_pos_;
    #ifdef CONFIG_IDF_TARGET_ESP32S3
    static constexpr size_t kAudioBufferSize = kMaxAudioSampleSize * 3; // 3 windows for ESP32-S3
    #else
    static constexpr size_t kAudioBufferSize = kMaxAudioSampleSize * 2; // 2 windows for ESP32 (reduced)
    #endif
    
    // Wake word detection
    bool wake_word_detected_;
    float confidence_;
    unsigned long last_detection_time_;
    static constexpr unsigned long kDetectionCooldownMs = 2000; // 2 second cooldown
};

#endif // TENSORFLOW_WAKE_WORD_H
