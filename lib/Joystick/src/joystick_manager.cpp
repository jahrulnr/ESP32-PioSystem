#include "joystick_manager.h"
#include "SerialDebug.h"

JoystickManager::JoystickManager(int maxJoysticks) {
    this->maxJoysticks = maxJoysticks;
    this->joystickCount = 0;
    
    // Allocate memory for joystick data and pins
    joysticks = new JoystickData[maxJoysticks];
    pins = new JoystickPin[maxJoysticks];
    
    // Set default settings
    deadzoneThreshold = DEADZONE_THRESHOLD;
    directionThreshold = DIRECTION_THRESHOLD;
    debounceDelay = SWITCH_DEBOUNCE_MS;
    
    // Initialize arrays
    for (int i = 0; i < maxJoysticks; i++) {
        joysticks[i] = {0};
        pins[i] = {-1, -1, -1};
    }
}

JoystickManager::~JoystickManager() {
    delete[] joysticks;
    delete[] pins;
}

bool JoystickManager::addJoystick(int vrxPin, int vryPin, int swPin) {
    if (joystickCount >= maxJoysticks) {
        DEBUG_PRINTLN("ERROR: Maximum number of joysticks reached");
        return false;
    }
    
    pins[joystickCount] = {vrxPin, vryPin, swPin};
    
    // Initialize joystick data
    joysticks[joystickCount].rawX = 0;
    joysticks[joystickCount].rawY = 0;
    joysticks[joystickCount].normalizedX = 0;
    joysticks[joystickCount].normalizedY = 0;
    joysticks[joystickCount].rotatedX = 0;
    joysticks[joystickCount].rotatedY = 0;
    joysticks[joystickCount].direction = JOYSTICK_CENTER;
    joysticks[joystickCount].switchPressed = false;
    joysticks[joystickCount].switchChanged = false;
    joysticks[joystickCount].lastSwitchChange = 0;
    joysticks[joystickCount].lastSwitchState = true; // Pullup
    
    // Initialize calibration with default values
    joysticks[joystickCount].centerX = CENTER_VALUE;
    joysticks[joystickCount].centerY = CENTER_VALUE;
    joysticks[joystickCount].minX = 0;
    joysticks[joystickCount].maxX = ANALOG_RESOLUTION - 1;
    joysticks[joystickCount].minY = 0;
    joysticks[joystickCount].maxY = ANALOG_RESOLUTION - 1;
    joysticks[joystickCount].calibrated = false;
    
    // Initialize rotation and orientation
    joysticks[joystickCount].rotation = JOYSTICK_ROTATION_0;
    joysticks[joystickCount].invertX = false;
    joysticks[joystickCount].invertY = true;
    
    joystickCount++;
    
    DEBUG_PRINT("Joystick ");
    DEBUG_PRINT(joystickCount - 1);
    DEBUG_PRINT(" added: VRx=");
    DEBUG_PRINT(vrxPin);
    DEBUG_PRINT(", VRy=");
    DEBUG_PRINT(vryPin);
    DEBUG_PRINT(", SW=");
    DEBUG_PRINTLN(swPin);
    
    return true;
}

bool JoystickManager::addJoystick(JoystickPin pinConfig) {
    return addJoystick(pinConfig.vrx, pinConfig.vry, pinConfig.sw);
}

void JoystickManager::removeJoystick(int index) {
    if (!isValidIndex(index)) return;
    
    // Shift all joysticks after this index down
    for (int i = index; i < joystickCount - 1; i++) {
        joysticks[i] = joysticks[i + 1];
        pins[i] = pins[i + 1];
    }
    
    joystickCount--;
    DEBUG_PRINT("Joystick ");
    DEBUG_PRINT(index);
    DEBUG_PRINTLN(" removed");
}

void JoystickManager::init() {
    DEBUG_PRINT("Initializing ");
    DEBUG_PRINT(joystickCount);
    DEBUG_PRINTLN(" joystick(s)...");
    
    for (int i = 0; i < joystickCount; i++) {
        if (pins[i].sw != -1) {
            pinMode(pins[i].sw, INPUT_PULLUP);
        }
        DEBUG_PRINT("Joystick ");
        DEBUG_PRINT(i);
        DEBUG_PRINTLN(" initialized");
    }
}

void JoystickManager::update() {
    for (int i = 0; i < joystickCount; i++) {
        updateJoystickData(i);
        updateSwitchState(i);
    }
}

void JoystickManager::updateJoystickData(int index) {
    if (!isValidIndex(index)) return;
    
    // Read raw analog values
    joysticks[index].rawX = analogRead(pins[index].vrx);
    joysticks[index].rawY = analogRead(pins[index].vry);
    
    // Normalize values to -100 to 100 range
    joysticks[index].normalizedX = normalizeValue(
        joysticks[index].rawX,
        joysticks[index].centerX,
        joysticks[index].minX,
        joysticks[index].maxX
    );
    
    joysticks[index].normalizedY = normalizeValue(
        joysticks[index].rawY,
        joysticks[index].centerY,
        joysticks[index].minY,
        joysticks[index].maxY
    );
    
    // Apply rotation and inversion
    applyRotation(index);
    
    // Calculate direction using rotated values
    joysticks[index].direction = calculateDirection(
        joysticks[index].rotatedX,
        joysticks[index].rotatedY,
        index
    );
}

void JoystickManager::updateSwitchState(int index) {
    if (!isValidIndex(index) || pins[index].sw == -1) return;
    
    unsigned long currentTime = millis();
    bool currentSwitchState = digitalRead(pins[index].sw);
    
    // Detect state change
    if (currentSwitchState != joysticks[index].lastSwitchState) {
        joysticks[index].lastSwitchChange = currentTime;
        joysticks[index].lastSwitchState = currentSwitchState;
        joysticks[index].switchChanged = true;
    } else {
        joysticks[index].switchChanged = false;
    }
    
    // Debounce and update switch state
    if ((currentTime - joysticks[index].lastSwitchChange) > debounceDelay) {
        joysticks[index].switchPressed = !currentSwitchState; // Inverted because of pullup
    }
}

int JoystickManager::normalizeValue(int raw, int center, int min, int max) {
    int normalized;
    
    if (raw > center) {
        // Map from center to max -> 0 to 100
        if (max == center) return 0;
        normalized = map(raw, center, max, 0, 100);
    } else {
        // Map from min to center -> -100 to 0
        if (min == center) return 0;
        normalized = map(raw, min, center, -100, 0);
    }
    
    // Apply deadzone
    if (abs(normalized) < deadzoneThreshold * 100 / ANALOG_RESOLUTION) {
        normalized = 0;
    }
    
    return constrain(normalized, -100, 100);
}

int JoystickManager::calculateDirection(int rotatedX, int rotatedY, int index) {
    // Apply directional threshold
    int threshold = directionThreshold * 100 / ANALOG_RESOLUTION;
    
    bool isUpDir = rotatedY > threshold;
    bool isDownDir = rotatedY < -threshold;
    bool isLeftDir = rotatedX < -threshold;
    bool isRightDir = rotatedX > threshold;
    
    // Determine direction
    if (isUpDir && isLeftDir) return JOYSTICK_UP_LEFT;
    if (isUpDir && isRightDir) return JOYSTICK_UP_RIGHT;
    if (isDownDir && isLeftDir) return JOYSTICK_DOWN_LEFT;
    if (isDownDir && isRightDir) return JOYSTICK_DOWN_RIGHT;
    if (isUpDir) return JOYSTICK_UP;
    if (isDownDir) return JOYSTICK_DOWN;
    if (isLeftDir) return JOYSTICK_LEFT;
    if (isRightDir) return JOYSTICK_RIGHT;
    
    return JOYSTICK_CENTER;
}

bool JoystickManager::isValidIndex(int index) {
    if (index < 0 || index >= joystickCount) {
        DEBUG_PRINT("ERROR: Invalid joystick index: ");
        DEBUG_PRINTLN(index);
        return false;
    }
    return true;
}

// Data access methods
JoystickData JoystickManager::getJoystickData(int index) {
    if (!isValidIndex(index)) {
        return {0}; // Return empty data
    }
    return joysticks[index];
}

int JoystickManager::getRawX(int index) {
    return isValidIndex(index) ? joysticks[index].rawX : 0;
}

int JoystickManager::getRawY(int index) {
    return isValidIndex(index) ? joysticks[index].rawY : 0;
}

int JoystickManager::getNormalizedX(int index) {
    return isValidIndex(index) ? joysticks[index].normalizedX : 0;
}

int JoystickManager::getNormalizedY(int index) {
    return isValidIndex(index) ? joysticks[index].normalizedY : 0;
}

int JoystickManager::getRotatedX(int index) {
    return isValidIndex(index) ? joysticks[index].rotatedX : 0;
}

int JoystickManager::getRotatedY(int index) {
    return isValidIndex(index) ? joysticks[index].rotatedY : 0;
}

int JoystickManager::getDirection(int index) {
    return isValidIndex(index) ? joysticks[index].direction : JOYSTICK_CENTER;
}

// Switch methods
bool JoystickManager::isSwitchPressed(int index) {
    return isValidIndex(index) ? joysticks[index].switchPressed : false;
}

bool JoystickManager::wasSwitchPressed(int index) {
    return isValidIndex(index) ? 
        (joysticks[index].switchPressed && joysticks[index].switchChanged) : false;
}

bool JoystickManager::wasSwitchReleased(int index) {
    return isValidIndex(index) ? 
        (!joysticks[index].switchPressed && joysticks[index].switchChanged) : false;
}

void JoystickManager::clearSwitchState(int index) {
    if (isValidIndex(index)) {
        joysticks[index].switchChanged = false;
    }
}

// Direction helpers
bool JoystickManager::isUp(int index) {
    int dir = getDirection(index);
    return (dir == JOYSTICK_UP || dir == JOYSTICK_UP_LEFT || dir == JOYSTICK_UP_RIGHT);
}

bool JoystickManager::isDown(int index) {
    int dir = getDirection(index);
    return (dir == JOYSTICK_DOWN || dir == JOYSTICK_DOWN_LEFT || dir == JOYSTICK_DOWN_RIGHT);
}

bool JoystickManager::isLeft(int index) {
    int dir = getDirection(index);
    return (dir == JOYSTICK_LEFT || dir == JOYSTICK_UP_LEFT || dir == JOYSTICK_DOWN_LEFT);
}

bool JoystickManager::isRight(int index) {
    int dir = getDirection(index);
    return (dir == JOYSTICK_RIGHT || dir == JOYSTICK_UP_RIGHT || dir == JOYSTICK_DOWN_RIGHT);
}

bool JoystickManager::isCenter(int index) {
    return getDirection(index) == JOYSTICK_CENTER;
}

bool JoystickManager::isDiagonal(int index) {
    int dir = getDirection(index);
    return (dir == JOYSTICK_UP_LEFT || dir == JOYSTICK_UP_RIGHT || 
            dir == JOYSTICK_DOWN_LEFT || dir == JOYSTICK_DOWN_RIGHT);
}

bool JoystickManager::isPressed(int index) {
    if (!isValidIndex(index)) return false;
    return getDirection(index) != JOYSTICK_CENTER || joysticks[index].switchPressed;
}

// Calibration methods
void JoystickManager::startCalibration(int index) {
    if (!isValidIndex(index)) return;
    
    joysticks[index].minX = ANALOG_RESOLUTION;
    joysticks[index].maxX = 0;
    joysticks[index].minY = ANALOG_RESOLUTION;
    joysticks[index].maxY = 0;
    joysticks[index].calibrated = false;
    
    DEBUG_PRINT("Started calibration for joystick ");
    DEBUG_PRINTLN(index);
}

void JoystickManager::calibrateCenter(int index) {
    if (!isValidIndex(index)) return;
    
    joysticks[index].centerX = analogRead(pins[index].vrx);
    joysticks[index].centerY = analogRead(pins[index].vry);
    
    DEBUG_PRINT("Center calibrated for joystick ");
    DEBUG_PRINT(index);
    DEBUG_PRINT(" - X: ");
    DEBUG_PRINT(joysticks[index].centerX);
    DEBUG_PRINT(", Y: ");
    DEBUG_PRINTLN(joysticks[index].centerY);
}

void JoystickManager::finishCalibration(int index) {
    if (!isValidIndex(index)) return;
    
    joysticks[index].calibrated = true;
    
    DEBUG_PRINT("Calibration finished for joystick ");
    DEBUG_PRINTLN(index);
    DEBUG_PRINT("X range: ");
    DEBUG_PRINT(joysticks[index].minX);
    DEBUG_PRINT(" - ");
    DEBUG_PRINTLN(joysticks[index].maxX);
    DEBUG_PRINT("Y range: ");
    DEBUG_PRINT(joysticks[index].minY);
    DEBUG_PRINT(" - ");
    DEBUG_PRINTLN(joysticks[index].maxY);
}

void JoystickManager::autoCalibrate(int index, unsigned long duration) {
    if (!isValidIndex(index)) return;
    
    DEBUG_PRINT("Auto-calibrating joystick ");
    DEBUG_PRINT(index);
    DEBUG_PRINT(" for ");
    DEBUG_PRINT(duration);
    DEBUG_PRINTLN("ms. Move joystick to all extremes...");
    
    startCalibration(index);
    
    unsigned long startTime = millis();
    while (millis() - startTime < duration) {
        int x = analogRead(pins[index].vrx);
        int y = analogRead(pins[index].vry);
        
        if (x < joysticks[index].minX) joysticks[index].minX = x;
        if (x > joysticks[index].maxX) joysticks[index].maxX = x;
        if (y < joysticks[index].minY) joysticks[index].minY = y;
        if (y > joysticks[index].maxY) joysticks[index].maxY = y;
        
        delay(10);
    }
    
    calibrateCenter(index);
    finishCalibration(index);
}

bool JoystickManager::isCalibrated(int index) {
    return isValidIndex(index) ? joysticks[index].calibrated : false;
}

void JoystickManager::resetCalibration(int index) {
    if (!isValidIndex(index)) return;
    
    joysticks[index].centerX = CENTER_VALUE;
    joysticks[index].centerY = CENTER_VALUE;
    joysticks[index].minX = 0;
    joysticks[index].maxX = ANALOG_RESOLUTION - 1;
    joysticks[index].minY = 0;
    joysticks[index].maxY = ANALOG_RESOLUTION - 1;
    joysticks[index].calibrated = false;
    
    DEBUG_PRINT("Calibration reset for joystick ");
    DEBUG_PRINTLN(index);
}

// Pin management
bool JoystickManager::setJoystickPins(int index, int vrxPin, int vryPin, int swPin) {
    if (!isValidIndex(index)) return false;
    
    pins[index] = {vrxPin, vryPin, swPin};
    
    if (swPin != -1) {
        pinMode(swPin, INPUT_PULLUP);
    }
    
    DEBUG_PRINT("Updated pins for joystick ");
    DEBUG_PRINT(index);
    DEBUG_PRINT(": VRx=");
    DEBUG_PRINT(vrxPin);
    DEBUG_PRINT(", VRy=");
    DEBUG_PRINT(vryPin);
    DEBUG_PRINT(", SW=");
    DEBUG_PRINTLN(swPin);
    
    return true;
}

JoystickPin JoystickManager::getJoystickPins(int index) {
    if (!isValidIndex(index)) {
        return {-1, -1, -1};
    }
    return pins[index];
}

// Convenience setup methods
void JoystickManager::setupDefaultTwoJoysticks() {
	setupMirroredJoysticks();
}

void JoystickManager::setupSingleJoystick(int vrxPin, int vryPin, int swPin) {
    addJoystick(vrxPin, vryPin, swPin);
}

// Convenience setup with rotation
void JoystickManager::setupJoystickWithRotation(int vrxPin, int vryPin, int swPin, int rotation) {
    addJoystick(vrxPin, vryPin, swPin);
    setRotation(joystickCount - 1, rotation); // Set rotation for the just-added joystick
}

void JoystickManager::setupTwoJoysticksWithRotation(int rotation1, int rotation2) {
    addJoystick(JOYSTICK1_VRX_PIN, JOYSTICK1_VRY_PIN, JOYSTICK1_SW_PIN);
    setRotation(0, rotation1);
    
    addJoystick(JOYSTICK2_VRX_PIN, JOYSTICK2_VRY_PIN, JOYSTICK2_SW_PIN);
    setRotation(1, rotation2);
}

void JoystickManager::setupMirroredJoysticks() {
    addJoystick(JOYSTICK1_VRX_PIN, JOYSTICK1_VRY_PIN, JOYSTICK1_SW_PIN);
    setRotation(0, JOYSTICK_ROTATION_180);
    addJoystick(JOYSTICK2_VRX_PIN, JOYSTICK2_VRY_PIN, JOYSTICK2_SW_PIN);
}

// Debug methods
void JoystickManager::printDebugInfo(int index) {
    if (!isValidIndex(index)) return;
    
    JoystickData& joy = joysticks[index];
    
    DEBUG_PRINT("Joystick ");
    DEBUG_PRINT(index);
    DEBUG_PRINT(": Raw(");
    DEBUG_PRINT(joy.rawX);
    DEBUG_PRINT(",");
    DEBUG_PRINT(joy.rawY);
    DEBUG_PRINT(") Norm(");
    DEBUG_PRINT(joy.normalizedX);
    DEBUG_PRINT(",");
    DEBUG_PRINT(joy.normalizedY);
    DEBUG_PRINT(") Rot(");
    DEBUG_PRINT(joy.rotatedX);
    DEBUG_PRINT(",");
    DEBUG_PRINT(joy.rotatedY);
    DEBUG_PRINT(") Dir:");
    DEBUG_PRINT(joy.direction);
    DEBUG_PRINT(" SW:");
    DEBUG_PRINT(joy.switchPressed ? "1" : "0");
    if (joy.calibrated) {
        DEBUG_PRINT(" [CAL]");
    }
    if (joy.rotation != 0) {
        DEBUG_PRINT(" [ROT:");
        DEBUG_PRINT(joy.rotation);
        DEBUG_PRINT("°]");
    }
    if (joy.invertX || joy.invertY) {
        DEBUG_PRINT(" [INV:");
        if (joy.invertX) DEBUG_PRINT("X");
        if (joy.invertY) DEBUG_PRINT("Y");
        DEBUG_PRINT("]");
    }
    DEBUG_PRINTLN();
}

void JoystickManager::printAllDebugInfo() {
    for (int i = 0; i < joystickCount; i++) {
        printDebugInfo(i);
    }
}

void JoystickManager::printConfiguration() {
    DEBUG_PRINTLN("=== Joystick Manager Configuration ===");
    DEBUG_PRINT("Active joysticks: ");
    DEBUG_PRINT(joystickCount);
    DEBUG_PRINT(" / ");
    DEBUG_PRINTLN(maxJoysticks);
    DEBUG_PRINT("Deadzone: ");
    DEBUG_PRINTLN(deadzoneThreshold);
    DEBUG_PRINT("Direction threshold: ");
    DEBUG_PRINTLN(directionThreshold);
    DEBUG_PRINT("Debounce delay: ");
    DEBUG_PRINT(debounceDelay);
    DEBUG_PRINTLN("ms");
    
    for (int i = 0; i < joystickCount; i++) {
        DEBUG_PRINT("Joystick ");
        DEBUG_PRINT(i);
        DEBUG_PRINT(": VRx=");
        DEBUG_PRINT(pins[i].vrx);
        DEBUG_PRINT(", VRy=");
        DEBUG_PRINT(pins[i].vry);
        DEBUG_PRINT(", SW=");
        DEBUG_PRINT(pins[i].sw);
        DEBUG_PRINT(" [");
        DEBUG_PRINT(joysticks[i].calibrated ? "Calibrated" : "Default");
        DEBUG_PRINT("] Rot:");
        DEBUG_PRINT(joysticks[i].rotation);
        DEBUG_PRINT("°");
        if (joysticks[i].invertX || joysticks[i].invertY) {
            DEBUG_PRINT(" Inv:");
            if (joysticks[i].invertX) DEBUG_PRINT("X");
            if (joysticks[i].invertY) DEBUG_PRINT("Y");
        }
        DEBUG_PRINTLN();
    }
    DEBUG_PRINTLN("=====================================");
}

// Rotation and orientation methods
void JoystickManager::applyRotation(int index) {
    if (!isValidIndex(index)) return;
    
    int x = joysticks[index].normalizedX;
    int y = joysticks[index].normalizedY;
    
    // Apply inversion first
    if (joysticks[index].invertX) x = -x;
    if (joysticks[index].invertY) y = -y;
    
    // Apply rotation
    switch (joysticks[index].rotation) {
        case JOYSTICK_ROTATION_0:
            joysticks[index].rotatedX = x;
            joysticks[index].rotatedY = y;
            break;
            
        case JOYSTICK_ROTATION_90:
            joysticks[index].rotatedX = -y;
            joysticks[index].rotatedY = x;
            break;
            
        case JOYSTICK_ROTATION_180:
            joysticks[index].rotatedX = -x;
            joysticks[index].rotatedY = -y;
            break;
            
        case JOYSTICK_ROTATION_270:
            joysticks[index].rotatedX = y;
            joysticks[index].rotatedY = -x;
            break;
            
        default:
            // Invalid rotation, use no rotation
            joysticks[index].rotatedX = x;
            joysticks[index].rotatedY = y;
            break;
    }
}

void JoystickManager::setRotation(int index, int degrees) {
    if (!isValidIndex(index)) return;
    
    // Normalize degrees to 0, 90, 180, 270
    degrees = degrees % 360;
    if (degrees < 0) degrees += 360;
    
    // Round to nearest 90 degrees
    if (degrees < 45) degrees = 0;
    else if (degrees < 135) degrees = 90;
    else if (degrees < 225) degrees = 180;
    else if (degrees < 315) degrees = 270;
    else degrees = 0;
    
    joysticks[index].rotation = degrees;
    
    DEBUG_PRINT("Joystick ");
    DEBUG_PRINT(index);
    DEBUG_PRINT(" rotation set to ");
    DEBUG_PRINT(degrees);
    DEBUG_PRINTLN("°");
}

int JoystickManager::getRotation(int index) {
    return isValidIndex(index) ? joysticks[index].rotation : 0;
}

void JoystickManager::setInvertX(int index, bool invert) {
    if (!isValidIndex(index)) return;
    
    joysticks[index].invertX = invert;
    
    DEBUG_PRINT("Joystick ");
    DEBUG_PRINT(index);
    DEBUG_PRINT(" X-axis inversion: ");
    DEBUG_PRINTLN(invert ? "ON" : "OFF");
}

void JoystickManager::setInvertY(int index, bool invert) {
    if (!isValidIndex(index)) return;
    
    joysticks[index].invertY = invert;
    
    DEBUG_PRINT("Joystick ");
    DEBUG_PRINT(index);
    DEBUG_PRINT(" Y-axis inversion: ");
    DEBUG_PRINTLN(invert ? "ON" : "OFF");
}

bool JoystickManager::isXInverted(int index) {
    return isValidIndex(index) ? joysticks[index].invertX : false;
}

bool JoystickManager::isYInverted(int index) {
    return isValidIndex(index) ? joysticks[index].invertY : false;
}

void JoystickManager::resetOrientation(int index) {
    if (!isValidIndex(index)) return;
    
    joysticks[index].rotation = JOYSTICK_ROTATION_0;
    joysticks[index].invertX = false;
    joysticks[index].invertY = false;
    
    DEBUG_PRINT("Joystick ");
    DEBUG_PRINT(index);
    DEBUG_PRINTLN(" orientation reset to default");
}
