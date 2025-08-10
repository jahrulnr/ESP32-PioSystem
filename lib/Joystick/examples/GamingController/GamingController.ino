/*
 * Gaming Controller Example
 * 
 * This example demonstrates how to use the JoystickManager for gaming applications
 * including dual joystick control, button combinations, gesture recognition,
 * and real-time response handling.
 * 
 * Features:
 * - Dual joystick gaming setup (movement + camera/aim)
 * - Button combination detection
 * - Gesture and pattern recognition
 * - Configurable sensitivity and response curves
 * - Game state management
 * - Player input logging and analysis
 * - Customizable control schemes
 * 
 * Hardware Requirements:
 * - ESP32-S3 development board
 * - 2 analog joystick modules (for full gaming experience)
 * - Optional: LED strip or display for visual feedback
 * 
 * Control Scheme:
 * Left Joystick (Movement):      Right Joystick (Camera/Aim):
 * - X/Y: Character movement      - X/Y: Camera rotation/aiming
 * - SW: Sprint/Run               - SW: Fire/Action
 * 
 * Joystick Connections:
 * Left Joystick (Player Movement):   Right Joystick (Camera/Aim):
 * - VRX → GPIO 16                    - VRX → GPIO 8  
 * - VRY → GPIO 15                    - VRY → GPIO 18
 * - SW  → GPIO 7                     - SW  → GPIO 17
 * 
 * Author: Your Name
 * Date: 2025
 */

#include <Arduino.h>
#include "joystick_manager.h"

// Create JoystickManager for dual-stick gaming
JoystickManager* gameController;

// Gaming control definitions
#define LEFT_STICK    0  // Movement stick
#define RIGHT_STICK   1  // Camera/aim stick

// Game state
enum GameState {
    MENU,
    PLAYING,
    PAUSED,
    SETTINGS
};

struct PlayerInput {
    // Movement
    float moveX, moveY;
    float aimX, aimY;
    bool sprint;
    bool fire;
    
    // Derived values
    float moveSpeed;
    float moveDirection;
    bool isMoving;
    bool isAiming;
    
    // Button states
    bool sprintPressed;
    bool firePressed;
    bool sprintHeld;
    bool fireHeld;
    
    // Timing
    unsigned long sprintStartTime;
    unsigned long fireStartTime;
    unsigned long lastInputTime;
};

struct GameSettings {
    float moveSensitivity;
    float aimSensitivity;
    int deadzone;
    bool invertYMovement;
    bool invertYAim;
    int responseMode; // 0=Linear, 1=Exponential, 2=Custom curve
};

// Game variables
GameState currentState = MENU;
PlayerInput playerInput;
GameSettings gameSettings;
unsigned long gameStartTime;
unsigned long lastUpdateTime;

// Statistics
struct InputStats {
    unsigned long totalInputs;
    unsigned long movementInputs;
    unsigned long aimInputs;
    unsigned long buttonPresses;
    float averageResponseTime;
    unsigned long sessionStartTime;
};

InputStats stats;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Gaming Controller Demo ===");
    Serial.println();
    
    // Initialize game controller
    gameController = new JoystickManager(2);
    
    // Setup dual joysticks for gaming
    setupGamingController();
    
    // Initialize game settings
    initializeGameSettings();
    
    // Initialize statistics
    resetStats();
    
    gameController->init();
    
    Serial.println("✅ Gaming Controller initialized!");
    Serial.println("🎮 Ready for dual-stick gaming action!");
    
    showControlScheme();
    enterGameState(MENU);
}

void loop() {
    // Update controller input
    gameController->update();
    
    // Process input based on current game state
    switch (currentState) {
        case MENU:
            handleMenuInput();
            break;
        case PLAYING:
            handleGameplayInput();
            break;
        case PAUSED:
            handlePauseInput();
            break;
        case SETTINGS:
            handleSettingsInput();
            break;
    }
    
    // Update game systems
    updatePlayerInput();
    updateGameLogic();
    updateDisplay();
    
    // Handle serial commands
    if (Serial.available()) {
        handleSerialCommands();
    }
    
    delay(16); // ~60 FPS update rate
}

void setupGamingController() {
    Serial.println("🔧 Setting up gaming controller...");
    
    // Add left joystick (movement)
    if (gameController->addJoystick(JOYSTICK1_VRX_PIN, JOYSTICK1_VRY_PIN, JOYSTICK1_SW_PIN)) {
        Serial.printf("✅ Left stick (movement) - VRX:%d, VRY:%d, SW:%d\\n", 
                      JOYSTICK1_VRX_PIN, JOYSTICK1_VRY_PIN, JOYSTICK1_SW_PIN);
    }
    
    // Add right joystick (camera/aim)
    if (gameController->addJoystick(JOYSTICK2_VRX_PIN, JOYSTICK2_VRY_PIN, JOYSTICK2_SW_PIN)) {
        Serial.printf("✅ Right stick (aim) - VRX:%d, VRY:%d, SW:%d\\n", 
                      JOYSTICK2_VRX_PIN, JOYSTICK2_VRY_PIN, JOYSTICK2_SW_PIN);
    }
    
    // Optimize for gaming
    gameController->setDeadzone(150);          // Smaller deadzone for precision
    gameController->setDirectionThreshold(800); // Higher threshold for directions
    gameController->setDebounceDelay(25);      // Faster response for gaming
    
    Serial.printf("Total joysticks: %d\\n", gameController->getJoystickCount());
}

void initializeGameSettings() {
    gameSettings.moveSensitivity = 1.0;
    gameSettings.aimSensitivity = 0.8;
    gameSettings.deadzone = 150;
    gameSettings.invertYMovement = false;
    gameSettings.invertYAim = true;  // Common in FPS games
    gameSettings.responseMode = 1;   // Exponential response
}

void resetStats() {
    stats.totalInputs = 0;
    stats.movementInputs = 0;
    stats.aimInputs = 0;
    stats.buttonPresses = 0;
    stats.averageResponseTime = 0;
    stats.sessionStartTime = millis();
}

void updatePlayerInput() {
    // Get raw joystick values
    int leftX = gameController->getNormalizedX(LEFT_STICK);
    int leftY = gameController->getNormalizedY(LEFT_STICK);
    int rightX = gameController->getNormalizedX(RIGHT_STICK);
    int rightY = gameController->getNormalizedY(RIGHT_STICK);
    
    // Apply sensitivity and response curves
    playerInput.moveX = applyResponseCurve(leftX / 100.0 * gameSettings.moveSensitivity);
    playerInput.moveY = applyResponseCurve(leftY / 100.0 * gameSettings.moveSensitivity);
    
    if (gameSettings.invertYMovement) {
        playerInput.moveY = -playerInput.moveY;
    }
    
    playerInput.aimX = applyResponseCurve(rightX / 100.0 * gameSettings.aimSensitivity);
    playerInput.aimY = applyResponseCurve(rightY / 100.0 * gameSettings.aimSensitivity);
    
    if (gameSettings.invertYAim) {
        playerInput.aimY = -playerInput.aimY;
    }
    
    // Calculate derived values
    playerInput.moveSpeed = sqrt(playerInput.moveX * playerInput.moveX + 
                                playerInput.moveY * playerInput.moveY);
    playerInput.moveDirection = atan2(playerInput.moveY, playerInput.moveX) * 180.0 / PI;
    
    playerInput.isMoving = playerInput.moveSpeed > 0.1;
    playerInput.isAiming = (abs(rightX) > 10 || abs(rightY) > 10);
    
    // Handle button states
    updateButtonStates();
    
    // Update statistics
    if (playerInput.isMoving || playerInput.isAiming) {
        stats.totalInputs++;
        playerInput.lastInputTime = millis();
        
        if (playerInput.isMoving) stats.movementInputs++;
        if (playerInput.isAiming) stats.aimInputs++;
    }
}

void updateButtonStates() {
    // Sprint button (left stick)
    bool leftPressed = gameController->isSwitchPressed(LEFT_STICK);
    bool leftJustPressed = gameController->wasSwitchPressed(LEFT_STICK);
    bool leftJustReleased = gameController->wasSwitchReleased(LEFT_STICK);
    
    if (leftJustPressed) {
        playerInput.sprintPressed = true;
        playerInput.sprintStartTime = millis();
        stats.buttonPresses++;
    }
    
    playerInput.sprintHeld = leftPressed;
    
    if (leftJustReleased) {
        playerInput.sprintPressed = false;
    }
    
    // Fire button (right stick)
    bool rightPressed = gameController->isSwitchPressed(RIGHT_STICK);
    bool rightJustPressed = gameController->wasSwitchPressed(RIGHT_STICK);
    bool rightJustReleased = gameController->wasSwitchReleased(RIGHT_STICK);
    
    if (rightJustPressed) {
        playerInput.firePressed = true;
        playerInput.fireStartTime = millis();
        stats.buttonPresses++;
    }
    
    playerInput.fireHeld = rightPressed;
    
    if (rightJustReleased) {
        playerInput.firePressed = false;
    }
}

float applyResponseCurve(float input) {
    switch (gameSettings.responseMode) {
        case 0: // Linear
            return input;
            
        case 1: // Exponential (more precision at low values)
            return input * abs(input);
            
        case 2: // Custom S-curve
            return input * input * input;
            
        default:
            return input;
    }
}

void handleMenuInput() {
    // Use joystick to navigate menu
    if (gameController->wasSwitchPressed(LEFT_STICK)) {
        enterGameState(PLAYING);
        Serial.println("🎮 Starting game!");
    }
    
    if (gameController->wasSwitchPressed(RIGHT_STICK)) {
        enterGameState(SETTINGS);
        Serial.println("⚙️  Entering settings...");
    }
    
    // Menu navigation with joystick directions
    if (gameController->isUp(LEFT_STICK)) {
        Serial.println("📱 Menu: Up");
    } else if (gameController->isDown(LEFT_STICK)) {
        Serial.println("📱 Menu: Down");
    }
}

void handleGameplayInput() {
    // Movement processing
    if (playerInput.isMoving) {
        processMovement();
    }
    
    // Aiming processing
    if (playerInput.isAiming) {
        processAiming();
    }
    
    // Action processing
    if (playerInput.sprintPressed) {
        Serial.println("🏃 Sprint activated!");
        playerInput.sprintPressed = false; // Clear flag
    }
    
    if (playerInput.firePressed) {
        Serial.println("💥 Fire!");
        playerInput.firePressed = false; // Clear flag
    }
    
    // Continuous actions
    if (playerInput.sprintHeld && playerInput.isMoving) {
        // Modify movement speed for sprinting
        // This would typically increase movement speed
    }
    
    if (playerInput.fireHeld) {
        // Continuous fire mode
        static unsigned long lastAutoFire = 0;
        if (millis() - lastAutoFire > 100) { // 10 rounds per second
            Serial.println("🔥 Auto-fire!");
            lastAutoFire = millis();
        }
    }
    
    // Game state transitions
    if (gameController->wasSwitchPressed(LEFT_STICK) && gameController->isSwitchPressed(RIGHT_STICK)) {
        enterGameState(PAUSED);
        Serial.println("⏸️  Game paused!");
    }
    
    // Gesture recognition
    detectGestures();
}

void processMovement() {
    // Convert joystick input to game movement
    float speed = playerInput.moveSpeed;
    float direction = playerInput.moveDirection;
    
    // Scale speed (0.0 to 1.0)
    if (playerInput.sprintHeld) {
        speed *= 1.5; // Sprint multiplier
    }
    
    // Display movement info
    static unsigned long lastMovementOutput = 0;
    if (millis() - lastMovementOutput > 500) { // Limit output frequency
        Serial.printf("🚶 Move: Speed=%.2f, Dir=%.1f°, Sprint=%s\\n", 
                     speed, direction, playerInput.sprintHeld ? "ON" : "OFF");
        lastMovementOutput = millis();
    }
}

void processAiming() {
    // Convert joystick input to camera/aim movement
    float aimSpeedX = playerInput.aimX;
    float aimSpeedY = playerInput.aimY;
    
    // Display aiming info
    static unsigned long lastAimOutput = 0;
    if (millis() - lastAimOutput > 500) { // Limit output frequency
        Serial.printf("🎯 Aim: X=%.2f, Y=%.2f\\n", aimSpeedX, aimSpeedY);
        lastAimOutput = millis();
    }
}

void detectGestures() {
    // Simple gesture detection - rapid movements
    static float lastMoveX = 0, lastMoveY = 0;
    static unsigned long gestureStartTime = 0;
    
    float deltaX = playerInput.moveX - lastMoveX;
    float deltaY = playerInput.moveY - lastMoveY;
    float gestureSpeed = sqrt(deltaX * deltaX + deltaY * deltaY);
    
    if (gestureSpeed > 0.8) { // Rapid movement threshold
        if (gestureStartTime == 0) {
            gestureStartTime = millis();
        } else if (millis() - gestureStartTime > 200) { // Sustained rapid movement
            Serial.println("🌀 Gesture: Rapid movement detected!");
            gestureStartTime = 0; // Reset
        }
    } else {
        gestureStartTime = 0; // Reset if movement slows down
    }
    
    lastMoveX = playerInput.moveX;
    lastMoveY = playerInput.moveY;
}

void handlePauseInput() {
    if (gameController->wasSwitchPressed(LEFT_STICK)) {
        enterGameState(PLAYING);
        Serial.println("▶️  Game resumed!");
    }
    
    if (gameController->wasSwitchPressed(RIGHT_STICK)) {
        enterGameState(MENU);
        Serial.println("📱 Returning to menu...");
    }
}

void handleSettingsInput() {
    // Use joysticks to adjust settings
    if (gameController->isUp(LEFT_STICK)) {
        gameSettings.moveSensitivity = min(2.0f, gameSettings.moveSensitivity + 0.1f);
        Serial.printf("⚙️  Move sensitivity: %.1f\\n", gameSettings.moveSensitivity);
        delay(200); // Prevent rapid changes
    }
    
    if (gameController->isDown(LEFT_STICK)) {
        gameSettings.moveSensitivity = max(0.1f, gameSettings.moveSensitivity - 0.1f);
        Serial.printf("⚙️  Move sensitivity: %.1f\\n", gameSettings.moveSensitivity);
        delay(200);
    }
    
    if (gameController->isUp(RIGHT_STICK)) {
        gameSettings.aimSensitivity = min(2.0f, gameSettings.aimSensitivity + 0.1f);
        Serial.printf("⚙️  Aim sensitivity: %.1f\\n", gameSettings.aimSensitivity);
        delay(200);
    }
    
    if (gameController->isDown(RIGHT_STICK)) {
        gameSettings.aimSensitivity = max(0.1f, gameSettings.aimSensitivity - 0.1f);
        Serial.printf("⚙️  Aim sensitivity: %.1f\\n", gameSettings.aimSensitivity);
        delay(200);
    }
    
    // Toggle inversions
    if (gameController->wasSwitchPressed(LEFT_STICK)) {
        gameSettings.invertYMovement = !gameSettings.invertYMovement;
        Serial.printf("⚙️  Invert Y movement: %s\\n", gameSettings.invertYMovement ? "ON" : "OFF");
    }
    
    if (gameController->wasSwitchPressed(RIGHT_STICK)) {
        gameSettings.invertYAim = !gameSettings.invertYAim;
        Serial.printf("⚙️  Invert Y aim: %s\\n", gameSettings.invertYAim ? "ON" : "OFF");
    }
    
    // Exit settings (both buttons together)
    if (gameController->isSwitchPressed(LEFT_STICK) && gameController->isSwitchPressed(RIGHT_STICK)) {
        enterGameState(MENU);
        Serial.println("📱 Returning to menu...");
        delay(500); // Prevent immediate trigger
    }
}

void updateGameLogic() {
    // This would contain actual game logic
    // For demo purposes, we'll just update timers and states
    
    static unsigned long lastLogicUpdate = 0;
    if (millis() - lastLogicUpdate > 1000) { // Update every second
        // Game logic would go here
        lastLogicUpdate = millis();
    }
}

void updateDisplay() {
    // Update display every 2 seconds with current status
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 2000) {
        showGameStatus();
        lastDisplayUpdate = millis();
    }
}

void enterGameState(GameState newState) {
    currentState = newState;
    
    switch (newState) {
        case MENU:
            Serial.println("\\n📱 === MAIN MENU ===");
            Serial.println("Left stick button: Start Game");
            Serial.println("Right stick button: Settings");
            Serial.println("Use stick to navigate");
            break;
            
        case PLAYING:
            Serial.println("\\n🎮 === GAME STARTED ===");
            gameStartTime = millis();
            break;
            
        case PAUSED:
            Serial.println("\\n⏸️  === GAME PAUSED ===");
            Serial.println("Left stick button: Resume");
            Serial.println("Right stick button: Menu");
            break;
            
        case SETTINGS:
            Serial.println("\\n⚙️  === SETTINGS ===");
            Serial.println("Left stick up/down: Move sensitivity");
            Serial.println("Right stick up/down: Aim sensitivity");
            Serial.println("Button presses: Toggle inversions");
            Serial.println("Both buttons: Return to menu");
            break;
    }
}

void showGameStatus() {
    if (currentState != PLAYING) return;
    
    unsigned long gameTime = (millis() - gameStartTime) / 1000;
    unsigned long sessionTime = (millis() - stats.sessionStartTime) / 1000;
    
    Serial.println("\\n📊 Game Status:");
    Serial.printf("Game time: %lu seconds\\n", gameTime);
    Serial.printf("Move: (%.2f, %.2f) Speed: %.2f\\n", 
                  playerInput.moveX, playerInput.moveY, playerInput.moveSpeed);
    Serial.printf("Aim: (%.2f, %.2f)\\n", playerInput.aimX, playerInput.aimY);
    Serial.printf("Actions: Sprint=%s, Fire=%s\\n", 
                  playerInput.sprintHeld ? "ON" : "OFF",
                  playerInput.fireHeld ? "ON" : "OFF");
    Serial.printf("Stats: Inputs=%lu, Buttons=%lu, Session=%lu s\\n",
                  stats.totalInputs, stats.buttonPresses, sessionTime);
}

void showControlScheme() {
    Serial.println("\\n🎮 Gaming Control Scheme:");
    Serial.println("==========================");
    Serial.println("Left Joystick (Movement):");
    Serial.println("  ↕️ X/Y Axis: Character movement");
    Serial.println("  🔘 Button: Sprint/Run");
    Serial.println("\\nRight Joystick (Camera/Aim):");
    Serial.println("  ↕️ X/Y Axis: Camera rotation/aiming");
    Serial.println("  🔘 Button: Fire/Action");
    Serial.println("\\nCombination Controls:");
    Serial.println("  🔘🔘 Both buttons: Pause/Menu");
    Serial.println();
}

void handleSerialCommands() {
    String command = Serial.readStringUntil('\\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "stats") {
        showDetailedStats();
    } else if (command == "settings") {
        showCurrentSettings();
    } else if (command == "reset") {
        resetStats();
        Serial.println("✅ Statistics reset!");
    } else if (command == "menu") {
        enterGameState(MENU);
    } else if (command == "help") {
        showControlScheme();
        Serial.println("Commands: stats, settings, reset, menu, help");
    }
}

void showDetailedStats() {
    unsigned long sessionTime = (millis() - stats.sessionStartTime) / 1000;
    
    Serial.println("\\n📈 Detailed Statistics:");
    Serial.println("========================");
    Serial.printf("Session time: %lu seconds\\n", sessionTime);
    Serial.printf("Total inputs: %lu\\n", stats.totalInputs);
    Serial.printf("Movement inputs: %lu (%.1f%%)\\n", 
                  stats.movementInputs, 
                  sessionTime > 0 ? (stats.movementInputs * 100.0 / sessionTime) : 0);
    Serial.printf("Aim inputs: %lu (%.1f%%)\\n", 
                  stats.aimInputs,
                  sessionTime > 0 ? (stats.aimInputs * 100.0 / sessionTime) : 0);
    Serial.printf("Button presses: %lu\\n", stats.buttonPresses);
    Serial.printf("Inputs per second: %.2f\\n", 
                  sessionTime > 0 ? (stats.totalInputs / (float)sessionTime) : 0);
}

void showCurrentSettings() {
    Serial.println("\\n⚙️  Current Settings:");
    Serial.println("====================");
    Serial.printf("Move sensitivity: %.1f\\n", gameSettings.moveSensitivity);
    Serial.printf("Aim sensitivity: %.1f\\n", gameSettings.aimSensitivity);
    Serial.printf("Deadzone: %d\\n", gameSettings.deadzone);
    Serial.printf("Invert Y movement: %s\\n", gameSettings.invertYMovement ? "ON" : "OFF");
    Serial.printf("Invert Y aim: %s\\n", gameSettings.invertYAim ? "ON" : "OFF");
    Serial.printf("Response mode: %s\\n", 
                  gameSettings.responseMode == 0 ? "Linear" : 
                  gameSettings.responseMode == 1 ? "Exponential" : "Custom");
}

/*
 * Advanced Gaming Features:
 * 
 * 1. Custom button mappings:
 *    // Allow users to remap buttons for different games
 *    
 * 2. Profile management:
 *    // Save/load different control profiles
 *    // Game-specific settings
 * 
 * 3. Advanced gestures:
 *    // Circular motions for special moves
 *    // Quick flick gestures for rapid actions
 * 
 * 4. Haptic feedback integration:
 *    // Vibration motors for force feedback
 *    // Different patterns for different actions
 * 
 * 5. Network multiplayer:
 *    // Send input data over WiFi
 *    // Remote gaming capabilities
 */
