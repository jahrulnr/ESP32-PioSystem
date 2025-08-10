/*
 * Advanced Button Control Example
 * 
 * This example demonstrates advanced button handling techniques including:
 * - Custom debouncing and timing
 * - Multi-button combinations and sequences
 * - Button state machines
 * - Custom hold thresholds per button
 * - Button event logging and analysis
 * - Input validation and filtering
 * 
 * This example is perfect for understanding how to implement sophisticated
 * input systems for complex applications like games or control systems.
 * 
 * Hardware Requirements:
 * - ESP32 development board
 * - 4 push buttons (UP, DOWN, SELECT, BACK)
 * - Optional: LED for visual feedback
 * 
 * Author: Your Name
 * Date: 2025
 */

#include <Arduino.h>
#include "input_manager.h"

// Create input manager instance
InputManager* inputMgr;

// Advanced button tracking
struct AdvancedButton {
    unsigned long totalPresses;
    unsigned long totalHolds;
    unsigned long longestHold;
    unsigned long averageHoldTime;
    unsigned long lastEventTime;
    bool inSequence;
};

AdvancedButton advancedButtons[BTN_COUNT];

// Button sequence detection
#define MAX_SEQUENCE_LENGTH 8
ButtonIndex buttonSequence[MAX_SEQUENCE_LENGTH];
int sequencePosition = 0;
unsigned long sequenceStartTime = 0;
const unsigned long SEQUENCE_TIMEOUT = 3000; // 3 seconds

// Special sequences to detect
const ButtonIndex konamiCode[] = {BTN_UP, BTN_UP, BTN_DOWN, BTN_DOWN, BTN_SELECT, BTN_BACK};
const int konamiLength = 6;

// Multi-button state tracking
struct ButtonCombination {
    ButtonIndex btn1, btn2;
    const char* name;
    void (*action)();
};

// Forward declarations
void actionReboot();
void actionFactory();
void actionDebug();
void actionCalibrate();

// Define button combinations
ButtonCombination combinations[] = {
    {BTN_UP, BTN_DOWN, "UP+DOWN", actionReboot},
    {BTN_SELECT, BTN_BACK, "SELECT+BACK", actionFactory},
    {BTN_UP, BTN_SELECT, "UP+SELECT", actionDebug},
    {BTN_DOWN, BTN_BACK, "DOWN+BACK", actionCalibrate}
};

const int numCombinations = sizeof(combinations) / sizeof(ButtonCombination);

// Timing analysis
unsigned long sessionStartTime;
unsigned long totalButtonEvents = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Advanced Button Control Demo ===");
    Serial.println();
    
    // Initialize input manager
    inputMgr = new InputManager();
    inputMgr->init();
    
    // Initialize advanced tracking
    sessionStartTime = millis();
    for (int i = 0; i < BTN_COUNT; i++) {
        advancedButtons[i] = {0, 0, 0, 0, 0, false};
    }
    
    Serial.println("🎮 Advanced Features:");
    Serial.println("- Button sequence detection (try UP,UP,DOWN,DOWN,SELECT,BACK)");
    Serial.println("- Multi-button combinations");
    Serial.println("- Statistical analysis");
    Serial.println("- Custom timing control");
    Serial.println();
    
    showInstructions();
}

void loop() {
    // Update button states
    inputMgr->update();
    
    // Handle individual button events
    handleButtonEvents();
    
    // Check for button combinations
    checkButtonCombinations();
    
    // Update button sequences
    updateSequenceDetection();
    
    // Periodic status updates
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus >= 10000) { // Every 10 seconds
        showStatistics();
        lastStatus = millis();
    }
    
    delay(10);
}

void handleButtonEvents() {
    for (int i = 0; i < BTN_COUNT; i++) {
        ButtonIndex btn = (ButtonIndex)i;
        ButtonEvent event = inputMgr->getButtonEvent(btn);
        
        if (event != BTN_NONE) {
            totalButtonEvents++;
            advancedButtons[i].lastEventTime = millis();
            
            switch (event) {
                case BTN_PRESSED:
                    handleButtonPressed(btn);
                    break;
                    
                case BTN_RELEASED:
                    handleButtonReleased(btn);
                    break;
                    
                case BTN_HELD:
                    handleButtonHeld(btn);
                    break;
                    
                default:
                    break;
            }
        }
    }
}

void handleButtonPressed(ButtonIndex btn) {
    const char* names[] = {"UP", "DOWN", "SELECT", "BACK"};
    Serial.printf("⬇️  [PRESS] %s\n", names[btn]);
    
    advancedButtons[btn].totalPresses++;
    
    // Add to sequence
    addToSequence(btn);
    
    // Custom actions per button
    switch (btn) {
        case BTN_UP:
            // Demonstrate rapid repeat
            if (inputMgr->isPressed(BTN_UP)) {
                Serial.println("   🔄 Rapid UP repeat enabled");
            }
            break;
            
        case BTN_DOWN:
            // Toggle behavior demonstration
            static bool downToggle = false;
            downToggle = !downToggle;
            Serial.printf("   🔄 DOWN toggle state: %s\n", downToggle ? "ON" : "OFF");
            break;
            
        case BTN_SELECT:
            // Precision timing demonstration
            Serial.printf("   ⏱️  Precise timing: %lu ms\n", millis() % 10000);
            break;
            
        case BTN_BACK:
            // Context-sensitive behavior
            Serial.println("   ⬅️  Context: Advanced demo mode");
            break;
    }
}

void handleButtonReleased(ButtonIndex btn) {
    const char* names[] = {"UP", "DOWN", "SELECT", "BACK"};
    Serial.printf("⬆️  [RELEASE] %s\n", names[btn]);
}

void handleButtonHeld(ButtonIndex btn) {
    const char* names[] = {"UP", "DOWN", "SELECT", "BACK"};
    Serial.printf("🔒 [HELD] %s (Hold #%lu)\n", names[btn], ++advancedButtons[btn].totalHolds);
    
    // Calculate hold duration
    unsigned long holdDuration = millis() - advancedButtons[btn].lastEventTime;
    if (holdDuration > advancedButtons[btn].longestHold) {
        advancedButtons[btn].longestHold = holdDuration;
        Serial.printf("   📈 New longest hold: %lu ms\n", holdDuration);
    }
    
    // Clear to avoid repeated messages
    inputMgr->clearButton(btn);
    
    // Special hold actions
    switch (btn) {
        case BTN_UP:
            Serial.println("   🚀 UP held - Turbo mode!");
            break;
        case BTN_DOWN:
            Serial.println("   🐌 DOWN held - Slow mode!");
            break;
        case BTN_SELECT:
            Serial.println("   ⭐ SELECT held - Special function!");
            break;
        case BTN_BACK:
            Serial.println("   🚨 BACK held - Emergency mode!");
            break;
    }
}

void checkButtonCombinations() {
    for (int i = 0; i < numCombinations; i++) {
        if (inputMgr->isPressed(combinations[i].btn1) && 
            inputMgr->isPressed(combinations[i].btn2)) {
            
            Serial.printf("🎯 COMBO: %s detected!\n", combinations[i].name);
            
            if (combinations[i].action != nullptr) {
                combinations[i].action();
            }
            
            // Clear both buttons
            inputMgr->clearButton(combinations[i].btn1);
            inputMgr->clearButton(combinations[i].btn2);
        }
    }
}

void addToSequence(ButtonIndex btn) {
    unsigned long currentTime = millis();
    
    // Reset sequence if timeout
    if (currentTime - sequenceStartTime > SEQUENCE_TIMEOUT) {
        sequencePosition = 0;
    }
    
    // Add button to sequence
    if (sequencePosition < MAX_SEQUENCE_LENGTH) {
        buttonSequence[sequencePosition] = btn;
        sequencePosition++;
        
        if (sequencePosition == 1) {
            sequenceStartTime = currentTime;
        }
        
        Serial.printf("🔗 Sequence: [%d/%d] ", sequencePosition, MAX_SEQUENCE_LENGTH);
        for (int i = 0; i < sequencePosition; i++) {
            const char* names[] = {"UP", "DOWN", "SEL", "BCK"};
            Serial.printf("%s ", names[buttonSequence[i]]);
        }
        Serial.println();
    }
}

void updateSequenceDetection() {
    // Check for Konami code
    if (sequencePosition >= konamiLength) {
        bool match = true;
        int startPos = sequencePosition - konamiLength;
        
        for (int i = 0; i < konamiLength; i++) {
            if (buttonSequence[startPos + i] != konamiCode[i]) {
                match = false;
                break;
            }
        }
        
        if (match) {
            Serial.println("🎊 KONAMI CODE DETECTED! 🎊");
            Serial.println("🚀 30 lives unlocked! (Just kidding)");
            sequencePosition = 0; // Reset sequence
        }
    }
}

void showStatistics() {
    Serial.println("\n📊 === BUTTON STATISTICS ===");
    
    unsigned long sessionTime = millis() - sessionStartTime;
    Serial.printf("Session time: %lu seconds\n", sessionTime / 1000);
    Serial.printf("Total events: %lu\n", totalButtonEvents);
    Serial.printf("Events/minute: %.1f\n", (float)totalButtonEvents / (sessionTime / 60000.0));
    
    const char* names[] = {"UP", "DOWN", "SELECT", "BACK"};
    
    for (int i = 0; i < BTN_COUNT; i++) {
        Serial.printf("\n%s Button:\n", names[i]);
        Serial.printf("  Presses: %lu\n", advancedButtons[i].totalPresses);
        Serial.printf("  Holds: %lu\n", advancedButtons[i].totalHolds);
        Serial.printf("  Longest hold: %lu ms\n", advancedButtons[i].longestHold);
        
        if (advancedButtons[i].lastEventTime > 0) {
            unsigned long timeSince = millis() - advancedButtons[i].lastEventTime;
            Serial.printf("  Last used: %lu ms ago\n", timeSince);
        }
    }
    
    Serial.println("============================\n");
}

void showInstructions() {
    Serial.println("🎯 Try these advanced features:");
    Serial.println("- Single buttons: Basic press/hold");
    Serial.println("- UP+DOWN: Simulated reboot");
    Serial.println("- SELECT+BACK: Factory reset simulation");
    Serial.println("- UP+SELECT: Debug mode");
    Serial.println("- DOWN+BACK: Calibration mode");
    Serial.println("- Sequence: UP,UP,DOWN,DOWN,SELECT,BACK (Konami code)");
    Serial.println("- Hold any button for 1+ second");
    Serial.println();
}

// Button combination actions
void actionReboot() {
    Serial.println("🔄 REBOOT: System restart simulation...");
    Serial.println("   (In real application, this would restart the device)");
    delay(1000);
}

void actionFactory() {
    Serial.println("🏭 FACTORY RESET: Clearing all settings...");
    Serial.println("   (In real application, this would reset to defaults)");
    
    // Reset statistics as demonstration
    for (int i = 0; i < BTN_COUNT; i++) {
        advancedButtons[i] = {0, 0, 0, 0, 0, false};
    }
    totalButtonEvents = 0;
    sessionStartTime = millis();
    
    Serial.println("   ✅ Statistics reset complete!");
    delay(1000);
}

void actionDebug() {
    Serial.println("🐛 DEBUG MODE: Detailed button analysis...");
    
    Serial.println("   Current button states:");
    const char* names[] = {"UP", "DOWN", "SELECT", "BACK"};
    for (int i = 0; i < BTN_COUNT; i++) {
        ButtonIndex btn = (ButtonIndex)i;
        Serial.printf("   %s: %s%s\n", names[i], 
                     inputMgr->isPressed(btn) ? "PRESSED" : "released",
                     inputMgr->isHeld(btn) ? " (HELD)" : "");
    }
    delay(1000);
}

void actionCalibrate() {
    Serial.println("📐 CALIBRATION: Button sensitivity adjustment...");
    Serial.println("   (In real application, this would calibrate button response)");
    
    // Demonstrate custom hold threshold
    static bool fastMode = false;
    fastMode = !fastMode;
    
    if (fastMode) {
        inputMgr->setHoldThreshold(500); // Fast mode: 0.5 seconds
        Serial.println("   ⚡ Fast hold mode enabled (0.5s threshold)");
    } else {
        inputMgr->setHoldThreshold(1000); // Normal mode: 1 second
        Serial.println("   🐢 Normal hold mode enabled (1.0s threshold)");
    }
    
    delay(1000);
}
