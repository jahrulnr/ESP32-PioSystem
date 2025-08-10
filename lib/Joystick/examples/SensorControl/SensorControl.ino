/*
 * Joystick Sensor Control Example
 * 
 * This example demonstrates using joysticks for precision sensor control,
 * data acquisition, and scientific/industrial applications. Perfect for
 * controlling cameras, sensors, robotics, and measurement equipment.
 * 
 * Features:
 * - Precision sensor positioning control
 * - Variable speed control with acceleration/deceleration
 * - Multi-axis coordinate system management
 * - Data logging and sensor calibration
 * - Real-time sensor feedback integration
 * - Emergency stop and safety controls
 * - Automated scanning and measurement routines
 * 
 * Hardware Requirements:
 * - ESP32-S3 development board
 * - 1-2 analog joystick modules
 * - Optional: Servos, stepper motors, or actuators for physical control
 * - Optional: Sensors for feedback (encoders, position sensors, etc.)
 * 
 * Control Applications:
 * - Camera gimbal control
 * - Microscope stage positioning
 * - Robotic arm control
 * - Antenna positioning
 * - Laboratory equipment control
 * 
 * Joystick Connections:
 * Primary Control:               Secondary Control (optional):
 * - VRX → GPIO 16               - VRX → GPIO 8
 * - VRY → GPIO 15               - VRY → GPIO 18
 * - SW  → GPIO 7                - SW  → GPIO 17
 * 
 * Author: Your Name
 * Date: 2025
 */

#include <Arduino.h>
#include "joystick_manager.h"

// Create JoystickManager for sensor control
JoystickManager* sensorController;

// Control definitions
#define PRIMARY_CONTROL    0  // Main positioning control
#define SECONDARY_CONTROL  1  // Secondary/fine control

// Control modes
enum ControlMode {
    POSITION_MODE,    // Direct position control
    VELOCITY_MODE,    // Velocity control with acceleration
    FINE_MODE,        // High precision mode
    SCAN_MODE,        // Automated scanning mode
    CALIBRATION_MODE  // Sensor calibration mode
};

// Coordinate system
struct Position3D {
    float x, y, z;        // Current position
    float targetX, targetY, targetZ;  // Target position
    float minX, maxX;     // X axis limits
    float minY, maxY;     // Y axis limits
    float minZ, maxZ;     // Z axis limits
    bool limitingEnabled; // Enable position limiting
};

// Control parameters
struct ControlParams {
    float sensitivity;    // Control sensitivity (0.1 - 5.0)
    float acceleration;   // Acceleration factor
    float maxSpeed;       // Maximum movement speed
    float deadband;       // Deadband for stability
    bool smoothing;       // Enable movement smoothing
    int smoothingFactor;  // Smoothing strength (1-10)
};

// Sensor data
struct SensorData {
    float value;          // Current sensor reading
    float minValue;       // Minimum recorded value
    float maxValue;       // Maximum recorded value
    float calibrationOffset; // Calibration offset
    float calibrationScale;  // Calibration scale factor
    bool isCalibrated;    // Calibration status
    unsigned long lastReading; // Last reading timestamp
};

// System state
ControlMode currentMode = POSITION_MODE;
Position3D currentPosition;
ControlParams controlParams;
SensorData sensorData;

// Movement tracking
struct MovementState {
    float velocityX, velocityY, velocityZ;
    float smoothX, smoothY, smoothZ;
    bool isMoving;
    unsigned long lastMoveTime;
    unsigned long totalMoveTime;
};

MovementState movement;

// Scanning parameters
struct ScanParams {
    float startX, startY, endX, endY;
    float stepSize;
    int scanSpeed;
    bool scanActive;
    int currentStep;
    int totalSteps;
    unsigned long scanStartTime;
};

ScanParams scanParams;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Joystick Sensor Control Demo ===");
    Serial.println();
    
    // Initialize sensor controller
    sensorController = new JoystickManager(2);
    
    // Setup control joysticks
    setupSensorControl();
    
    // Initialize system parameters
    initializeControlSystem();
    
    sensorController->init();
    
    Serial.println("✅ Sensor Control System initialized!");
    
    showControlModes();
    showCurrentStatus();
}

void loop() {
    // Update joystick input
    sensorController->update();
    
    // Process control based on current mode
    switch (currentMode) {
        case POSITION_MODE:
            handlePositionControl();
            break;
        case VELOCITY_MODE:
            handleVelocityControl();
            break;
        case FINE_MODE:
            handleFineControl();
            break;
        case SCAN_MODE:
            handleScanControl();
            break;
        case CALIBRATION_MODE:
            handleCalibrationControl();
            break;
    }
    
    // Update system state
    updatePosition();
    updateSensorData();
    updateMovementTracking();
    
    // Handle user commands
    if (Serial.available()) {
        handleSerialCommands();
    }
    
    // Safety checks
    performSafetyChecks();
    
    // Display updates
    static unsigned long lastDisplay = 0;
    if (millis() - lastDisplay > 500) {
        displaySystemStatus();
        lastDisplay = millis();
    }
    
    delay(10); // 100Hz control loop
}

void setupSensorControl() {
    Serial.println("🔧 Setting up sensor control system...");
    
    // Add primary control joystick
    if (sensorController->addJoystick(JOYSTICK1_VRX_PIN, JOYSTICK1_VRY_PIN, JOYSTICK1_SW_PIN)) {
        Serial.printf("✅ Primary control - VRX:%d, VRY:%d, SW:%d\\n", 
                      JOYSTICK1_VRX_PIN, JOYSTICK1_VRY_PIN, JOYSTICK1_SW_PIN);
    }
    
    // Add secondary control joystick (optional)
    if (sensorController->addJoystick(JOYSTICK2_VRX_PIN, JOYSTICK2_VRY_PIN, JOYSTICK2_SW_PIN)) {
        Serial.printf("✅ Secondary control - VRX:%d, VRY:%d, SW:%d\\n", 
                      JOYSTICK2_VRX_PIN, JOYSTICK2_VRY_PIN, JOYSTICK2_SW_PIN);
    }
    
    // Optimize for precision control
    sensorController->setDeadzone(50);           // Small deadzone for precision
    sensorController->setDirectionThreshold(200); // Lower threshold for smooth control
    sensorController->setDebounceDelay(30);      // Quick response
    
    Serial.printf("Control joysticks: %d\\n", sensorController->getJoystickCount());
}

void initializeControlSystem() {
    // Initialize position limits
    currentPosition.x = 0.0; currentPosition.y = 0.0; currentPosition.z = 0.0;
    currentPosition.targetX = 0.0; currentPosition.targetY = 0.0; currentPosition.targetZ = 0.0;
    currentPosition.minX = -100.0; currentPosition.maxX = 100.0;
    currentPosition.minY = -100.0; currentPosition.maxY = 100.0;
    currentPosition.minZ = -50.0;  currentPosition.maxZ = 50.0;
    currentPosition.limitingEnabled = true;
    
    // Initialize control parameters
    controlParams.sensitivity = 1.0;
    controlParams.acceleration = 0.1;
    controlParams.maxSpeed = 10.0;
    controlParams.deadband = 0.05;
    controlParams.smoothing = true;
    controlParams.smoothingFactor = 5;
    
    // Initialize sensor data
    sensorData.value = 0.0;
    sensorData.minValue = 1000.0;
    sensorData.maxValue = -1000.0;
    sensorData.calibrationOffset = 0.0;
    sensorData.calibrationScale = 1.0;
    sensorData.isCalibrated = false;
    
    // Initialize movement state
    movement.velocityX = 0.0; movement.velocityY = 0.0; movement.velocityZ = 0.0;
    movement.smoothX = 0.0; movement.smoothY = 0.0; movement.smoothZ = 0.0;
    movement.isMoving = false;
    movement.totalMoveTime = 0;
    
    // Initialize scan parameters
    scanParams.startX = -50.0; scanParams.startY = -50.0;
    scanParams.endX = 50.0; scanParams.endY = 50.0;
    scanParams.stepSize = 5.0;
    scanParams.scanSpeed = 2;
    scanParams.scanActive = false;
    scanParams.currentStep = 0;
}

void handlePositionControl() {
    // Direct position control mode
    if (sensorController->getJoystickCount() > 0) {
        // Primary joystick controls X and Y
        float inputX = sensorController->getNormalizedX(PRIMARY_CONTROL) / 100.0;
        float inputY = sensorController->getNormalizedY(PRIMARY_CONTROL) / 100.0;
        
        // Apply sensitivity and deadband
        if (abs(inputX) > controlParams.deadband) {
            currentPosition.targetX += inputX * controlParams.sensitivity * 0.1;
        }
        if (abs(inputY) > controlParams.deadband) {
            currentPosition.targetY += inputY * controlParams.sensitivity * 0.1;
        }
        
        // Secondary joystick controls Z and fine adjustments
        if (sensorController->getJoystickCount() > 1) {
            float inputZ = sensorController->getNormalizedY(SECONDARY_CONTROL) / 100.0;
            if (abs(inputZ) > controlParams.deadband) {
                currentPosition.targetZ += inputZ * controlParams.sensitivity * 0.05;
            }
        }
        
        // Handle button presses
        if (sensorController->wasSwitchPressed(PRIMARY_CONTROL)) {
            switchControlMode();
        }
        
        if (sensorController->wasSwitchPressed(SECONDARY_CONTROL)) {
            zeroPosition();
        }
    }
}

void handleVelocityControl() {
    // Velocity control with acceleration
    if (sensorController->getJoystickCount() > 0) {
        float inputX = sensorController->getNormalizedX(PRIMARY_CONTROL) / 100.0;
        float inputY = sensorController->getNormalizedY(PRIMARY_CONTROL) / 100.0;
        
        // Calculate target velocities
        float targetVelX = inputX * controlParams.maxSpeed;
        float targetVelY = inputY * controlParams.maxSpeed;
        
        // Apply acceleration/deceleration
        movement.velocityX += (targetVelX - movement.velocityX) * controlParams.acceleration;
        movement.velocityY += (targetVelY - movement.velocityY) * controlParams.acceleration;
        
        // Update positions based on velocity
        currentPosition.targetX += movement.velocityX * 0.01; // 10ms time step
        currentPosition.targetY += movement.velocityY * 0.01;
        
        // Z control from secondary joystick
        if (sensorController->getJoystickCount() > 1) {
            float inputZ = sensorController->getNormalizedY(SECONDARY_CONTROL) / 100.0;
            float targetVelZ = inputZ * controlParams.maxSpeed * 0.5; // Slower Z movement
            movement.velocityZ += (targetVelZ - movement.velocityZ) * controlParams.acceleration;
            currentPosition.targetZ += movement.velocityZ * 0.01;
        }
        
        // Button handling
        if (sensorController->wasSwitchPressed(PRIMARY_CONTROL)) {
            switchControlMode();
        }
    }
}

void handleFineControl() {
    // High precision control mode
    float fineSensitivity = controlParams.sensitivity * 0.1; // 10x more precise
    
    if (sensorController->getJoystickCount() > 0) {
        float inputX = sensorController->getNormalizedX(PRIMARY_CONTROL) / 100.0;
        float inputY = sensorController->getNormalizedY(PRIMARY_CONTROL) / 100.0;
        
        // Very small movements for precision
        if (abs(inputX) > controlParams.deadband * 0.5) {
            currentPosition.targetX += inputX * fineSensitivity * 0.01;
        }
        if (abs(inputY) > controlParams.deadband * 0.5) {
            currentPosition.targetY += inputY * fineSensitivity * 0.01;
        }
        
        // Fine Z control
        if (sensorController->getJoystickCount() > 1) {
            float inputZ = sensorController->getNormalizedY(SECONDARY_CONTROL) / 100.0;
            if (abs(inputZ) > controlParams.deadband * 0.5) {
                currentPosition.targetZ += inputZ * fineSensitivity * 0.005;
            }
        }
        
        if (sensorController->wasSwitchPressed(PRIMARY_CONTROL)) {
            switchControlMode();
        }
    }
}

void handleScanControl() {
    // Automated scanning mode
    if (!scanParams.scanActive) {
        // Start scan with button press
        if (sensorController->wasSwitchPressed(PRIMARY_CONTROL)) {
            startAutomatedScan();
        }
        
        // Manual position adjustment for scan setup
        if (sensorController->getJoystickCount() > 0) {
            float inputX = sensorController->getNormalizedX(PRIMARY_CONTROL) / 100.0;
            float inputY = sensorController->getNormalizedY(PRIMARY_CONTROL) / 100.0;
            
            if (abs(inputX) > 0.5) {
                scanParams.startX += inputX * 2.0;
                scanParams.endX += inputX * 2.0;
            }
            if (abs(inputY) > 0.5) {
                scanParams.startY += inputY * 2.0;
                scanParams.endY += inputY * 2.0;
            }
        }
    } else {
        // Execute scan
        executeAutomatedScan();
        
        // Stop scan with button press
        if (sensorController->wasSwitchPressed(PRIMARY_CONTROL)) {
            stopAutomatedScan();
        }
    }
    
    // Mode switch
    if (sensorController->wasSwitchPressed(SECONDARY_CONTROL)) {
        switchControlMode();
    }
}

void handleCalibrationControl() {
    // Sensor calibration mode
    static bool calibrationStarted = false;
    static float calibrationSum = 0;
    static int calibrationSamples = 0;
    
    if (!calibrationStarted) {
        if (sensorController->wasSwitchPressed(PRIMARY_CONTROL)) {
            Serial.println("🎯 Starting sensor calibration...");
            calibrationStarted = true;
            calibrationSum = 0;
            calibrationSamples = 0;
            sensorData.minValue = 1000.0;
            sensorData.maxValue = -1000.0;
        }
    } else {
        // Collect calibration data
        float currentValue = readSensorValue();
        calibrationSum += currentValue;
        calibrationSamples++;
        
        if (currentValue < sensorData.minValue) sensorData.minValue = currentValue;
        if (currentValue > sensorData.maxValue) sensorData.maxValue = currentValue;
        
        // Finish calibration
        if (sensorController->wasSwitchPressed(PRIMARY_CONTROL) || calibrationSamples >= 1000) {
            sensorData.calibrationOffset = calibrationSum / calibrationSamples;
            sensorData.calibrationScale = 1.0 / (sensorData.maxValue - sensorData.minValue);
            sensorData.isCalibrated = true;
            calibrationStarted = false;
            
            Serial.printf("✅ Calibration complete! Offset: %.3f, Scale: %.3f\\n", 
                         sensorData.calibrationOffset, sensorData.calibrationScale);
        }
        
        // Show calibration progress
        if (calibrationSamples % 100 == 0) {
            Serial.printf("📊 Calibration: %d samples, Range: %.3f to %.3f\\n",
                         calibrationSamples, sensorData.minValue, sensorData.maxValue);
        }
    }
    
    // Mode switch
    if (sensorController->wasSwitchPressed(SECONDARY_CONTROL)) {
        switchControlMode();
    }
}

void updatePosition() {
    // Apply position limiting
    if (currentPosition.limitingEnabled) {
        currentPosition.targetX = constrain(currentPosition.targetX, 
                                          currentPosition.minX, currentPosition.maxX);
        currentPosition.targetY = constrain(currentPosition.targetY, 
                                          currentPosition.minY, currentPosition.maxY);
        currentPosition.targetZ = constrain(currentPosition.targetZ, 
                                          currentPosition.minZ, currentPosition.maxZ);
    }
    
    // Apply smoothing
    if (controlParams.smoothing) {
        float smoothFactor = controlParams.smoothingFactor / 10.0;
        currentPosition.x += (currentPosition.targetX - currentPosition.x) * smoothFactor;
        currentPosition.y += (currentPosition.targetY - currentPosition.y) * smoothFactor;
        currentPosition.z += (currentPosition.targetZ - currentPosition.z) * smoothFactor;
    } else {
        currentPosition.x = currentPosition.targetX;
        currentPosition.y = currentPosition.targetY;
        currentPosition.z = currentPosition.targetZ;
    }
}

void updateSensorData() {
    // Simulate sensor reading (replace with actual sensor)
    sensorData.value = readSensorValue();
    sensorData.lastReading = millis();
    
    // Apply calibration if available
    if (sensorData.isCalibrated) {
        sensorData.value = (sensorData.value - sensorData.calibrationOffset) * sensorData.calibrationScale;
    }
}

float readSensorValue() {
    // Simulate sensor reading based on position
    // In real application, this would read from actual sensors
    float simulatedValue = sin(currentPosition.x * 0.1) * cos(currentPosition.y * 0.1) * 100.0;
    simulatedValue += random(-5, 6); // Add some noise
    return simulatedValue;
}

void updateMovementTracking() {
    float totalVelocity = sqrt(movement.velocityX * movement.velocityX + 
                              movement.velocityY * movement.velocityY + 
                              movement.velocityZ * movement.velocityZ);
    
    bool wasMoving = movement.isMoving;
    movement.isMoving = (totalVelocity > 0.1);
    
    if (movement.isMoving) {
        if (!wasMoving) {
            movement.lastMoveTime = millis();
        }
        movement.totalMoveTime += 10; // 10ms update period
    }
}

void startAutomatedScan() {
    Serial.println("🔄 Starting automated scan...");
    
    scanParams.scanActive = true;
    scanParams.currentStep = 0;
    scanParams.scanStartTime = millis();
    
    // Calculate total steps
    float rangeX = scanParams.endX - scanParams.startX;
    float rangeY = scanParams.endY - scanParams.startY;
    int stepsX = (int)(rangeX / scanParams.stepSize) + 1;
    int stepsY = (int)(rangeY / scanParams.stepSize) + 1;
    scanParams.totalSteps = stepsX * stepsY;
    
    Serial.printf("📊 Scan area: (%.1f,%.1f) to (%.1f,%.1f), Steps: %d\\n",
                  scanParams.startX, scanParams.startY, scanParams.endX, scanParams.endY,
                  scanParams.totalSteps);
    
    // Move to start position
    currentPosition.targetX = scanParams.startX;
    currentPosition.targetY = scanParams.startY;
}

void executeAutomatedScan() {
    static unsigned long lastScanStep = 0;
    
    if (millis() - lastScanStep > (1000 / scanParams.scanSpeed)) {
        // Calculate current scan position
        float rangeX = scanParams.endX - scanParams.startX;
        int stepsX = (int)(rangeX / scanParams.stepSize) + 1;
        
        int stepX = scanParams.currentStep % stepsX;
        int stepY = scanParams.currentStep / stepsX;
        
        currentPosition.targetX = scanParams.startX + stepX * scanParams.stepSize;
        currentPosition.targetY = scanParams.startY + stepY * scanParams.stepSize;
        
        // Log scan data
        Serial.printf("📍 Scan step %d/%d: (%.1f,%.1f) = %.3f\\n",
                     scanParams.currentStep + 1, scanParams.totalSteps,
                     currentPosition.targetX, currentPosition.targetY, sensorData.value);
        
        scanParams.currentStep++;
        lastScanStep = millis();
        
        // Check if scan is complete
        if (scanParams.currentStep >= scanParams.totalSteps) {
            stopAutomatedScan();
        }
    }
}

void stopAutomatedScan() {
    scanParams.scanActive = false;
    unsigned long scanTime = millis() - scanParams.scanStartTime;
    
    Serial.printf("✅ Scan completed in %.2f seconds\\n", scanTime / 1000.0);
}

void switchControlMode() {
    // Cycle through control modes
    currentMode = (ControlMode)((currentMode + 1) % 5);
    
    String modeName;
    switch (currentMode) {
        case POSITION_MODE:    modeName = "Position Control"; break;
        case VELOCITY_MODE:    modeName = "Velocity Control"; break;
        case FINE_MODE:        modeName = "Fine Control"; break;
        case SCAN_MODE:        modeName = "Scan Mode"; break;
        case CALIBRATION_MODE: modeName = "Calibration Mode"; break;
    }
    
    Serial.printf("🔄 Switched to: %s\\n", modeName.c_str());
}

void zeroPosition() {
    currentPosition.x = 0.0;
    currentPosition.y = 0.0;
    currentPosition.z = 0.0;
    currentPosition.targetX = 0.0;
    currentPosition.targetY = 0.0;
    currentPosition.targetZ = 0.0;
    
    movement.velocityX = 0.0;
    movement.velocityY = 0.0;
    movement.velocityZ = 0.0;
    
    Serial.println("🎯 Position zeroed!");
}

void performSafetyChecks() {
    // Emergency stop check (both buttons pressed)
    if (sensorController->getJoystickCount() >= 2 &&
        sensorController->isSwitchPressed(PRIMARY_CONTROL) &&
        sensorController->isSwitchPressed(SECONDARY_CONTROL)) {
        
        // Emergency stop
        movement.velocityX = 0.0;
        movement.velocityY = 0.0;
        movement.velocityZ = 0.0;
        currentPosition.targetX = currentPosition.x;
        currentPosition.targetY = currentPosition.y;
        currentPosition.targetZ = currentPosition.z;
        
        if (scanParams.scanActive) {
            stopAutomatedScan();
        }
        
        static unsigned long lastEmergencyMessage = 0;
        if (millis() - lastEmergencyMessage > 2000) {
            Serial.println("🚨 EMERGENCY STOP ACTIVATED!");
            lastEmergencyMessage = millis();
        }
    }
}

void displaySystemStatus() {
    static int displayCounter = 0;
    
    // Show header every 20 updates (10 seconds)
    if (displayCounter % 20 == 0) {
        Serial.println("\\n📊 System Status:");
        Serial.println("Mode     | Position (X,Y,Z)     | Velocity    | Sensor   | Status");
        Serial.println("---------|---------------------|-------------|----------|--------");
    }
    
    String modeName = getModeString();
    String velocityStr = String(sqrt(movement.velocityX*movement.velocityX + 
                                   movement.velocityY*movement.velocityY), 1);
    String statusStr = movement.isMoving ? "MOVING" : 
                      scanParams.scanActive ? "SCANNING" : "IDLE";
    
    Serial.printf("%-8s |(%6.1f,%6.1f,%6.1f) | %8s    | %7.2f | %s\\n",
                 modeName.c_str(), currentPosition.x, currentPosition.y, currentPosition.z,
                 velocityStr.c_str(), sensorData.value, statusStr.c_str());
    
    displayCounter++;
}

void showControlModes() {
    Serial.println("\\n🎮 Control Modes:");
    Serial.println("==================");
    Serial.println("1. Position Mode - Direct position control");
    Serial.println("2. Velocity Mode - Speed-based control with acceleration");
    Serial.println("3. Fine Mode - High precision positioning");
    Serial.println("4. Scan Mode - Automated area scanning");
    Serial.println("5. Calibration Mode - Sensor calibration");
    Serial.println("\\nPress primary joystick button to cycle modes");
    Serial.println("Press secondary button for special functions");
    Serial.println("Hold both buttons for emergency stop");
}

void showCurrentStatus() {
    Serial.println("\\n📋 Current Configuration:");
    Serial.println("=========================");
    Serial.printf("Control Mode: %s\\n", getModeString().c_str());
    Serial.printf("Sensitivity: %.1f\\n", controlParams.sensitivity);
    Serial.printf("Max Speed: %.1f\\n", controlParams.maxSpeed);
    Serial.printf("Position Limits: X(%.1f to %.1f), Y(%.1f to %.1f), Z(%.1f to %.1f)\\n",
                  currentPosition.minX, currentPosition.maxX,
                  currentPosition.minY, currentPosition.maxY,
                  currentPosition.minZ, currentPosition.maxZ);
    Serial.printf("Sensor Calibrated: %s\\n", sensorData.isCalibrated ? "YES" : "NO");
}

String getModeString() {
    switch (currentMode) {
        case POSITION_MODE:    return "POSITION";
        case VELOCITY_MODE:    return "VELOCITY";
        case FINE_MODE:        return "FINE";
        case SCAN_MODE:        return "SCAN";
        case CALIBRATION_MODE: return "CALIB";
        default:               return "UNKNOWN";
    }
}

void handleSerialCommands() {
    String command = Serial.readStringUntil('\\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "status") {
        showCurrentStatus();
    } else if (command == "zero") {
        zeroPosition();
    } else if (command == "calibrate") {
        currentMode = CALIBRATION_MODE;
        Serial.println("🎯 Entering calibration mode...");
    } else if (command == "scan") {
        currentMode = SCAN_MODE;
        Serial.println("🔄 Entering scan mode...");
    } else if (command == "help") {
        showControlModes();
        Serial.println("\\nCommands: status, zero, calibrate, scan, help");
    }
}

/*
 * Advanced Sensor Control Applications:
 * 
 * 1. Camera Gimbal Control:
 *    // Use joystick for smooth camera movement
 *    // Implement stabilization algorithms
 * 
 * 2. Microscope Stage:
 *    // High precision positioning for sample examination
 *    // Automated scanning for image stitching
 * 
 * 3. Robotic Arm Control:
 *    // Joint-space or Cartesian coordinate control
 *    // End-effector positioning with feedback
 * 
 * 4. Antenna Positioning:
 *    // Azimuth and elevation control
 *    // Signal strength optimization
 * 
 * 5. Laboratory Equipment:
 *    // Sample positioning for measurements
 *    // Automated data collection routines
 */
