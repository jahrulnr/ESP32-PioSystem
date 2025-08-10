# Button Manager Quick Start Guide

## 🚀 Quick Setup (3 minutes)

### Step 1: Wire Your Buttons
```
ESP32 GPIO ──── Button ──── GND
```

**Default Pins:**
- GPIO 4 → UP Button → GND
- GPIO 5 → DOWN Button → GND  
- GPIO 9 → SELECT Button → GND
- GPIO 6 → BACK Button → GND

### Step 2: Basic Code
```cpp
#include "input_manager.h"

InputManager input;

void setup() {
    Serial.begin(115200);
    input.init();
}

void loop() {
    input.update();
    
    if (input.wasPressed(BTN_UP)) {
        Serial.println("UP pressed!");
    }
    if (input.wasPressed(BTN_SELECT)) {
        Serial.println("SELECT pressed!");
    }
    
    delay(10);
}
```

### Step 3: Upload and Test
1. Upload the code
2. Open Serial Monitor (115200 baud)
3. Press buttons to see events

## 📚 Example Projects

| Example | Complexity | Features |
|---------|------------|----------|
| **BasicButtonTest** | ⭐ | Press/Release/Hold events |
| **ButtonMenuDemo** | ⭐⭐ | Menu navigation system |
| **AdvancedButtonControl** | ⭐⭐⭐ | Combos, sequences, stats |

## 🎮 Common Button Patterns

### Navigation Menu
```cpp
if (input.wasPressed(BTN_UP)) {
    menuIndex--;
}
if (input.wasPressed(BTN_DOWN)) {
    menuIndex++;
}
if (input.wasPressed(BTN_SELECT)) {
    selectItem();
}
```

### Toggle Action
```cpp
static bool state = false;
if (input.wasPressed(BTN_SELECT)) {
    state = !state;
    Serial.println(state ? "ON" : "OFF");
}
```

### Hold for Fast Action
```cpp
if (input.isHeld(BTN_UP)) {
    value += 10; // Fast increment
} else if (input.wasPressed(BTN_UP)) {
    value++; // Single increment
}
```

## 🔧 Quick Fixes

| Problem | Solution |
|---------|----------|
| No response | Check wiring, verify pins |
| False triggers | Increase debounce delay |
| Missed presses | Call `update()` more often |
| Multiple events | Use `wasPressed()` not `isPressed()` |

## ⚙️ Configuration

### Change Button Pins
Edit `lib/Button/src/btn_config.h`:
```cpp
#define BUTTON_UP 4      // Change to your pin
#define BUTTON_DOWN 5    // Change to your pin
```

### Adjust Debounce
```cpp
#define DEBOUNCE_DELAY 50  // Milliseconds
```

## 🎯 Try These Examples

1. **Start with BasicButtonTest** - Learn the fundamentals
2. **Try ButtonMenuDemo** - Build a menu system  
3. **Explore AdvancedButtonControl** - Advanced techniques

## ✅ Success Checklist

- [ ] Buttons wired to correct GPIO pins
- [ ] Serial monitor shows button events
- [ ] UP/DOWN navigation works
- [ ] SELECT chooses items
- [ ] BACK returns/exits

**Ready to build amazing button interfaces!** 🎮
