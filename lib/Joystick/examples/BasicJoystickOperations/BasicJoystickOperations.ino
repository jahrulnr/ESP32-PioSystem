/*
 * Basic Joystick Operations Example
 * 
 * This example demonstrates fundamental joystick operations using the JoystickManager library.
 * It shows how to setup, read, and process analog joystick input with switch buttons.
 * 
 * Features:
 * - Single and dual joystick setup
 * - Raw and normalized value reading
 * - Direction detection and switch handling
 * - Real-time joystick monitoring
 * - Deadzone and threshold configuration
 * 
 * Hardware Requirements:
 * - ESP32-S3 development board
 * - 1-2 analog joystick modules
 * 
 * Joystick Connections:
 * Joystick 1:                 Joystick 2 (optional):
 * - VRX → GPIO 16             - VRX → GPIO 8
 * - VRY → GPIO 15             - VRY → GPIO 18
 * - SW  → GPIO 7              - SW  → GPIO 17
 * - VCC → 3.3V                - VCC → 3.3V
 * - GND → GND                 - GND → GND
 * 
 * Author: Your Name
 * Date: 2025
 */

#include <Arduino.h>
#include "joystick_manager.h"

// Create JoystickManager instance for up to 2 joysticks
JoystickManager* joystickMgr;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== JoystickManager Basic Operations Demo ===");
    Serial.println();
    
    // Initialize JoystickManager
    joystickMgr = new JoystickManager(2); // Support up to 2 joysticks
    
    // Add joysticks with pin configurations
    setupJoysticks();
    
    // Initialize the joystick manager
    joystickMgr->init();
    
    Serial.println("✅ JoystickManager initialized successfully!");
    
    // Show initial configuration
    showConfiguration();
    
    // Run demonstrations
    demonstrateBasicReading();
    Serial.println("\\n🎮 Starting real-time joystick monitoring...");
    Serial.println("Move joysticks and press buttons to see live data!");
    Serial.println("(Data updates every 100ms)");
    Serial.println();
}

void loop() {
    // Update joystick states
    joystickMgr->update();
    
    // Monitor joysticks every 100ms
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate >= 100) {
        monitorJoysticks();
        lastUpdate = millis();
    }
    
    // Handle joystick events
    handleJoystickEvents();
    
    delay(10);
}

void setupJoysticks() {
    Serial.println("🔧 Setting up joysticks...");
    
    // Add first joystick using default pins from config
    if (joystickMgr->addJoystick(JOYSTICK1_VRX_PIN, JOYSTICK1_VRY_PIN, JOYSTICK1_SW_PIN)) {
        Serial.printf("✅ Joystick 1 added - VRX:%d, VRY:%d, SW:%d\\n", 
                      JOYSTICK1_VRX_PIN, JOYSTICK1_VRY_PIN, JOYSTICK1_SW_PIN);
    } else {
        Serial.println("❌ Failed to add Joystick 1");
    }
    
    // Add second joystick (optional)
    if (joystickMgr->addJoystick(JOYSTICK2_VRX_PIN, JOYSTICK2_VRY_PIN, JOYSTICK2_SW_PIN)) {
        Serial.printf("✅ Joystick 2 added - VRX:%d, VRY:%d, SW:%d\\n", 
                      JOYSTICK2_VRX_PIN, JOYSTICK2_VRY_PIN, JOYSTICK2_SW_PIN);
    } else {
        Serial.println("ℹ️  Joystick 2 not connected (optional)");
    }
    
    Serial.printf("Total joysticks: %d\\n", joystickMgr->getJoystickCount());
}

void showConfiguration() {
    Serial.println("\\n📊 Joystick Configuration:");
    Serial.println("===========================");
    
    Serial.printf("Deadzone threshold: %d\\n", joystickMgr->getDeadzone());
    Serial.printf("Direction threshold: %d\\n", joystickMgr->getDirectionThreshold());
    Serial.printf("Debounce delay: %lu ms\\n", joystickMgr->getDebounceDelay());
    
    // Show individual joystick configurations
    for (int i = 0; i < joystickMgr->getJoystickCount(); i++) {
        JoystickPin pins = joystickMgr->getJoystickPins(i);
        Serial.printf("\\nJoystick %d pins - VRX:%d, VRY:%d, SW:%d\\n", 
                      i, pins.vrx, pins.vry, pins.sw);
    }
}

void demonstrateBasicReading() {
    Serial.println("\\n📖 Basic Reading Demo:");
    Serial.println("======================");
    
    // Take several readings to show data
    for (int reading = 0; reading < 5; reading++) {
        joystickMgr->update();
        
        Serial.printf("Reading %d:\\n", reading + 1);
        
        for (int i = 0; i < joystickMgr->getJoystickCount(); i++) {
            // Get raw values
            int rawX = joystickMgr->getRawX(i);
            int rawY = joystickMgr->getRawY(i);
            
            // Get normalized values (-100 to 100)
            int normX = joystickMgr->getNormalizedX(i);
            int normY = joystickMgr->getNormalizedY(i);
            
            // Get direction
            int direction = joystickMgr->getDirection(i);
            String directionName = getDirectionName(direction);
            
            // Get switch state
            bool switchPressed = joystickMgr->isSwitchPressed(i);
            
            Serial.printf("  Joystick %d - Raw:(%d,%d) Norm:(%d,%d) Dir:%s SW:%s\\n",
                         i, rawX, rawY, normX, normY, 
                         directionName.c_str(), switchPressed ? "PRESSED" : "RELEASED");
        }
        
        delay(500);
    }
}

void monitorJoysticks() {
    static bool headerPrinted = false;
    
    // Print header occasionally
    if (!headerPrinted || (millis() % 5000) < 100) {
        Serial.println("\\n📊 Live Joystick Data:");
        Serial.println("J# |  Raw X  |  Raw Y  | Norm X | Norm Y | Direction   | Switch");
        Serial.println("---|---------|---------|--------|--------|-----------  |-------");
        headerPrinted = true;
    }
    
    for (int i = 0; i < joystickMgr->getJoystickCount(); i++) {
        JoystickData data = joystickMgr->getJoystickData(i);
        String dirName = getDirectionName(data.direction);
        String swState = data.switchPressed ? "PRESS" : "     ";
        
        Serial.printf("%d  | %7d | %7d | %6d | %6d | %-11s | %s\\n",
                     i, data.rawX, data.rawY, data.normalizedX, data.normalizedY,
                     dirName.c_str(), swState.c_str());
    }
}

void handleJoystickEvents() {
    for (int i = 0; i < joystickMgr->getJoystickCount(); i++) {
        // Handle switch press events
        if (joystickMgr->wasSwitchPressed(i)) {
            Serial.printf("🔘 Joystick %d switch PRESSED!\\n", i);
            
            // Example: Print current joystick state when switch is pressed
            JoystickData data = joystickMgr->getJoystickData(i);
            Serial.printf("   Position: (%d, %d) Direction: %s\\n", 
                         data.normalizedX, data.normalizedY, 
                         getDirectionName(data.direction).c_str());
        }
        
        if (joystickMgr->wasSwitchReleased(i)) {
            Serial.printf("🔘 Joystick %d switch RELEASED!\\n", i);
        }
        
        // Handle direction changes (only print when direction changes)
        static int lastDirection[4] = {-1, -1, -1, -1}; // Support up to 4 joysticks
        int currentDirection = joystickMgr->getDirection(i);
        
        if (currentDirection != lastDirection[i]) {
            if (currentDirection != JOYSTICK_CENTER) {
                Serial.printf("🎮 Joystick %d: %s\\n", i, getDirectionName(currentDirection).c_str());
            }
            lastDirection[i] = currentDirection;
        }
        
        // Example: Detect specific directions
        if (joystickMgr->isUp(i)) {
            static unsigned long lastUpMessage[4] = {0};
            if (millis() - lastUpMessage[i] > 1000) { // Limit messages
                Serial.printf("⬆️  Joystick %d: Moving UP!\\n", i);
                lastUpMessage[i] = millis();
            }
        }
        
        if (joystickMgr->isPressed(i)) {
            static unsigned long lastPressedMessage[4] = {0};
            if (millis() - lastPressedMessage[i] > 1000) { // Limit messages
                Serial.printf("🎯 Joystick %d: Active (not centered)\\n", i);
                lastPressedMessage[i] = millis();
            }
        }
    }
}

String getDirectionName(int direction) {
    switch (direction) {
        case JOYSTICK_CENTER:     return "CENTER";
        case JOYSTICK_UP:         return "UP";
        case JOYSTICK_DOWN:       return "DOWN";
        case JOYSTICK_LEFT:       return "LEFT";
        case JOYSTICK_RIGHT:      return "RIGHT";
        case JOYSTICK_UP_LEFT:    return "UP-LEFT";
        case JOYSTICK_UP_RIGHT:   return "UP-RIGHT";
        case JOYSTICK_DOWN_LEFT:  return "DOWN-LEFT";
        case JOYSTICK_DOWN_RIGHT: return "DOWN-RIGHT";
        default:                  return "UNKNOWN";
    }
}

/*
 * Advanced Usage Examples:
 * 
 * 1. Custom deadzone and threshold:
 *    joystickMgr->setDeadzone(500);           // Larger deadzone
 *    joystickMgr->setDirectionThreshold(800); // Higher threshold for directions
 * 
 * 2. Multiple joystick setup:
 *    joystickMgr->setupDefaultTwoJoysticks(); // Quick dual setup
 * 
 * 3. Single joystick with custom pins:
 *    joystickMgr->setupSingleJoystick(34, 35, 32); // Custom pins
 * 
 * 4. Check for diagonal movement:
 *    if (joystickMgr->isDiagonal(0)) {
 *        Serial.println("Diagonal movement detected!");
 *    }
 * 
 * 5. Get full joystick data structure:
 *    JoystickData data = joystickMgr->getJoystickData(0);
 *    Serial.printf("Center: (%d, %d)\\n", data.centerX, data.centerY);
 */
