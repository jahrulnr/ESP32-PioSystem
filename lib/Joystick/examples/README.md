# Joystick Library Examples

This directory contains comprehensive examples demonstrating the capabilities of the JoystickManager library for ESP32-S3 systems with analog joystick modules.

## Overview

The JoystickManager library provides advanced analog joystick input handling with features like calibration, rotation, deadzone management, and multi-joystick support. These examples showcase various applications from basic input reading to complex gaming and precision control systems.

## Examples Included

### 1. BasicJoystickOperations
**Purpose**: Introduction to fundamental joystick input and processing
- ✅ Single and dual joystick setup
- ✅ Raw and normalized value reading
- ✅ Direction detection and switch handling
- ✅ Real-time input monitoring
- ✅ Deadzone and threshold configuration
- ✅ Event handling for button presses

**Best for**: Learning joystick basics, simple input applications

### 2. CalibrationAndRotation
**Purpose**: Advanced joystick calibration and orientation control
- ✅ Interactive manual calibration process
- ✅ Automatic calibration with movement detection
- ✅ 90°, 180°, 270° rotation support
- ✅ Axis inversion for different mounting orientations
- ✅ Real-time calibration visualization
- ✅ Calibration data persistence simulation
- ✅ Serial command interface for configuration

**Best for**: Custom hardware setups, precision applications, different joystick orientations

### 3. GamingController
**Purpose**: Complete dual-stick gaming controller implementation
- ✅ Dual joystick gaming setup (movement + camera/aim)
- ✅ Button combination detection
- ✅ Gesture and pattern recognition
- ✅ Configurable sensitivity and response curves
- ✅ Game state management (menu, playing, paused, settings)
- ✅ Player input statistics and analysis
- ✅ Customizable control schemes

**Best for**: Gaming applications, remote control, interactive entertainment

### 4. SensorControl
**Purpose**: Precision sensor positioning and control applications
- ✅ Multi-mode control (position, velocity, fine, scan, calibration)
- ✅ Variable speed control with acceleration/deceleration
- ✅ 3D coordinate system management
- ✅ Automated scanning and measurement routines
- ✅ Real-time sensor feedback integration
- ✅ Emergency stop and safety controls
- ✅ Data logging and sensor calibration

**Best for**: Scientific instruments, robotics, camera control, precision positioning

## Hardware Requirements

### Minimum Requirements
- ESP32-S3 microcontroller
- 1 analog joystick module

### Recommended Setup
- ESP32-S3 microcontroller
- 2 analog joystick modules (for full dual-stick functionality)
- Breadboard or PCB for connections
- Pull-up resistors (if required by joystick modules)

### Optional Components
- LED strips for visual feedback
- Display module for status information
- Servos or actuators for physical control demonstrations

## Wiring for Joystick Modules

### Standard Joystick Module Connections

```
Joystick 1:              Joystick 2 (optional):
-----------              -------------------
VRX → GPIO 16            VRX → GPIO 8
VRY → GPIO 15            VRY → GPIO 18  
SW  → GPIO 7             SW  → GPIO 17
VCC → 3.3V               VCC → 3.3V
GND → GND                GND → GND
```

### Pin Configuration Options

The default pins are defined in `joystick_config.h` but can be customized:

```cpp
// Custom pin configuration
JoystickPin customPins = {
    .vrx = 34,  // X-axis analog pin
    .vry = 35,  // Y-axis analog pin
    .sw = 32    // Switch/button pin
};
joystickMgr->addJoystick(customPins);
```

### Hardware Considerations

- **Analog Pins**: Use ADC1 pins (GPIO 32-39) for best performance
- **Digital Pins**: Switch pins can use any digital GPIO
- **Power**: Most joystick modules work with 3.3V or 5V
- **Pull-ups**: Switch pins typically need pull-up resistors (often built into modules)

## Getting Started

### 1. Installation
Copy the Joystick library to your `lib/` directory:
```
lib/
├── Joystick/
│   ├── src/
│   │   ├── joystick_manager.h
│   │   ├── joystick_manager.cpp
│   │   └── joystick_config.h
│   └── examples/          ← This directory
```

### 2. Basic Usage
```cpp
#include "joystick_manager.h"

// Create manager for up to 2 joysticks
JoystickManager* joystickMgr = new JoystickManager(2);

void setup() {
    Serial.begin(115200);
    
    // Add joysticks using default pins
    joystickMgr->addJoystick(JOYSTICK1_VRX_PIN, JOYSTICK1_VRY_PIN, JOYSTICK1_SW_PIN);
    joystickMgr->addJoystick(JOYSTICK2_VRX_PIN, JOYSTICK2_VRY_PIN, JOYSTICK2_SW_PIN);
    
    // Initialize
    joystickMgr->init();
}

void loop() {
    // Update joystick states
    joystickMgr->update();
    
    // Read values
    int x = joystickMgr->getNormalizedX(0);  // -100 to 100
    int y = joystickMgr->getNormalizedY(0);  // -100 to 100
    bool pressed = joystickMgr->isSwitchPressed(0);
    
    // Check directions
    if (joystickMgr->isUp(0)) {
        Serial.println("Moving up!");
    }
    
    delay(10);
}
```

### 3. Running Examples

#### Option A: Direct Upload
1. Copy example code to your main project
2. Ensure Joystick library is in your `lib/` directory  
3. Upload to your ESP32-S3

#### Option B: PlatformIO Example Environments
```ini
# platformio.ini
[env:basic_joystick]
board = esp32-s3-devkitc-1
framework = arduino
lib_deps = 
    file://lib/Joystick

[env:gaming_controller]
board = esp32-s3-devkitc-1
framework = arduino
lib_deps = 
    file://lib/Joystick
```

## Example Configurations

### For BasicJoystickOperations
```cpp
JoystickManager* joystickMgr = new JoystickManager(2);
// Standard setup with default pins and settings
joystickMgr->addJoystick(16, 15, 7);  // Joystick 1
joystickMgr->addJoystick(8, 18, 17);  // Joystick 2 (optional)
```

### For CalibrationAndRotation
```cpp
JoystickManager* joystickMgr = new JoystickManager(2);
// Setup with calibration support
joystickMgr->setDeadzone(50);      // Small deadzone for calibration
joystickMgr->setDirectionThreshold(200);
```

### For GamingController
```cpp
JoystickManager* gameController = new JoystickManager(2);
// Gaming-optimized settings
gameController->setDeadzone(150);          // Balanced deadzone
gameController->setDirectionThreshold(800); // Higher threshold
gameController->setDebounceDelay(25);      // Fast response
```

### For SensorControl
```cpp
JoystickManager* sensorController = new JoystickManager(2);
// Precision control settings
sensorController->setDeadzone(50);           // Small deadzone
sensorController->setDirectionThreshold(200); // Lower threshold
sensorController->setDebounceDelay(30);      // Quick response
```

## API Reference

### Core Functions
```cpp
// Initialization
JoystickManager(int maxJoysticks = 4)
bool addJoystick(int vrxPin, int vryPin, int swPin)
bool addJoystick(JoystickPin pinConfig)
void init()
void update()

// Data Access
int getRawX(int index)                    // 0-4095 raw ADC
int getRawY(int index)                    // 0-4095 raw ADC
int getNormalizedX(int index)             // -100 to 100
int getNormalizedY(int index)             // -100 to 100
int getRotatedX(int index)                // After rotation applied
int getRotatedY(int index)                // After rotation applied
int getDirection(int index)               // JOYSTICK_UP, DOWN, etc.

// Switch/Button Functions
bool isSwitchPressed(int index)           // Current state
bool wasSwitchPressed(int index)          // Just pressed this frame
bool wasSwitchReleased(int index)         // Just released this frame

// Direction Helpers
bool isUp(int index)
bool isDown(int index)
bool isLeft(int index)
bool isRight(int index)
bool isCenter(int index)
bool isDiagonal(int index)
bool isPressed(int index)                 // Any direction active

// Calibration
void startCalibration(int index)
void calibrateCenter(int index)
void finishCalibration(int index)
void autoCalibrate(int index, unsigned long duration = 3000)
bool isCalibrated(int index)
void resetCalibration(int index)

// Rotation and Orientation
void setRotation(int index, int degrees)  // 0, 90, 180, 270
int getRotation(int index)
void setInvertX(int index, bool invert)
void setInvertY(int index, bool invert)
void resetOrientation(int index)

// Configuration
void setDeadzone(int threshold)           // Center deadzone
void setDirectionThreshold(int threshold) // Direction detection sensitivity
void setDebounceDelay(unsigned long delay) // Button debounce time
```

### Data Structures
```cpp
struct JoystickPin {
    int vrx;  // X-axis analog pin
    int vry;  // Y-axis analog pin
    int sw;   // Switch/button pin
};

struct JoystickData {
    int rawX, rawY;              // Raw ADC values (0-4095)
    int normalizedX, normalizedY; // Normalized values (-100 to 100)
    int rotatedX, rotatedY;       // Values after rotation
    int direction;                // Current direction constant
    bool switchPressed;           // Current switch state
    bool switchChanged;           // Switch state changed this frame
    
    // Calibration data
    int centerX, centerY;         // Calibrated center point
    int minX, maxX, minY, maxY;   // Calibrated ranges
    bool calibrated;              // Calibration status
    
    // Orientation
    int rotation;                 // Rotation in degrees
    bool invertX, invertY;        // Axis inversions
};
```

## Configuration Options

### Deadzone Settings
```cpp
// Small deadzone for precision (0-100)
joystickMgr->setDeadzone(50);

// Medium deadzone for general use (100-300)
joystickMgr->setDeadzone(200);

// Large deadzone for noisy joysticks (300-500)
joystickMgr->setDeadzone(400);
```

### Direction Sensitivity
```cpp
// High sensitivity - easier to trigger directions
joystickMgr->setDirectionThreshold(500);

// Medium sensitivity - balanced
joystickMgr->setDirectionThreshold(800);

// Low sensitivity - requires more movement
joystickMgr->setDirectionThreshold(1200);
```

### Button Debouncing
```cpp
// Fast response for gaming (10-30ms)
joystickMgr->setDebounceDelay(25);

// Standard debouncing (30-50ms)
joystickMgr->setDebounceDelay(50);

// Slow/noisy switches (50-100ms)
joystickMgr->setDebounceDelay(75);
```

## Performance Considerations

### Update Frequency
- **Gaming**: 60-100 Hz (16-10ms intervals)
- **UI Control**: 20-30 Hz (50-33ms intervals)
- **Sensor Control**: 100+ Hz (10ms or less)

### Memory Usage
- Base JoystickManager: ~200 bytes
- Per joystick: ~80 bytes
- 4 joysticks total: ~520 bytes

### CPU Usage
- Basic update: ~0.1ms per joystick
- With calibration: ~0.2ms per joystick
- Negligible impact at typical update rates

## Troubleshooting

### Common Issues

#### "Erratic joystick readings"
- Check power supply stability (use 3.3V, not 5V if possible)
- Verify analog pin connections
- Increase deadzone setting
- Check for electromagnetic interference

#### "Switch not responding"
- Verify pull-up resistor (10kΩ to 3.3V)
- Check pin configuration (INPUT_PULLUP)
- Adjust debounce delay
- Test switch with multimeter

#### "Joystick drift/offset"
- Perform calibration using CalibrationAndRotation example
- Check for mechanical wear in joystick
- Increase deadzone around center
- Use `calibrateCenter()` to reset center point

#### "Wrong directions detected"
- Check wiring (VRX/VRY may be swapped)
- Use rotation settings to correct orientation
- Verify direction threshold settings
- Check for analog pin conflicts

### Debug Tips

1. **Check raw values**:
```cpp
Serial.printf("Raw: X=%d, Y=%d\\n", joystickMgr->getRawX(0), joystickMgr->getRawY(0));
```

2. **Monitor calibration status**:
```cpp
JoystickData data = joystickMgr->getJoystickData(0);
Serial.printf("Calibrated: %s, Center: (%d,%d)\\n", 
              data.calibrated ? "YES" : "NO", data.centerX, data.centerY);
```

3. **Use debug output**:
```cpp
joystickMgr->printDebugInfo(0);      // Single joystick
joystickMgr->printAllDebugInfo();    // All joysticks
```

## Advanced Features

### Custom Response Curves
```cpp
// Exponential response for fine control at center
float customResponse(int input) {
    float normalized = input / 100.0;  // Convert to -1.0 to 1.0
    return normalized * abs(normalized); // Quadratic curve
}
```

### Multi-Joystick Coordination
```cpp
// Average two joysticks for stability
int avgX = (joystickMgr->getNormalizedX(0) + joystickMgr->getNormalizedX(1)) / 2;
int avgY = (joystickMgr->getNormalizedY(0) + joystickMgr->getNormalizedY(1)) / 2;
```

### Gesture Recognition
```cpp
// Detect circular motions
void detectCircularGesture(int index) {
    static float lastAngle = 0;
    float currentAngle = atan2(joystickMgr->getNormalizedY(index), 
                              joystickMgr->getNormalizedX(index));
    
    float angleDiff = currentAngle - lastAngle;
    // Detect continuous rotation...
}
```

### State Machine Integration
```cpp
enum ControlState { IDLE, MOVING, SELECTING, CONFIGURING };

void updateControlState() {
    switch (currentState) {
        case IDLE:
            if (joystickMgr->isPressed(0)) currentState = MOVING;
            break;
        case MOVING:
            if (joystickMgr->wasSwitchPressed(0)) currentState = SELECTING;
            break;
        // ... more states
    }
}
```

## Integration Examples

### With Display Manager
```cpp
#include "display_manager.h"
#include "joystick_manager.h"

// Show joystick position on display
int x = joystickMgr->getNormalizedX(0);
int y = joystickMgr->getNormalizedY(0);
displayMgr->drawJoystickPosition(x, y);
```

### With Battery Manager
```cpp
#include "battery_manager.h"
#include "joystick_manager.h"

// Reduce update frequency on low battery
if (batteryMgr->getBatteryPercentage() < 20) {
    // Update every 50ms instead of 10ms
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 50) {
        joystickMgr->update();
        lastUpdate = millis();
    }
}
```

### With IoT Manager
```cpp
#include "iot_manager.h"
#include "joystick_manager.h"

// Send joystick data over network
if (joystickMgr->isPressed(0)) {
    int x = joystickMgr->getNormalizedX(0);
    int y = joystickMgr->getNormalizedY(0);
    iotMgr->sendJoystickData(x, y);
}
```

## Contributing

To add new examples or improve existing ones:

1. Follow the existing code structure and documentation style
2. Include comprehensive error handling and user feedback
3. Add detailed comments explaining joystick-specific concepts
4. Test with different joystick modules and orientations
5. Ensure examples work with single joystick configurations
6. Include performance considerations and optimization tips

## License

This examples collection is provided under the same license as the main project.

---

**Need Help?** 
- Start with BasicJoystickOperations to verify your hardware setup
- Use CalibrationAndRotation for precision applications
- Check the joystick_config.h file for pin definitions
- Test with known-good joystick modules first
- Monitor Serial output for debugging information
