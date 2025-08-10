# Joystick Quick Start Guide

Get up and running with analog joystick control in 5 minutes!

## 🚀 Quick Setup

### 1. Hardware Check
- ✅ ESP32-S3 board
- ✅ Analog joystick module (or 2 for full experience)
- ✅ Breadboard and jumper wires
- ⚡ USB connection for Serial Monitor

### 2. Copy Library
Ensure Joystick library is in your `lib/` directory:
```
your_project/
├── src/main.cpp
├── lib/
│   └── Joystick/
│       ├── src/
│       └── examples/
└── platformio.ini
```

### 3. Wire Your Joystick
Connect your joystick module:

```
Joystick Module    ESP32-S3
---------------    --------
VRX     ------>    GPIO 16 (X-axis)
VRY     ------>    GPIO 15 (Y-axis)  
SW      ------>    GPIO 7  (Button)
VCC     ------>    3.3V
GND     ------>    GND
```

### 4. Basic Test
Copy this code to test your setup:

```cpp
#include <Arduino.h>
#include "joystick_manager.h"

JoystickManager* joystick;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("Joystick Quick Test");
    Serial.println("===================");
    
    // Create joystick manager
    joystick = new JoystickManager(1);
    
    // Add joystick with default pins
    if (!joystick->addJoystick(16, 15, 7)) {  // VRX, VRY, SW
        Serial.println("❌ Failed to add joystick!");
        return;
    }
    
    // Initialize
    joystick->init();
    Serial.println("✅ Joystick initialized!");
    
    Serial.println("\\n🎮 Move joystick and press button to see data...");
    Serial.println("Values update every 200ms");
}

void loop() {
    // Update joystick
    joystick->update();
    
    // Show data every 200ms
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate >= 200) {
        
        // Get values
        int rawX = joystick->getRawX(0);      // 0-4095
        int rawY = joystick->getRawY(0);      // 0-4095
        int normX = joystick->getNormalizedX(0); // -100 to 100
        int normY = joystick->getNormalizedY(0); // -100 to 100
        bool pressed = joystick->isSwitchPressed(0);
        
        // Show direction
        String direction = "CENTER";
        if (joystick->isUp(0)) direction = "UP";
        else if (joystick->isDown(0)) direction = "DOWN";
        else if (joystick->isLeft(0)) direction = "LEFT";
        else if (joystick->isRight(0)) direction = "RIGHT";
        
        // Print data
        Serial.printf("Raw:(%4d,%4d) Norm:(%4d,%4d) Dir:%-6s Btn:%s\\n",
                     rawX, rawY, normX, normY, direction.c_str(),
                     pressed ? "PRESS" : "     ");
        
        lastUpdate = millis();
    }
    
    // Handle button press events
    if (joystick->wasSwitchPressed(0)) {
        Serial.println("🔘 Button PRESSED!");
    }
    
    if (joystick->wasSwitchReleased(0)) {
        Serial.println("🔘 Button RELEASED!");
    }
    
    delay(10);
}
```

### 5. Upload and Test
1. Upload the code to your ESP32-S3
2. Open Serial Monitor (115200 baud)
3. Move joystick and press button
4. You should see real-time data!

## 📝 Next Steps

### Try the Examples

#### For Learning Basics:
```bash
# Start with BasicJoystickOperations
cp lib/Joystick/examples/BasicJoystickOperations/BasicJoystickOperations.ino src/main.cpp
```

#### For Precision Control:
```bash
# Try CalibrationAndRotation for accuracy
cp lib/Joystick/examples/CalibrationAndRotation/CalibrationAndRotation.ino src/main.cpp
```

#### For Gaming:
```bash
# Use GamingController for dual-stick gaming
cp lib/Joystick/examples/GamingController/GamingController.ino src/main.cpp
```

#### For Professional Applications:
```bash
# Try SensorControl for precision positioning
cp lib/Joystick/examples/SensorControl/SensorControl.ino src/main.cpp
```

### Common Patterns

#### Read Joystick Direction
```cpp
if (joystick->isUp(0)) {
    Serial.println("Moving up!");
} else if (joystick->isDown(0)) {
    Serial.println("Moving down!");
}
```

#### Handle Button Events
```cpp
if (joystick->wasSwitchPressed(0)) {
    // Button just pressed
    Serial.println("Button pressed!");
}

if (joystick->wasSwitchReleased(0)) {
    // Button just released
    Serial.println("Button released!");
}
```

#### Get Smooth Movement Values
```cpp
// Get normalized values (-100 to 100)
int moveX = joystick->getNormalizedX(0);
int moveY = joystick->getNormalizedY(0);

// Convert to movement speed (0.0 to 1.0)
float speedX = moveX / 100.0;
float speedY = moveY / 100.0;
```

#### Check for Any Movement
```cpp
if (joystick->isPressed(0)) {
    // Joystick moved from center
    Serial.println("Joystick active!");
}
```

#### Multiple Joysticks
```cpp
// Add second joystick
joystick->addJoystick(8, 18, 17);  // Different pins

// Read from specific joystick
int leftX = joystick->getNormalizedX(0);   // First joystick
int rightX = joystick->getNormalizedX(1);  // Second joystick
```

## 🔧 Troubleshooting

### Problem: "No joystick response"
**Solutions**: 
- Check wiring connections (VRX→16, VRY→15, SW→7)
- Verify power (3.3V, not 5V recommended)
- Test with multimeter on VRX/VRY pins
- Try different analog pins (32-39 work best)

### Problem: "Erratic readings"
**Solutions**:
- Use stable power supply
- Add small capacitor (0.1µF) between VCC and GND
- Increase deadzone: `joystick->setDeadzone(300)`
- Check for loose connections

### Problem: "Button not working"
**Solutions**:
- Verify SW pin connection
- Check if pull-up resistor needed (10kΩ to 3.3V)
- Adjust debounce: `joystick->setDebounceDelay(50)`
- Test button with simple digitalRead()

### Problem: "Wrong directions"
**Solutions**:
- Swap VRX and VRY wires
- Use axis inversion: `joystick->setInvertX(0, true)`
- Apply rotation: `joystick->setRotation(0, 90)` (90°, 180°, 270°)
- Check pin definitions in joystick_config.h

### Problem: "Drift from center"
**Solutions**:
- Run calibration: Use CalibrationAndRotation example
- Increase deadzone around center
- Replace worn joystick module
- Use `calibrateCenter()` function

## 💡 Pro Tips

1. **Start simple**: Test with one joystick before adding more
2. **Use Serial Monitor**: Watch real-time values to debug issues
3. **Check raw values**: If normalized values are wrong, check raw (0-4095)
4. **Power matters**: Use clean 3.3V supply for stable readings
5. **Calibrate for precision**: Always calibrate for accurate applications
6. **Adjust settings**: Tune deadzone and thresholds for your specific joystick
7. **Test orientations**: Different mounting may need rotation/inversion settings

## 🎮 Quick Configurations

### Gaming Setup (Dual Joysticks)
```cpp
JoystickManager* game = new JoystickManager(2);
game->addJoystick(16, 15, 7);   // Left stick (movement)
game->addJoystick(8, 18, 17);   // Right stick (camera)
game->setDeadzone(150);         // Gaming deadzone
game->setDebounceDelay(25);     // Fast response
```

### Precision Control
```cpp
JoystickManager* precision = new JoystickManager(1);
precision->addJoystick(16, 15, 7);
precision->setDeadzone(50);      // Small deadzone
precision->setDirectionThreshold(200); // Sensitive directions
// Run calibration for best accuracy
```

### Menu Navigation
```cpp
JoystickManager* menu = new JoystickManager(1);
menu->addJoystick(16, 15, 7);
menu->setDeadzone(400);          // Large deadzone
menu->setDirectionThreshold(1000); // Prevent accidental moves
menu->setDebounceDelay(100);     // Slow, deliberate presses
```

## 📚 Learn More

- Read the full [README.md](README.md) for detailed examples
- Check `joystick_config.h` for default pin definitions
- Try different examples to see various use cases
- Experiment with calibration and rotation features
- Integrate with other system libraries

---

**Ready to control the world with joysticks? Start with BasicJoystickOperations and level up!** 🎮
