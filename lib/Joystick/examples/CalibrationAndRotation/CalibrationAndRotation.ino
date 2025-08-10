/*
 * Joystick Calibration and Rotation Example
 * 
 * This example demonstrates advanced joystick features including calibration,
 * rotation, and orientation adjustment for optimal gaming and control applications.
 * 
 * Features:
 * - Interactive calibration process
 * - Automatic center point detection
 * - Manual and automatic calibration modes
 * - 90°, 180°, 270° rotation support
 * - Axis inversion for different mounting orientations
 * - Calibration data persistence simulation
 * - Real-time calibration visualization
 * 
 * Hardware Requirements:
 * - ESP32-S3 development board
 * - 1-2 analog joystick modules
 * - Serial monitor for interactive calibration
 * 
 * Joystick Connections:
 * Joystick 1:
 * - VRX → GPIO 16
 * - VRY → GPIO 15
 * - SW  → GPIO 7
 * - VCC → 3.3V
 * - GND → GND
 * 
 * Author: Your Name
 * Date: 2025
 */

#include <Arduino.h>
#include "joystick_manager.h"

// Create JoystickManager instance
JoystickManager* joystickMgr;

// Calibration state
bool calibrationMode = false;
int calibrationStep = 0;
unsigned long calibrationStartTime = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Joystick Calibration & Rotation Demo ===");
    Serial.println();
    
    // Initialize JoystickManager
    joystickMgr = new JoystickManager(2);
    
    // Setup joysticks
    setupJoysticks();
    joystickMgr->init();
    
    Serial.println("✅ JoystickManager initialized!");
    
    // Show menu
    showMenu();
}

void loop() {
    // Update joystick states
    joystickMgr->update();
    
    // Handle serial input for interactive commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\\n');
        command.trim();
        processCommand(command);
    }
    
    // Handle calibration process
    if (calibrationMode) {
        handleCalibration();
    } else {
        // Normal monitoring
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate >= 200) {
            showJoystickStatus();
            lastUpdate = millis();
        }
    }
    
    delay(10);
}

void setupJoysticks() {
    Serial.println("🔧 Setting up joysticks...");
    
    // Add joystick with default configuration
    if (joystickMgr->addJoystick(JOYSTICK1_VRX_PIN, JOYSTICK1_VRY_PIN, JOYSTICK1_SW_PIN)) {
        Serial.printf("✅ Joystick 0 added - VRX:%d, VRY:%d, SW:%d\\n", 
                      JOYSTICK1_VRX_PIN, JOYSTICK1_VRY_PIN, JOYSTICK1_SW_PIN);
    }
    
    // Optional second joystick
    if (joystickMgr->addJoystick(JOYSTICK2_VRX_PIN, JOYSTICK2_VRY_PIN, JOYSTICK2_SW_PIN)) {
        Serial.printf("✅ Joystick 1 added - VRX:%d, VRY:%d, SW:%d\\n", 
                      JOYSTICK2_VRX_PIN, JOYSTICK2_VRY_PIN, JOYSTICK2_SW_PIN);
    }
}

void showMenu() {
    Serial.println("\\n📋 Available Commands:");
    Serial.println("======================");
    Serial.println("1. 'calibrate [index]'  - Start interactive calibration");
    Serial.println("2. 'auto [index]'       - Auto-calibrate (move joystick around)");
    Serial.println("3. 'rotate [index] [degrees]' - Set rotation (0, 90, 180, 270)");
    Serial.println("4. 'invertx [index]'    - Toggle X-axis inversion");
    Serial.println("5. 'inverty [index]'    - Toggle Y-axis inversion");
    Serial.println("6. 'reset [index]'      - Reset calibration and orientation");
    Serial.println("7. 'status'             - Show current joystick status");
    Serial.println("8. 'debug [index]'      - Show detailed debug info");
    Serial.println("9. 'menu'               - Show this menu");
    Serial.println("\\nExample: 'calibrate 0' or 'rotate 0 90'");
    Serial.println("Type a command and press Enter...");
    Serial.println();
}

void processCommand(String command) {
    command.toLowerCase();
    
    if (command.startsWith("calibrate")) {
        int index = extractIndex(command, 0);
        if (isValidJoystick(index)) {
            startInteractiveCalibration(index);
        }
    }
    else if (command.startsWith("auto")) {
        int index = extractIndex(command, 0);
        if (isValidJoystick(index)) {
            startAutoCalibration(index);
        }
    }
    else if (command.startsWith("rotate")) {
        int index = extractIndex(command, 0);
        int degrees = extractDegrees(command);
        if (isValidJoystick(index) && isValidRotation(degrees)) {
            setJoystickRotation(index, degrees);
        }
    }
    else if (command.startsWith("invertx")) {
        int index = extractIndex(command, 0);
        if (isValidJoystick(index)) {
            toggleXInversion(index);
        }
    }
    else if (command.startsWith("inverty")) {
        int index = extractIndex(command, 0);
        if (isValidJoystick(index)) {
            toggleYInversion(index);
        }
    }
    else if (command.startsWith("reset")) {
        int index = extractIndex(command, 0);
        if (isValidJoystick(index)) {
            resetJoystick(index);
        }
    }
    else if (command == "status") {
        showDetailedStatus();
    }
    else if (command.startsWith("debug")) {
        int index = extractIndex(command, 0);
        if (isValidJoystick(index)) {
            joystickMgr->printDebugInfo(index);
        }
    }
    else if (command == "menu") {
        showMenu();
    }
    else {
        Serial.println("❌ Unknown command. Type 'menu' for help.");
    }
}

void startInteractiveCalibration(int index) {
    Serial.printf("\\n🎯 Starting interactive calibration for Joystick %d\\n", index);
    Serial.println("Follow the instructions carefully...");
    
    calibrationMode = true;
    calibrationStep = 0;
    calibrationStartTime = millis();
    
    joystickMgr->startCalibration(index);
    
    Serial.println("\\nStep 1: Release the joystick to center position");
    Serial.println("Press Enter when joystick is centered...");
}

void startAutoCalibration(int index) {
    Serial.printf("\\n🔄 Starting auto-calibration for Joystick %d\\n", index);
    Serial.println("Move joystick in all directions for 5 seconds...");
    Serial.println("Make sure to reach all corners and edges!");
    
    joystickMgr->autoCalibrate(index, 5000); // 5 second calibration
    
    // Show countdown
    for (int i = 5; i > 0; i--) {
        Serial.printf("⏰ %d seconds remaining...\\n", i);
        unsigned long start = millis();
        while (millis() - start < 1000) {
            joystickMgr->update();
            delay(10);
        }
    }
    
    Serial.println("✅ Auto-calibration completed!");
    showCalibrationResults(index);
}

void handleCalibration() {
    static int activeJoystick = 0; // Track which joystick we're calibrating
    
    switch (calibrationStep) {
        case 0: // Wait for center position confirmation
            // User should press Enter when joystick is centered
            break;
            
        case 1: // Calibrate center
            joystickMgr->calibrateCenter(activeJoystick);
            Serial.println("✅ Center position calibrated!");
            Serial.println("\\nStep 2: Move joystick to all extreme positions");
            Serial.println("(up, down, left, right, and corners)");
            Serial.println("This will take 3 seconds...");
            calibrationStartTime = millis();
            calibrationStep++;
            break;
            
        case 2: // Collect extreme values
            if (millis() - calibrationStartTime < 3000) {
                // Show progress
                int remaining = 3 - ((millis() - calibrationStartTime) / 1000);
                static int lastSecond = -1;
                if (remaining != lastSecond) {
                    Serial.printf("⏰ %d seconds remaining...\\n", remaining + 1);
                    lastSecond = remaining;
                }
            } else {
                joystickMgr->finishCalibration(activeJoystick);
                Serial.println("✅ Calibration completed!");
                showCalibrationResults(activeJoystick);
                calibrationMode = false;
                calibrationStep = 0;
            }
            break;
    }
    
    // Handle Enter key press for step advancement
    if (Serial.available() && calibrationStep == 0) {
        String input = Serial.readStringUntil('\\n');
        calibrationStep = 1;
    }
}

void setJoystickRotation(int index, int degrees) {
    joystickMgr->setRotation(index, degrees);
    Serial.printf("✅ Joystick %d rotation set to %d degrees\\n", index, degrees);
    
    // Show effect of rotation
    Serial.println("\\n📊 Rotation Effect Demo:");
    Serial.println("Move joystick to see rotated values...");
    
    for (int i = 0; i < 10; i++) {
        joystickMgr->update();
        
        int originalX = joystickMgr->getNormalizedX(index);
        int originalY = joystickMgr->getNormalizedY(index);
        int rotatedX = joystickMgr->getRotatedX(index);
        int rotatedY = joystickMgr->getRotatedY(index);
        
        Serial.printf("Original: (%3d, %3d) → Rotated: (%3d, %3d)\\n", 
                     originalX, originalY, rotatedX, rotatedY);
        
        delay(200);
    }
}

void toggleXInversion(int index) {
    bool currentInvert = joystickMgr->isXInverted(index);
    joystickMgr->setInvertX(index, !currentInvert);
    
    Serial.printf("✅ Joystick %d X-axis inversion: %s\\n", 
                  index, !currentInvert ? "ENABLED" : "DISABLED");
}

void toggleYInversion(int index) {
    bool currentInvert = joystickMgr->isYInverted(index);
    joystickMgr->setInvertY(index, !currentInvert);
    
    Serial.printf("✅ Joystick %d Y-axis inversion: %s\\n", 
                  index, !currentInvert ? "ENABLED" : "DISABLED");
}

void resetJoystick(int index) {
    joystickMgr->resetCalibration(index);
    joystickMgr->resetOrientation(index);
    
    Serial.printf("✅ Joystick %d reset to default settings\\n", index);
}

void showJoystickStatus() {
    static int displayCounter = 0;
    
    // Show header every 10 updates
    if (displayCounter % 10 == 0) {
        Serial.println("\\n📊 Live Joystick Status:");
        Serial.println("J# | Raw(X,Y) | Norm(X,Y) | Rot(X,Y) | Dir       | Cal | Rot°| Inv");
        Serial.println("---|----------|-----------|----------|-----------|-----|-----|----");
    }
    
    for (int i = 0; i < joystickMgr->getJoystickCount(); i++) {
        JoystickData data = joystickMgr->getJoystickData(i);
        
        String dirName = getDirectionName(data.direction);
        String calStatus = data.calibrated ? "YES" : "NO ";
        String invStatus = String(joystickMgr->isXInverted(i) ? "X" : "-") + 
                          String(joystickMgr->isYInverted(i) ? "Y" : "-");
        
        Serial.printf("%d  |(%4d,%4d)|(%4d,%4d) |(%4d,%4d) | %-9s | %s | %3d | %s\\n",
                     i, data.rawX, data.rawY, data.normalizedX, data.normalizedY,
                     data.rotatedX, data.rotatedY, dirName.c_str(), 
                     calStatus.c_str(), data.rotation, invStatus.c_str());
    }
    
    displayCounter++;
}

void showDetailedStatus() {
    Serial.println("\\n📋 Detailed Joystick Status:");
    Serial.println("==============================");
    
    for (int i = 0; i < joystickMgr->getJoystickCount(); i++) {
        JoystickData data = joystickMgr->getJoystickData(i);
        JoystickPin pins = joystickMgr->getJoystickPins(i);
        
        Serial.printf("\\n🎮 Joystick %d:\\n", i);
        Serial.printf("   Pins: VRX=%d, VRY=%d, SW=%d\\n", pins.vrx, pins.vry, pins.sw);
        Serial.printf("   Raw values: X=%d, Y=%d\\n", data.rawX, data.rawY);
        Serial.printf("   Normalized: X=%d, Y=%d\\n", data.normalizedX, data.normalizedY);
        Serial.printf("   Rotated: X=%d, Y=%d\\n", data.rotatedX, data.rotatedY);
        Serial.printf("   Direction: %s\\n", getDirectionName(data.direction).c_str());
        Serial.printf("   Switch: %s\\n", data.switchPressed ? "PRESSED" : "RELEASED");
        
        if (data.calibrated) {
            Serial.printf("   Calibration: Center=(%d,%d), Min=(%d,%d), Max=(%d,%d)\\n",
                         data.centerX, data.centerY, data.minX, data.minY, data.maxX, data.maxY);
        } else {
            Serial.println("   Calibration: NOT CALIBRATED");
        }
        
        Serial.printf("   Rotation: %d degrees\\n", data.rotation);
        Serial.printf("   Inversions: X=%s, Y=%s\\n", 
                     data.invertX ? "YES" : "NO", data.invertY ? "YES" : "NO");
    }
    
    // Global settings
    Serial.printf("\\n⚙️  Global Settings:\\n");
    Serial.printf("   Deadzone: %d\\n", joystickMgr->getDeadzone());
    Serial.printf("   Direction threshold: %d\\n", joystickMgr->getDirectionThreshold());
    Serial.printf("   Debounce delay: %lu ms\\n", joystickMgr->getDebounceDelay());
}

void showCalibrationResults(int index) {
    JoystickData data = joystickMgr->getJoystickData(index);
    
    Serial.printf("\\n📊 Calibration Results for Joystick %d:\\n", index);
    Serial.println("=========================================");
    Serial.printf("Center point: (%d, %d)\\n", data.centerX, data.centerY);
    Serial.printf("X range: %d to %d\\n", data.minX, data.maxX);
    Serial.printf("Y range: %d to %d\\n", data.minY, data.maxY);
    Serial.printf("Calibrated: %s\\n", data.calibrated ? "YES" : "NO");
    
    if (data.calibrated) {
        Serial.println("✅ Joystick is now properly calibrated!");
        Serial.println("You can save these values for future use.");
    }
}

// Helper functions
int extractIndex(String command, int defaultValue) {
    int spaceIndex = command.indexOf(' ');
    if (spaceIndex == -1) return defaultValue;
    
    String indexStr = command.substring(spaceIndex + 1);
    int nextSpace = indexStr.indexOf(' ');
    if (nextSpace != -1) {
        indexStr = indexStr.substring(0, nextSpace);
    }
    
    return indexStr.toInt();
}

int extractDegrees(String command) {
    int firstSpace = command.indexOf(' ');
    if (firstSpace == -1) return 0;
    
    int secondSpace = command.indexOf(' ', firstSpace + 1);
    if (secondSpace == -1) return 0;
    
    String degreesStr = command.substring(secondSpace + 1);
    return degreesStr.toInt();
}

bool isValidJoystick(int index) {
    if (index < 0 || index >= joystickMgr->getJoystickCount()) {
        Serial.printf("❌ Invalid joystick index: %d (available: 0-%d)\\n", 
                      index, joystickMgr->getJoystickCount() - 1);
        return false;
    }
    return true;
}

bool isValidRotation(int degrees) {
    if (degrees == 0 || degrees == 90 || degrees == 180 || degrees == 270) {
        return true;
    }
    Serial.printf("❌ Invalid rotation: %d (valid: 0, 90, 180, 270)\\n", degrees);
    return false;
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
 * Advanced Calibration Tips:
 * 
 * 1. Save calibration data to EEPROM/Flash:
 *    JoystickData data = joystickMgr->getJoystickData(0);
 *    // Save data.centerX, data.centerY, data.minX, data.maxX, etc.
 * 
 * 2. Restore calibration on startup:
 *    // Load saved values and apply them
 *    joystickMgr->startCalibration(0);
 *    // Set center and ranges manually
 *    joystickMgr->finishCalibration(0);
 * 
 * 3. Dynamic rotation based on device orientation:
 *    // Use accelerometer to detect device rotation
 *    // Apply matching joystick rotation
 * 
 * 4. Game-specific calibration profiles:
 *    // Save different calibration settings for different games
 *    // Quick profile switching
 */
