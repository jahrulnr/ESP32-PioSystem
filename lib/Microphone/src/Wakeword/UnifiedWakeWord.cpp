#include "UnifiedWakeWord.h"
#ifndef ESP32_MEMORY_LIMITED
#include "TensorFlowWakeWord.h"
#endif

UnifiedWakeWord::UnifiedWakeWord(WakeWordEngine engine) 
    : current_engine_(engine)
    , engine_instance_(nullptr) {
}

UnifiedWakeWord::~UnifiedWakeWord() {
    cleanupCurrentEngine();
}

bool UnifiedWakeWord::initialize() {
    Serial.printf("Initializing unified wake word with engine: %s\n", 
                 "TENSORFLOW");
    
    switch (current_engine_) {
        case TENSORFLOW_ENGINE:
            return initializeTensorFlowEngine();
        default:
            Serial.println("ERROR: Unknown wake word engine");
            return false;
    }
}

#ifndef ESP32_MEMORY_LIMITED
bool UnifiedWakeWord::initializeTensorFlowEngine() {
    TensorFlowWakeWord* detector = new TensorFlowWakeWord();
    if (!detector) {
        Serial.println("ERROR: Failed to create TensorFlow wake word detector");
        return false;
    }
    
    if (!detector->initialize()) {
        Serial.println("ERROR: Failed to initialize TensorFlow wake word detector");
        delete detector;
        return false;
    }
    
    engine_instance_ = detector;
    Serial.println("TensorFlow wake word engine initialized successfully");
    return true;
}
#endif

void UnifiedWakeWord::cleanupCurrentEngine() {
    if (engine_instance_) {
#ifndef ESP32_MEMORY_LIMITED
        switch (current_engine_) {
            case TENSORFLOW_ENGINE:
                delete static_cast<TensorFlowWakeWord*>(engine_instance_);
                break;
        }
#endif
        engine_instance_ = nullptr;
    }
}

bool UnifiedWakeWord::processAudio(int16_t* audio_data, size_t audio_length) {
    if (!engine_instance_) {
        return false;
    }
    
#ifndef ESP32_MEMORY_LIMITED
    switch (current_engine_) {
        case TENSORFLOW_ENGINE:
            return static_cast<TensorFlowWakeWord*>(engine_instance_)->processAudio(audio_data, audio_length);
        default:
            return false;
    }
#else
    return false;
#endif
}

bool UnifiedWakeWord::isWakeWordDetected() {
    if (!engine_instance_) return false;
    
#ifndef ESP32_MEMORY_LIMITED
    switch (current_engine_) {
        case TENSORFLOW_ENGINE:
            return static_cast<TensorFlowWakeWord*>(engine_instance_)->isWakeWordDetected();
        default:
            return false;
    }
#else
    return false;
#endif
}

float UnifiedWakeWord::getConfidence() {
    if (!engine_instance_) {
        return 0.0f;
    }
    
#ifndef ESP32_MEMORY_LIMITED
    switch (current_engine_) {
        case TENSORFLOW_ENGINE:
            return static_cast<TensorFlowWakeWord*>(engine_instance_)->getConfidence();
        default:
            return 0.0f;
    }
#else
    return 0.0f;
#endif
}

void UnifiedWakeWord::reset() {
    if (!engine_instance_) {
        return;
    }
    
#ifndef ESP32_MEMORY_LIMITED
    switch (current_engine_) {
        case TENSORFLOW_ENGINE:
            static_cast<TensorFlowWakeWord*>(engine_instance_)->reset();
            break;
    }
#endif
}

bool UnifiedWakeWord::switchEngine(WakeWordEngine engine) {
    if (engine == current_engine_) {
        return true; // Already using this engine
    }
    
    Serial.printf("Switching wake word engine from %s to %s\n",
                 "TENSORFLOW",
                 "TENSORFLOW");
    
    // Cleanup current engine
    cleanupCurrentEngine();
    
    // Switch to new engine
    current_engine_ = engine;
    
    // Initialize new engine
    return initialize();
}
