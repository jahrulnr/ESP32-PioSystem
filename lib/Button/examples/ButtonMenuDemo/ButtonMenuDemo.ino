/*
 * Button Menu Demo Example
 * 
 * This example demonstrates how to create a simple menu system using buttons.
 * It shows navigation through menu items, selection, and back functionality.
 * 
 * Features:
 * - Scrollable menu with UP/DOWN navigation
 * - SELECT button to choose menu items
 * - BACK button to return to previous menu or exit
 * - Submenu support
 * - Visual feedback in Serial Monitor
 * 
 * Hardware Requirements:
 * - ESP32 development board
 * - 4 push buttons (UP, DOWN, SELECT, BACK)
 * 
 * Controls:
 * - UP/DOWN: Navigate menu items
 * - SELECT: Choose current item / Enter submenu
 * - BACK: Return to previous menu / Exit
 * 
 * Author: Your Name
 * Date: 2025
 */

#include <Arduino.h>
#include "input_manager.h"

// Create input manager instance
InputManager* inputMgr;

// Menu system structures
enum MenuState {
    MENU_MAIN,
    MENU_SETTINGS,
    MENU_ABOUT,
    MENU_DEMO
};

struct MenuItem {
    const char* name;
    MenuState nextState;
    void (*action)();
};

// Current menu state
MenuState currentMenu = MENU_MAIN;
int selectedItem = 0;
int maxItems = 0;

// Forward declarations
void actionSettings();
void actionAbout();
void actionDemo();
void actionWiFi();
void actionDisplay();
void actionSystem();
void actionExit();

// Main menu items
MenuItem mainMenu[] = {
    {"⚙️  Settings", MENU_SETTINGS, actionSettings},
    {"ℹ️  About", MENU_ABOUT, actionAbout},
    {"🎮 Demo", MENU_DEMO, actionDemo},
    {"🚪 Exit", MENU_MAIN, actionExit}
};

// Settings submenu items  
MenuItem settingsMenu[] = {
    {"📶 WiFi Setup", MENU_SETTINGS, actionWiFi},
    {"🖥️  Display", MENU_SETTINGS, actionDisplay},
    {"⚙️  System", MENU_SETTINGS, actionSystem},
    {"⬅️  Back", MENU_MAIN, nullptr}
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Button Menu Demo ===");
    Serial.println();
    
    // Create and initialize input manager
    inputMgr = new InputManager();
    inputMgr->init();
    
    // Set shorter hold threshold for responsive menu
    inputMgr->setHoldThreshold(1000);
    
    Serial.println("🎮 Controls:");
    Serial.println("UP/DOWN - Navigate menu");
    Serial.println("SELECT  - Choose item");
    Serial.println("BACK    - Previous menu");
    Serial.println();
    
    // Show initial menu
    showCurrentMenu();
}

void loop() {
    // Update button states
    inputMgr->update();
    
    // Handle menu navigation
    handleMenuInput();
    
    delay(50); // Smooth operation
}

void handleMenuInput() {
    // UP button - Navigate up
    if (inputMgr->wasPressed(BTN_UP)) {
        selectedItem = (selectedItem - 1 + maxItems) % maxItems;
        showCurrentMenu();
        inputMgr->clearButton(BTN_UP);
    }
    
    // DOWN button - Navigate down
    if (inputMgr->wasPressed(BTN_DOWN)) {
        selectedItem = (selectedItem + 1) % maxItems;
        showCurrentMenu();
        inputMgr->clearButton(BTN_DOWN);
    }
    
    // SELECT button - Choose item
    if (inputMgr->wasPressed(BTN_SELECT)) {
        selectCurrentItem();
        inputMgr->clearButton(BTN_SELECT);
    }
    
    // BACK button - Previous menu
    if (inputMgr->wasPressed(BTN_BACK)) {
        goBack();
        inputMgr->clearButton(BTN_BACK);
    }
    
    // Hold SELECT for quick action
    if (inputMgr->isHeld(BTN_SELECT)) {
        Serial.println("🚀 Quick Action: SELECT held!");
        inputMgr->clearButton(BTN_SELECT);
    }
}

void showCurrentMenu() {
    Serial.println("\n" + String("=").substring(0, 30));
    
    MenuItem* menu;
    String title;
    
    switch (currentMenu) {
        case MENU_MAIN:
            menu = mainMenu;
            maxItems = sizeof(mainMenu) / sizeof(MenuItem);
            title = "📋 MAIN MENU";
            break;
            
        case MENU_SETTINGS:
            menu = settingsMenu;
            maxItems = sizeof(settingsMenu) / sizeof(MenuItem);
            title = "⚙️  SETTINGS";
            break;
            
        default:
            return;
    }
    
    Serial.println(title);
    Serial.println(String("=").substring(0, 30));
    
    // Display menu items
    for (int i = 0; i < maxItems; i++) {
        if (i == selectedItem) {
            Serial.println("👉 " + String(menu[i].name));
        } else {
            Serial.println("   " + String(menu[i].name));
        }
    }
    
    Serial.println(String("=").substring(0, 30));
    Serial.println("UP/DOWN=Navigate, SELECT=Choose, BACK=Return");
}

void selectCurrentItem() {
    MenuItem* menu;
    
    switch (currentMenu) {
        case MENU_MAIN:
            menu = mainMenu;
            break;
        case MENU_SETTINGS:
            menu = settingsMenu;
            break;
        default:
            return;
    }
    
    MenuItem selected = menu[selectedItem];
    
    Serial.println("\n✅ Selected: " + String(selected.name));
    
    // Execute action if available
    if (selected.action != nullptr) {
        selected.action();
    }
    
    // Change menu state if different
    if (selected.nextState != currentMenu) {
        currentMenu = selected.nextState;
        selectedItem = 0; // Reset selection
        showCurrentMenu();
    }
}

void goBack() {
    switch (currentMenu) {
        case MENU_SETTINGS:
        case MENU_ABOUT:
        case MENU_DEMO:
            currentMenu = MENU_MAIN;
            selectedItem = 0;
            showCurrentMenu();
            break;
            
        case MENU_MAIN:
            Serial.println("\n👋 Exiting menu system...");
            Serial.println("Press any button to return to main menu");
            break;
    }
}

// Menu action functions
void actionSettings() {
    Serial.println("🔧 Opening Settings...");
    delay(500); // Simulate loading
}

void actionAbout() {
    Serial.println("\n📋 ABOUT THIS DEMO");
    Serial.println("====================");
    Serial.println("Button Menu Demo v1.0");
    Serial.println("ESP32 Input Manager Example");
    Serial.println("Author: Your Name");
    Serial.println("Date: 2025");
    Serial.println("====================");
    Serial.println("\nPress BACK to return...");
    
    // Wait for back button
    waitForBack();
}

void actionDemo() {
    Serial.println("\n🎮 INTERACTIVE DEMO");
    Serial.println("===================");
    Serial.println("Try these button combinations:");
    Serial.println("- Hold UP: Rapid scroll demo");
    Serial.println("- Hold DOWN: Reverse scroll demo");  
    Serial.println("- UP+DOWN: Special combination");
    Serial.println("- Hold SELECT: Quick action");
    Serial.println("===================");
    
    runInteractiveDemo();
}

void actionWiFi() {
    Serial.println("📶 WiFi Setup selected");
    Serial.println("(WiFi configuration would go here)");
    delay(1000);
}

void actionDisplay() {
    Serial.println("🖥️  Display settings selected");
    Serial.println("(Display configuration would go here)");
    delay(1000);
}

void actionSystem() {
    Serial.println("⚙️  System settings selected");
    Serial.println("(System configuration would go here)");
    delay(1000);
}

void actionExit() {
    Serial.println("\n👋 Thanks for using the Button Menu Demo!");
    Serial.println("Restart to try again.");
    while(true) {
        delay(1000);
    }
}

void waitForBack() {
    while (true) {
        inputMgr->update();
        if (inputMgr->wasPressed(BTN_BACK)) {
            inputMgr->clearButton(BTN_BACK);
            currentMenu = MENU_MAIN;
            selectedItem = 0;
            showCurrentMenu();
            break;
        }
        delay(50);
    }
}

void runInteractiveDemo() {
    Serial.println("\nDemo running... Press BACK to exit demo");
    
    unsigned long startTime = millis();
    int demoCounter = 0;
    
    while (true) {
        inputMgr->update();
        
        // Exit demo on BACK
        if (inputMgr->wasPressed(BTN_BACK)) {
            inputMgr->clearButton(BTN_BACK);
            currentMenu = MENU_MAIN;
            selectedItem = 0;
            showCurrentMenu();
            break;
        }
        
        // Demo interactions
        if (inputMgr->isHeld(BTN_UP)) {
            Serial.println("⬆️  Rapid UP action! Counter: " + String(++demoCounter));
            delay(200);
        }
        
        if (inputMgr->isHeld(BTN_DOWN)) {
            Serial.println("⬇️  Rapid DOWN action! Counter: " + String(--demoCounter));
            delay(200);
        }
        
        if (inputMgr->isPressed(BTN_UP) && inputMgr->isPressed(BTN_DOWN)) {
            Serial.println("🔄 UP+DOWN combo! Resetting counter...");
            demoCounter = 0;
            inputMgr->clearAllButtons();
        }
        
        if (inputMgr->isHeld(BTN_SELECT)) {
            Serial.println("⭐ SELECT held - Quick demo action!");
            inputMgr->clearButton(BTN_SELECT);
        }
        
        // Show demo status every 3 seconds
        if (millis() - startTime >= 3000) {
            Serial.println("🎮 Demo active - Counter: " + String(demoCounter) + " (BACK to exit)");
            startTime = millis();
        }
        
        delay(50);
    }
}
