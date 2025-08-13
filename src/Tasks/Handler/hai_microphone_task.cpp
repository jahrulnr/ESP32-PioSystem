#include "tasks.h"
#include "mic_config.h"

TaskHandle_t haiMicrophoneTaskHandle = NULL;

// External HAI functions from hai_menu.cpp
extern void processHAIAudio();
extern bool haiListening;
extern bool haiInitialized;

void haiMicrophoneTask(void* parameter) {
    DEBUG_PRINTLN("HAI Microphone Task started on Core 0");
    
    const TickType_t micProcessFrequency = pdMS_TO_TICKS(32); // ~31.25 Hz processing
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    for (;;) {
        // Wait for the next cycle
        vTaskDelayUntil(&lastWakeTime, micProcessFrequency);
        
        // Only process audio if HAI is initialized and listening
        if (haiInitialized && haiListening) {
            processHAIAudio();
        }
        
        // Yield to other tasks
        taskYIELD();
    }
}

void startHAIMicrophoneTask() {
    if (haiMicrophoneTaskHandle == NULL) {
        xTaskCreatePinnedToCore(
            haiMicrophoneTask,           // Task function
            "HAIMicrophoneTask",         // Task name
            MIC_TASK_STACK_SIZE,         // Stack size (8KB for TensorFlow processing)
            NULL,                        // Parameters
            2,                           // Priority (higher than display, lower than input)
            &haiMicrophoneTaskHandle,    // Task handle
            0                            // Core 0 (background processing)
        );
        
        if (haiMicrophoneTaskHandle != NULL) {
            DEBUG_PRINTLN("HAI Microphone Task created successfully");
        } else {
            DEBUG_PRINTLN("Failed to create HAI Microphone Task");
        }
    }
}

void stopHAIMicrophoneTask() {
    if (haiMicrophoneTaskHandle != NULL) {
        vTaskDelete(haiMicrophoneTaskHandle);
        haiMicrophoneTaskHandle = NULL;
        DEBUG_PRINTLN("HAI Microphone Task stopped");
    }
}
