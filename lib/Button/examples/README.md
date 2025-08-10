# Button Manager Examples

This directory contains example sketches demonstrating how to use the InputManager library for ESP32 button handling.

## Examples Overview

### 1. BasicButtonTest
**File:** `BasicButtonTest/BasicButtonTest.ino`

A fundamental example demonstrating core button functionality:
- Button press, release, and hold detection
- Event-driven button handling
- Button state queries and management
- Basic debouncing demonstration
- Button combination detection

**Perfect for:** Learning the basics of button input handling

### 2. ButtonMenuDemo
**File:** `ButtonMenuDemo/ButtonMenuDemo.ino`

An interactive menu system example showing practical button usage:
- Scrollable menu navigation
- Submenu support
- SELECT/BACK navigation pattern
- Menu state management
- Real-world UI interaction patterns

**Perfect for:** Building menu-driven applications

### 3. AdvancedButtonControl
**File:** `AdvancedButtonControl/AdvancedButtonControl.ino`

A sophisticated example demonstrating advanced input techniques:
- Button sequence detection (Konami code!)
- Multi-button combinations
- Statistical analysis and logging
- Custom timing and debouncing
- Advanced state machines

**Perfect for:** Games, control systems, and complex applications

## Hardware Requirements

### Basic Setup
- **ESP32 Development Board** (any variant)
- **4 Push Buttons** (momentary contact, normally open)
- **Pull-up Resistors** (optional - internal pull-ups are used)

### Button Connections

The InputManager uses internal pull-up resistors, so buttons connect GPIO pins to GND:

```
ESP32 GPIO ──── Button ──── GND
```

**Default Pin Assignments:**
- **UP Button:**     GPIO 4
- **DOWN Button:**   GPIO 5  
- **SELECT Button:** GPIO 9
- **BACK Button:**   GPIO 6

### Wiring Diagram
```
ESP32          Button
GPIO 4  ────── [UP]    ────── GND
GPIO 5  ────── [DOWN]  ────── GND
GPIO 9  ────── [SELECT]────── GND
GPIO 6  ────── [BACK]  ────── GND
```

### Optional: External Pull-ups
If you prefer external pull-ups (for better noise immunity):

```
3.3V
 │
10kΩ
 │
GPIO ──── Button ──── GND
```

## Configuration

### Pin Configuration
Edit `lib/Button/src/btn_config.h` to change pin assignments:

```cpp
#define BUTTON_UP     4    // UP button pin
#define BUTTON_DOWN   5    // DOWN button pin  
#define BUTTON_SELECT 9    // SELECT button pin
#define BUTTON_BACK   6    // BACK button pin
#define DEBOUNCE_DELAY 50  // Debounce time in ms
```

### Debounce Settings
Adjust debounce timing based on your button quality:
- **Good quality buttons:** 20-30ms
- **Standard buttons:** 50ms (default)
- **Poor quality buttons:** 100ms+

## Usage Examples

### Basic Button Handling
```cpp
#include "input_manager.h"

InputManager input;

void setup() {
    input.init();
}

void loop() {
    input.update();
    
    if (input.wasPressed(BTN_SELECT)) {
        Serial.println("SELECT pressed!");
    }
    
    if (input.isHeld(BTN_UP)) {
        Serial.println("UP held!");
    }
}
```

### Menu Navigation Pattern
```cpp
// Navigate menu items
if (input.wasPressed(BTN_UP)) {
    menuIndex = (menuIndex - 1 + maxItems) % maxItems;
}
if (input.wasPressed(BTN_DOWN)) {
    menuIndex = (menuIndex + 1) % maxItems;
}

// Select item
if (input.wasPressed(BTN_SELECT)) {
    selectMenuItem(menuIndex);
}

// Go back
if (input.wasPressed(BTN_BACK)) {
    returnToPreviousMenu();
}
```

### Button Combinations
```cpp
// Check for combination
if (input.isPressed(BTN_UP) && input.isPressed(BTN_DOWN)) {
    Serial.println("UP+DOWN combo!");
    input.clearAllButtons(); // Prevent repeat
}
```

## Button Events

### Event Types
- **BTN_PRESSED:** Button just pressed
- **BTN_RELEASED:** Button just released  
- **BTN_HELD:** Button held for threshold time
- **BTN_NONE:** No event

### Query Methods
- `isPressed(btn)` - Currently pressed?
- `wasPressed(btn)` - Just pressed?
- `wasReleased(btn)` - Just released?
- `isHeld(btn)` - Currently held?
- `getButtonEvent(btn)` - Get current event

### State Management
- `clearButton(btn)` - Clear specific button state
- `clearAllButtons()` - Clear all button states
- `anyButtonPressed()` - Any button currently pressed?

## Advanced Features

### Custom Hold Timing
```cpp
input.setHoldThreshold(2000); // 2 second hold threshold
```

### Button Sequences
Track button sequences for special codes:
```cpp
// Example: Konami Code detection
const ButtonIndex konami[] = {BTN_UP, BTN_UP, BTN_DOWN, BTN_DOWN, BTN_SELECT, BTN_BACK};
```

### Statistical Analysis
Monitor button usage patterns:
```cpp
unsigned long pressCount = 0;
unsigned long holdCount = 0;
unsigned long longestHold = 0;
```

## Common Patterns

### Rapid Repeat
```cpp
if (input.isHeld(BTN_UP)) {
    // Execute rapid action
    incrementValue();
    delay(100); // Control repeat rate
}
```

### Toggle Behavior
```cpp
static bool state = false;
if (input.wasPressed(BTN_SELECT)) {
    state = !state;
    updateDisplay(state);
}
```

### Context-Sensitive Actions
```cpp
if (input.wasPressed(BTN_BACK)) {
    if (inSubMenu) {
        returnToMainMenu();
    } else {
        exitApplication();
    }
}
```

## Troubleshooting

### Buttons Not Responding
- Check wiring connections
- Verify GPIO pin assignments in `btn_config.h`
- Test with multimeter (should read 3.3V normally, 0V when pressed)
- Try different debounce delay

### False Triggers
- Increase debounce delay
- Check for electrical noise
- Ensure good connections
- Consider external pull-up resistors

### Missed Button Presses
- Decrease debounce delay
- Ensure `update()` is called frequently
- Check for blocking code in main loop

### Multiple Triggers
- Use `wasPressed()` instead of `isPressed()` for single events
- Call `clearButton()` after handling events
- Verify button wiring (should be normally open)

## Integration Tips

### With Display Systems
```cpp
void updateDisplay() {
    input.update();
    handleInput();
    drawScreen();
}
```

### With Real-Time Systems
```cpp
// Use in timer interrupt for precise timing
void IRAM_ATTR timerInterrupt() {
    input.update();
}
```

### Power Management
```cpp
// Wake from sleep on any button
if (input.anyButtonPressed()) {
    wakeFromSleep();
}
```

## Performance Notes

- **Update Frequency:** Call `update()` at least every 50ms
- **Memory Usage:** Minimal (< 100 bytes)
- **CPU Usage:** Very low (< 1% at 240MHz)
- **Interrupt Safe:** No (use polling only)

## Best Practices

### ✅ Do:
- Call `update()` regularly (every 10-50ms)
- Use `wasPressed()` for single-shot events
- Clear button states after handling events
- Test with different button types
- Consider user accessibility

### ❌ Don't:
- Call `update()` from interrupts
- Use blocking delays in input handling
- Forget to debounce buttons
- Make assumptions about button quality
- Skip input validation

## Example Projects

### Simple Game Controller
Implement directional movement and action buttons for games.

### Menu System
Create hierarchical menu navigation for device settings.

### Industrial Control Panel
Implement safety interlocks and operator controls.

### Home Automation Interface
Build a local control interface for smart home devices.

## Contributing

Contributions welcome! Ideas for new examples:
- Accessibility features (long press alternatives)
- Wireless button integration
- Touch button support
- Capacitive button handling
- Multiple button matrix support

## License

These examples are provided under the same license as the InputManager library.
