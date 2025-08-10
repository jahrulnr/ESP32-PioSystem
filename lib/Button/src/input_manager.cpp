#include "input_manager.h"
#include "SerialDebug.h"

InputManager::InputManager() {
    holdThreshold = 1000; // 1 second for hold
    
    buttonPins[BTN_UP] = BUTTON_UP;
    buttonPins[BTN_DOWN] = BUTTON_DOWN;
    buttonPins[BTN_SELECT] = BUTTON_SELECT;
    buttonPins[BTN_BACK] = BUTTON_BACK;
}

void InputManager::init() {
    for (int i = 0; i < BTN_COUNT; i++) {
        pinMode(buttonPins[i], INPUT_PULLUP);
        buttons[i].pressed = false;
        buttons[i].lastState = true;
        buttons[i].lastPress = 0;
        buttons[i].pressTime = 0;
        buttons[i].held = false;
    }
}

void InputManager::update() {
    unsigned long currentTime = millis();
    
    for (int i = 0; i < BTN_COUNT; i++) {
        bool currentState = digitalRead(buttonPins[i]);
        
        // Detect state change
        if (currentState != buttons[i].lastState) {
            buttons[i].lastPress = currentTime;
            buttons[i].lastState = currentState;
        }
        
        // Debounce and update button state
        if ((currentTime - buttons[i].lastPress) > DEBOUNCE_DELAY) {
            bool newPressed = !currentState; // Inverted because of pullup
            
            if (newPressed && !buttons[i].pressed) {
                // Button just pressed
                buttons[i].pressed = true;
                buttons[i].pressTime = currentTime;
                buttons[i].held = false;
                DEBUG_PRINTF("Button %d is pressed.\n", buttonPins[i]);
            } else if (!newPressed && buttons[i].pressed) {
                // Button just released
                buttons[i].pressed = false;
                buttons[i].held = false;
                DEBUG_PRINTF("Button %d is released.\n", buttonPins[i]);
            } else if (newPressed && buttons[i].pressed) {
                // Button is being held
                if (!buttons[i].held && (currentTime - buttons[i].pressTime) > holdThreshold) {
                    buttons[i].held = true;
                    DEBUG_PRINTF("Button %d is holded.\n", buttonPins[i]);
                }
            }
        }
    }
}

bool InputManager::isPressed(ButtonIndex btn) {
    if (btn >= BTN_COUNT) return false;
    return buttons[btn].pressed;
}

bool InputManager::wasPressed(ButtonIndex btn) {
    if (btn >= BTN_COUNT) return false;
    return buttons[btn].pressed && (millis() - buttons[btn].pressTime) < 50;
}

bool InputManager::wasReleased(ButtonIndex btn) {
    if (btn >= BTN_COUNT) return false;
    return !buttons[btn].pressed && buttons[btn].lastState;
}

bool InputManager::isHeld(ButtonIndex btn) {
    if (btn >= BTN_COUNT) return false;
    return buttons[btn].held;
}

ButtonEvent InputManager::getButtonEvent(ButtonIndex btn) {
    if (btn >= BTN_COUNT) return BTN_NONE;
    
    if (wasPressed(btn)) return BTN_PRESSED;
    if (wasReleased(btn)) return BTN_RELEASED;
    if (isHeld(btn)) return BTN_HELD;
    
    return BTN_NONE;
}

void InputManager::clearButton(ButtonIndex btn) {
    if (btn >= BTN_COUNT) return;
    buttons[btn].pressed = false;
    buttons[btn].held = false;
}

void InputManager::clearAllButtons() {
    for (int i = 0; i < BTN_COUNT; i++) {
        clearButton((ButtonIndex)i);
    }
}

bool InputManager::anyButtonPressed() {
    for (int i = 0; i < BTN_COUNT; i++) {
        if (buttons[i].pressed) {
            return true;
        }
    }
    return false;
}
