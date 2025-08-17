/**
 * MicBar Integration Example for HAI Menu
 * 
 * This example shows how to integrate the MicBar library into your existing HAI menu
 * to display real-time microphone levels while the HAI system is active.
 * 
 * To integrate this into your project:
 * 1. Add the includes at the top of hai_menu.cpp
 * 2. Add the global MicBar variable
 * 3. Initialize the MicBar in the HAI menu setup
 * 4. Update the display function to show the microphone bar
 */

// === ADD THESE INCLUDES TO hai_menu.cpp ===
#include "micbar.h"

// === ADD THIS GLOBAL VARIABLE ===
MicBar* haiMicBar = nullptr;

// === ADD THIS INITIALIZATION FUNCTION ===
void initializeHAIMicBar() {
    if (haiMicBar == nullptr && haiMicrophone != nullptr) {
        // Get TFT display from DisplayManager
        TFT_eSPI* tft = displayManager.getTFT();
        
        // Create MicBar at bottom of screen
        haiMicBar = new MicBar(tft, 10, 205, 220, 15);
        
        // Set colors for HAI theme
        haiMicBar->setColors(TFT_CYAN, TFT_BLACK, TFT_WHITE);
        
        DEBUG_PRINTLN("HAI MicBar initialized");
    }
}

// === ADD THIS TO displayHAIStatus() FUNCTION ===
void displayHAIStatus() {
    // ... existing code ...
    
    // Initialize MicBar if not already done
    if (haiMicBar == nullptr) {
        initializeHAIMicBar();
    }
    
    // Update microphone level bar
    if (haiMicBar != nullptr && haiMicrophone != nullptr && haiMicrophone->isInitialized()) {
        int micLevel = haiMicrophone->readLevel();
        
        // Color-code based on level for visual feedback
        if (micLevel < 1000) {
            haiMicBar->setColors(TFT_DARKGREEN, TFT_BLACK, TFT_WHITE);
        } else if (micLevel < 2500) {
            haiMicBar->setColors(TFT_GREEN, TFT_BLACK, TFT_WHITE);
        } else if (micLevel < 3500) {
            haiMicBar->setColors(TFT_YELLOW, TFT_BLACK, TFT_WHITE);
        } else {
            haiMicBar->setColors(TFT_RED, TFT_BLACK, TFT_WHITE);
        }
        
        haiMicBar->drawBar(micLevel);
        
        // Optional: Display level text
        displayManager.getTFT()->setTextColor(TFT_WHITE);
        displayManager.getTFT()->setTextSize(1);
        displayManager.getTFT()->drawString("Mic Level: " + String(micLevel), 10, 190);
    }
    
    // ... rest of existing code ...
}

// === ALTERNATIVE: Add to any menu display function ===
void updateMicBarDisplay() {
    static MicBar* micBar = nullptr;
    static unsigned long lastUpdate = 0;
    
    // Initialize once
    if (micBar == nullptr) {
        TFT_eSPI* tft = displayManager.getTFT();
        micBar = new MicBar(tft, 10, 200, 220, 20);
        micBar->setColors(TFT_GREEN, TFT_BLACK, TFT_DARKGREY);
    }
    
    // Update every 50ms to avoid overwhelming display
    if (millis() - lastUpdate > 50) {
        if (haiMicrophone && haiMicrophone->isInitialized()) {
            int level = haiMicrophone->readLevel();
            micBar->drawBar(level);
        }
        lastUpdate = millis();
    }
}

// === USAGE IN MAIN DISPLAY LOOP ===
void haiMenuLoop() {
    // ... existing menu code ...
    
    // Add this line to continuously update mic bar
    updateMicBarDisplay();
    
    // ... rest of menu code ...
}

// === CLEANUP FUNCTION ===
void cleanupHAIMicBar() {
    if (haiMicBar != nullptr) {
        delete haiMicBar;
        haiMicBar = nullptr;
        DEBUG_PRINTLN("HAI MicBar cleaned up");
    }
}
