#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>

// Define GPIO pins for buttons
#define BUTTON_SLEEP  BOOT_PIN
#define BUTTON_UP     4
#define BUTTON_DOWN   5
#define BUTTON_SELECT 14
#define BUTTON_BACK   6
#define DEBOUNCE_DELAY 50

enum ButtonIndex {
    BTN_UP = 0,
    BTN_DOWN = 1,
    BTN_SELECT = 2,
    BTN_BACK = 3,
    BTN_COUNT = 4,
};

enum ButtonEvent {
    BTN_NONE,
    BTN_PRESSED,
    BTN_RELEASED,
    BTN_HELD
};

struct Button {
    bool pressed;
    bool lastState;
    unsigned long lastPress;
    unsigned long pressTime;
    bool held;
};

class InputManager {
private:
    Button buttons[BTN_COUNT];
    int buttonPins[BTN_COUNT];
    unsigned long holdThreshold;
    
public:
    InputManager();
    void init();
    void update();
    
    // Button state queries
    bool isPressed(ButtonIndex btn);
    bool wasPressed(ButtonIndex btn);
    bool wasReleased(ButtonIndex btn);
    bool isHeld(ButtonIndex btn);
    ButtonEvent getButtonEvent(ButtonIndex btn);
    
    // Clear button states (useful after handling events)
    void clearButton(ButtonIndex btn);
    void clearAllButtons();
    
    // Check if any button is pressed
    bool anyButtonPressed();
    
    // Configuration
    void setHoldThreshold(unsigned long threshold) { holdThreshold = threshold; }
};

#endif // INPUT_MANAGER_H
