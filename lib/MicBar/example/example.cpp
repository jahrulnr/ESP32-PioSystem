/**
 * MicBar TFT Example
 * 
 * This example shows how to use the MicBar library with TFT_eSPI
 * in the OSSystem project. It integrates with the existing DisplayManager
 * and AnalogMicrophone systems.
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "micbar.h"
#include "display_manager.h"
#include "AnalogMicrophone.h"

// Initialize display and microphone
TFT_eSPI tft = TFT_eSPI();
AnalogMicrophone microphone(1); // Assuming mic is on pin 1
MicBar micBar(&tft);

void setup() {
    Serial.begin(115200);
    
    // Initialize TFT display
    tft.init();
    tft.setRotation(1); // Adjust rotation as needed
    tft.fillScreen(TFT_BLACK);
    
    // Initialize microphone
    if (!microphone.init(16000)) {
        Serial.println("Failed to initialize microphone!");
        return;
    }
    
    // Configure MicBar
    micBar.setPosition(20, 100);          // Position on screen
    micBar.setSize(200, 20);              // Width and height
    micBar.setColors(TFT_GREEN, TFT_BLACK, TFT_WHITE); // Bar, background, border colors
    
    Serial.println("MicBar TFT Example initialized");
}

void loop() {
    // Read current microphone level
    int micLevel = microphone.readLevel();
    
    // Update and draw the microphone bar
    micBar.drawBar(micLevel);
    
    // Print level to serial for debugging
    Serial.printf("Mic Level: %d\n", micLevel);
    
    // Small delay to prevent overwhelming the display
    delay(50);
}

/* 
 * Integration with DisplayManager example:
 * 
 * If you want to integrate this with your existing DisplayManager system:
 * 
 * 1. In your main menu or display task:
 * 
 * DisplayManager* displayMgr = DisplayManager::getInstance();
 * TFT_eSPI* tft = displayMgr->getTft(); // You may need to add this getter
 * MicBar micBar(tft, 10, 200, 220, 15); // Position at bottom of screen
 * 
 * 2. In your microphone task or audio processing:
 * 
 * void updateMicDisplay() {
 *     if (microphoneManager && microphoneManager->isInitialized()) {
 *         int level = microphoneManager->readLevel();
 *         micBar.drawBar(level);
 *     }
 * }
 * 
 * 3. Color coding based on levels:
 * 
 * void updateMicBarWithColors(int level) {
 *     if (level < 1000) {
 *         micBar.setColors(TFT_DARKGREEN, TFT_BLACK, TFT_WHITE);
 *     } else if (level < 2500) {
 *         micBar.setColors(TFT_GREEN, TFT_BLACK, TFT_WHITE);
 *     } else if (level < 3500) {
 *         micBar.setColors(TFT_YELLOW, TFT_BLACK, TFT_WHITE);
 *     } else {
 *         micBar.setColors(TFT_RED, TFT_BLACK, TFT_WHITE);
 *     }
 *     micBar.drawBar(level);
 * }
 */
