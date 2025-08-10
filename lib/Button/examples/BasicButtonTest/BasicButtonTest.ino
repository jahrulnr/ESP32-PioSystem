/*
 * Basic Button Test Example
 * 
 * This example demonstrates basic button functionality using the InputManager library.
 * It shows how to detect button presses, releases, and hold events.
 * 
 * Hardware Requirements:
 * - ESP32 development board
 * - 4 push buttons connected to GPIO pins (with pull-up resistors)
 * 
 * Button Connections (with internal pull-ups):
 * - UP Button:     GPIO 4  -> Button -> GND
 * - DOWN Button:   GPIO 5  -> Button -> GND  
 * - SELECT Button: GPIO 9  -> Button -> GND
 * - BACK Button:   GPIO 6  -> Button -> GND
 * 
 * Note: Internal pull-ups are used, so buttons connect GPIO to GND when pressed.
 * 
 * Author: Your Name
 * Date: 2025
 */

#include <Arduino.h>
#include "input_manager.h"

// Create input manager instance
InputManager* inputMgr;

// Button names for display
const char* buttonNames[] = {
    "UP",
    "DOWN", 
    "SELECT",
    "BACK"
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Basic Button Test ===");
    Serial.println("Press any button to see events!");
    Serial.println();
    
    // Create and initialize input manager
    inputMgr = new InputManager();
    inputMgr->init();
    
    // Set hold threshold to 2 seconds for this demo
    inputMgr->setHoldThreshold(2000);
    
    Serial.println("Input manager initialized!");
    Serial.println("Button mapping:");
    Serial.println("- UP:     GPIO 4");
    Serial.println("- DOWN:   GPIO 5"); 
    Serial.println("- SELECT: GPIO 9");
    Serial.println("- BACK:   GPIO 6");
    Serial.println();
    Serial.println("Events: P=Pressed, R=Released, H=Held");
    Serial.println("=====================================");
}

void loop() {
    // Update button states
    inputMgr->update();
    
    // Check each button for events
    for (int i = 0; i < BTN_COUNT; i++) {
        ButtonIndex btn = (ButtonIndex)i;
        ButtonEvent event = inputMgr->getButtonEvent(btn);
        
        switch (event) {
            case BTN_PRESSED:
                Serial.printf("[P] %s button pressed\n", buttonNames[i]);
                break;
                
            case BTN_RELEASED:
                Serial.printf("[R] %s button released\n", buttonNames[i]);
                break;
                
            case BTN_HELD:
                Serial.printf("[H] %s button held (2+ seconds)\n", buttonNames[i]);
                // Clear the held state to avoid repeated messages
                inputMgr->clearButton(btn);
                break;
                
            case BTN_NONE:
                // No event for this button
                break;
        }
    }
    
    // Demonstrate additional functionality every 5 seconds
    static unsigned long lastDemo = 0;
    if (millis() - lastDemo >= 5000) {
        demonstrateFeatures();
        lastDemo = millis();
    }
    
    delay(10); // Small delay for responsiveness
}

void demonstrateFeatures() {
    Serial.println("\n--- Button Status Check ---");
    
    // Show current button states
    bool anyPressed = false;
    for (int i = 0; i < BTN_COUNT; i++) {
        ButtonIndex btn = (ButtonIndex)i;
        if (inputMgr->isPressed(btn)) {
            Serial.printf("%s button is currently pressed\n", buttonNames[i]);
            anyPressed = true;
        }
    }
    
    if (!anyPressed) {
        Serial.println("No buttons currently pressed");
    }
    
    // Show if any button is pressed using utility function
    if (inputMgr->anyButtonPressed()) {
        Serial.println("At least one button is active");
    } else {
        Serial.println("All buttons are idle");
    }
    
    Serial.println("-----------------------------\n");
}

// Example function showing how to handle specific button combinations
void handleButtonCombinations() {
    // Example: UP + DOWN pressed together
    if (inputMgr->isPressed(BTN_UP) && inputMgr->isPressed(BTN_DOWN)) {
        Serial.println("🔄 UP + DOWN combination detected!");
        // Clear both buttons to avoid repeated triggers
        inputMgr->clearButton(BTN_UP);
        inputMgr->clearButton(BTN_DOWN);
    }
    
    // Example: SELECT held for special action
    if (inputMgr->isHeld(BTN_SELECT)) {
        Serial.println("⭐ SELECT held - Special action!");
        inputMgr->clearButton(BTN_SELECT);
    }
    
    // Example: BACK button for exit/cancel actions
    if (inputMgr->wasPressed(BTN_BACK)) {
        Serial.println("⬅️  BACK pressed - Cancel/Exit action");
    }
}

// Example interrupt-style button checking
void checkEmergencyButton() {
    // Quick check without waiting for update cycle
    // Useful for emergency stop or critical actions
    if (inputMgr->isPressed(BTN_SELECT) && inputMgr->isPressed(BTN_BACK)) {
        Serial.println("🚨 EMERGENCY: SELECT + BACK pressed!");
        Serial.println("Emergency stop procedure would trigger here");
        
        // Clear all buttons
        inputMgr->clearAllButtons();
        
        // Add your emergency handling code here
        // Examples:
        // - Stop all motors
        // - Turn off dangerous outputs  
        // - Save critical data
        // - Enter safe mode
    }
}
