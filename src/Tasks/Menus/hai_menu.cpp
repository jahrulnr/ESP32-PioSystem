#include "menus.h"
#include "SerialDebug.h"
#include "mic_config.h"
#include "../../../lib/Microphone/src/Driver/AnalogMicrophone.h"
#include "../../../lib/Microphone/src/Wakeword/UnifiedWakeWord.h"
#include "../../../lib/Microphone/src/Wakeword/TensorFlowWakeWord.h"
#include "../Handler/tasks.h"

// Forward declarations
void executeHAIAction();
void displayHAIStatus();
void toggleHAIListening();
void calibrateHAIMicrophone();
void displayHAISettings();

// Helper function for drawing left-aligned text
void drawText(const String& text, int x, int y, uint16_t color, int textSize = 1) {
    displayManager.getTFT()->setTextColor(color);
    displayManager.getTFT()->setTextSize(textSize);
    displayManager.getTFT()->drawString(text, x, y);
}

// HAI system variables (accessible from other files)
AnalogMicrophone* haiMicrophone = nullptr;
TensorFlowWakeWord* haiWakeWord = nullptr;
bool haiInitialized = false;
bool haiListening = false;
unsigned long lastWakeWordTime = 0;
float lastConfidence = 0.0;

// HAI Menu State
HAIMenuState currentHAIMenuItem = HAI_STATUS;

// Initialize HAI system
void initializeHAI() {
    DEBUG_PRINTLN("Initializing HAI Voice Assistant...");
    
    if (!haiMicrophone) {
        haiMicrophone = new AnalogMicrophone(MIC_ANALOG_PIN);
        if (!haiMicrophone->init(MIC_SAMPLE_RATE)) {
            DEBUG_PRINTLN("Failed to initialize microphone");
            return;
        }
    }
    
    if (!haiWakeWord) {
        haiWakeWord = new TensorFlowWakeWord();
        if (!haiWakeWord->initialize()) {
            DEBUG_PRINTLN("Failed to initialize wake word engine");
            return;
        }
    }
    
    haiInitialized = true;
    DEBUG_PRINTLN("HAI system initialized successfully");
}

// Main HAI processing loop (called from microphone task)
void processHAIAudio() {
    if (!haiInitialized || !haiListening) {
        return;
    }
    
    // Read audio samples using the microphone timing
    if (haiMicrophone && haiMicrophone->isInitialized()) {
        int16_t samples[32]; // Small buffer for processing
        int samplesRead = haiMicrophone->readSamplesWithTiming(samples, 32);
        
        // Process with wake word detector
        if (haiWakeWord && samplesRead > 0) {
            bool detected = haiWakeWord->processAudio(samples, samplesRead);
            
            // Check for wake word detection
            if (detected || haiWakeWord->isWakeWordDetected()) {
                lastWakeWordTime = millis();
                lastConfidence = haiWakeWord->getConfidence();
                
                DEBUG_PRINTF("Wake word detected! Confidence: %.2f\n", lastConfidence);
                
                // Visual feedback
                displayManager.getTFT()->fillCircle(300, 50, 10, TFT_GREEN);
                
                // Reset wake word detector for next detection
                haiWakeWord->reset();
            }
        }
				DEBUG_PRINTF("[HAI] sample read: %d\n", samplesRead);
    }
}

// Display HAI main menu
void displayHAIMenu() {
    DEBUG_PRINTLN("HAI: Attempting to display HAI menu");
    
    displayManager.clearScreen();
    displayManager.drawTitle("HAI Voice Assistant");
    
    // HAI status indicator
    uint16_t statusColor = haiInitialized ? TFT_GREEN : TFT_RED;
    String statusText = haiInitialized ? "READY" : "ERROR";
    displayManager.drawCenteredText("Status: " + statusText, 35, statusColor, 1);
    
    // Listening indicator
    if (haiListening) {
        displayManager.drawCenteredText("🎤 LISTENING", 50, TFT_CYAN, 1);
        
        // Show audio level bar
        if (haiMicrophone) {
            int audioLevel = haiMicrophone->readLevel();
            int barWidth = map(audioLevel, 0, 4095, 0, 200);
            displayManager.getTFT()->fillRect(20, 65, barWidth, 8, TFT_GREEN);
            displayManager.getTFT()->drawRect(20, 65, 200, 8, TFT_WHITE);
        }
    }
    
    // Menu items
    String menuItems[] = {
        "Status & Info",
        "Toggle Listening", 
        "Calibrate Mic",
        "Settings",
        "Exit"
    };
    
    for (int i = 0; i < 5; i++) {
        uint16_t color = (i == currentHAIMenuItem) ? TFT_YELLOW : TFT_WHITE;
        String prefix = (i == currentHAIMenuItem) ? "> " : "  ";
        displayManager.drawCenteredText(prefix + menuItems[i], 90 + (i * 20), color, 1);
    }
    
    // Instructions
    displayManager.drawCenteredText("UP/DOWN: Navigate  SELECT: Action", 220, TFT_LIGHTGREY, 1);
    
    DEBUG_PRINTLN("HAI: Display updated");
}

// Execute HAI menu actions
void executeHAIAction() {
    DEBUG_PRINTF("HAI: Executing action for menu item %d\n", currentHAIMenuItem);
    switch (currentHAIMenuItem) {
        case HAI_STATUS:
            displayHAIStatus();
            break;
        case HAI_LISTENING:
            toggleHAIListening();
            break;
        case HAI_CALIBRATE:
            calibrateHAIMicrophone();
            break;
        case HAI_SETTINGS:
            displayHAISettings();
            break;
        case HAI_EXIT:
            currentMenu = MENU_MAIN;
            selectedMenuItem = 0;
            break;
    }
}

// Display detailed HAI status
void displayHAIStatus() {
    displayManager.clearScreen();
    displayManager.drawTitle("HAI System Status");
    
    int y = 40;
    
    // Initialization status
    String initStatus = haiInitialized ? "Initialized ✓" : "Not Initialized ✗";
    uint16_t initColor = haiInitialized ? TFT_GREEN : TFT_RED;
    drawText(initStatus, 10, y, initColor, 1);
    y += 20;
    
    if (haiInitialized) {
        // Microphone status
        drawText("Microphone:", 10, y, TFT_WHITE, 1);
        y += 15;
        if (haiMicrophone) {
            int currentLevel = haiMicrophone->readLevel();
            int avgLevel = haiMicrophone->readAverageLevel();
            drawText("Current: " + String(currentLevel), 20, y, TFT_CYAN, 1);
            y += 12;
            drawText("Average: " + String(avgLevel), 20, y, TFT_CYAN, 1);
            y += 12;
            drawText("Sample Rate: " + String(haiMicrophone->getSampleRate()) + " Hz", 20, y, TFT_CYAN, 1);
            y += 20;
        }
        
        // Wake word engine status
        drawText("Wake Word Engine:", 10, y, TFT_WHITE, 1);
        y += 15;
        if (haiWakeWord) {
            String engineName = "TensorFlow Lite";
            drawText("Engine: " + engineName, 20, y, TFT_CYAN, 1);
            y += 15;
            
            String listenStatus = haiListening ? "Active" : "Inactive";
            uint16_t listenColor = haiListening ? TFT_GREEN : TFT_ORANGE;
            drawText("Listening: " + listenStatus, 20, y, listenColor, 1);
            y += 15;
            
            // Last detection info
            if (lastWakeWordTime > 0) {
                drawText("Last Detection:", 10, y, TFT_WHITE, 1);
                y += 12;
                drawText("Time: " + String((millis() - lastWakeWordTime) / 1000) + "s ago", 20, y, TFT_GREEN, 1);
                y += 12;
                drawText("Confidence: " + String(lastConfidence, 2), 20, y, TFT_GREEN, 1);
                y += 15;
            }
        }
    }
    
    displayManager.drawCenteredText("Press BACK to return", 220, TFT_LIGHTGREY, 1);
}

// Toggle HAI listening state
void toggleHAIListening() {
    if (!haiInitialized) {
        // Try to initialize if not already done
        initializeHAI();
        if (!haiInitialized) {
            return;
        }
    }
    
    haiListening = !haiListening;
    DEBUG_PRINTF("HAI listening %s\n", haiListening ? "enabled" : "disabled");
    
    // Refresh the display to show new state
    displayHAI();
}

// Calibrate HAI microphone
void calibrateHAIMicrophone() {
    if (!haiMicrophone) {
        return;
    }
    
    displayManager.clearScreen();
    displayManager.drawTitle("Microphone Calibration");
    displayManager.drawCenteredText("Calibrating baseline noise level...", 80, TFT_CYAN, 1);
    displayManager.drawCenteredText("Please remain quiet", 100, TFT_WHITE, 1);
    
    // Perform calibration
    int baseline = haiMicrophone->calibrateBaseline();
    
    displayManager.clearScreen();
    displayManager.drawTitle("Calibration Complete");
    displayManager.drawCenteredText("Baseline Level: " + String(baseline), 80, TFT_GREEN, 1);
    displayManager.drawCenteredText("Calibration successful!", 100, TFT_GREEN, 1);
    displayManager.drawCenteredText("Press BACK to continue", 140, TFT_LIGHTGREY, 1);
    
    DEBUG_PRINTF("Microphone calibrated, baseline: %d\n", baseline);
}

// Display HAI settings
void displayHAISettings() {
    displayManager.clearScreen();
    displayManager.drawTitle("HAI Settings");
    
    int y = 40;
    
    drawText("Wake Word: Hey ESP", 10, y, TFT_WHITE, 1);
    y += 15;
    drawText("Engine: TensorFlow Lite", 10, y, TFT_CYAN, 1);
    y += 15;
    drawText("Sample Rate: " + String(MIC_SAMPLE_RATE) + " Hz", 10, y, TFT_CYAN, 1);
    y += 15;
    drawText("Buffer Size: 32 samples", 10, y, TFT_CYAN, 1);
    y += 20;
    
    drawText("Microphone Settings:", 10, y, TFT_WHITE, 1);
    y += 15;
    drawText("Pin: " + String(MIC_ANALOG_PIN), 20, y, TFT_CYAN, 1);
    y += 12;
    drawText("Gain: Auto", 20, y, TFT_CYAN, 1);
    y += 20;
    
    drawText("Memory Usage:", 10, y, TFT_WHITE, 1);
    y += 15;
    drawText("Stack Size: " + String(MIC_TASK_STACK_SIZE) + " bytes", 20, y, TFT_CYAN, 1);
    y += 12;
    drawText("Core: 0 (Background)", 20, y, TFT_CYAN, 1);
    
    displayManager.drawCenteredText("Press BACK to return", 220, TFT_LIGHTGREY, 1);
}

// Main HAI display function (called from display task and menu actions)
void displayHAI() {
    DEBUG_PRINTLN("HAI: displayHAI() called");
    displayHAIMenu();
}
