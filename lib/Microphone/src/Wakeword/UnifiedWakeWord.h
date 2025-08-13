#ifndef UNIFIED_WAKE_WORD_H
#define UNIFIED_WAKE_WORD_H

#include <Arduino.h>

enum WakeWordEngine {
    TENSORFLOW_ENGINE   // TensorFlow Lite implementation  
};

class UnifiedWakeWord {
public:
    UnifiedWakeWord(WakeWordEngine engine = TENSORFLOW_ENGINE);
    ~UnifiedWakeWord();
    
    bool initialize();
    bool processAudio(int16_t* audio_data, size_t audio_length);
    bool isWakeWordDetected();
    float getConfidence();
    void reset();
    
    // Engine management
    bool switchEngine(WakeWordEngine engine);
    WakeWordEngine getCurrentEngine() const { return current_engine_; }
    
private:
    WakeWordEngine current_engine_;
    void* engine_instance_;  // Pointer to current engine instance
    
#ifndef ESP32_MEMORY_LIMITED
    bool initializeTensorFlowEngine();
#endif
    void cleanupCurrentEngine();
};

#endif // UNIFIED_WAKE_WORD_H
