#include "tasks.h"
#include "iot_device_manager.h"

InputManager inputManager;
TaskHandle_t inputTaskHandle = NULL;

void inputTask(void* parameter) {
  for (;;) {
    // Update input manager
    inputManager.update();
    
    // Check if any button was pressed and wake display if sleeping
    if (inputManager.anyButtonPressed()) {
      DisplayManager* display = DisplayManager::getInstance();
      if (display != nullptr && !display->isScreenOn()) {
        // Wake up the display
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
          display->wake();
          xSemaphoreGive(displayMutex);
        }
        // Clear all button states since this was just a wake-up event
        inputManager.clearAllButtons();
        // Continue to next iteration without processing menu events
        vTaskDelay(pdMS_TO_TICKS(20));
        continue;
      } else if (display != nullptr) {
        // Update activity timestamp when buttons are pressed and screen is on
        display->updateActivity();
      }
    }
    
    // Handle button events only if screen is on
    DisplayManager* display = DisplayManager::getInstance();
    if (display != nullptr && display->isScreenOn()) {
      if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        handleButtonEvents();
        xSemaphoreGive(displayMutex);
      }
    }
    
    // Small delay to reduce CPU usage
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void handleButtonEvents() {
  // Check for power button long press (BACK button held for sleep)
  if (inputManager.isHeld(BTN_BACK)) {
    // Long press on BACK button triggers sleep mode
    forceSleep();
    inputManager.clearButton(BTN_BACK);
    return;
  }
  
  // Process button events based on current menu state
  switch (currentMenu) {
    case MENU_MAIN:
      handleMenuNavigation();
      break;
    
    case MENU_WIFI_STATUS:
      // In WiFi status screen, BACK button returns to main menu
      if (inputManager.wasPressed(BTN_BACK)) {
        currentMenu = MENU_MAIN;
        drawMainMenu();
        inputManager.clearButton(BTN_BACK);
      }
      break;
    
    case MENU_WIFI_SCAN:
      // In WiFi scan screen, BACK button returns to main menu
      if (inputManager.wasPressed(BTN_BACK)) {
        currentMenu = MENU_MAIN;
        drawMainMenu();
        inputManager.clearButton(BTN_BACK);
      }
      break;
      
    case MENU_SETTINGS:
      // In settings screen, BACK button returns to main menu
      if (inputManager.wasPressed(BTN_BACK)) {
        currentMenu = MENU_MAIN;
        drawMainMenu();
        inputManager.clearButton(BTN_BACK);
      }
      break;
      
    case MENU_CLIENTS:
      // In clients screen, BACK button returns to main menu
      if (inputManager.wasPressed(BTN_BACK)) {
        currentMenu = MENU_MAIN;
        drawMainMenu();
        inputManager.clearButton(BTN_BACK);
      }
      break;
      
    case MENU_IOT_DEVICES:
      // In IoT devices screen, BACK button returns to main menu
      if (inputManager.wasPressed(BTN_BACK)) {
        currentMenu = MENU_MAIN;
        drawMainMenu();
        inputManager.clearButton(BTN_BACK);
      }
      // SELECT button triggers a manual scan
      if (inputManager.wasPressed(BTN_SELECT)) {
        if (iotDeviceManager != nullptr) {
          iotDeviceManager->startManualScan();
        }
        displayIoTDevices(); // Refresh the display
        inputManager.clearButton(BTN_SELECT);
      }
      break;
  }

  inputManager.clearAllButtons();
}

void handleMenuNavigation() {
  // Handle UP/DOWN navigation
  if (inputManager.wasPressed(BTN_UP)) {
    selectedMenuItem = (selectedMenuItem > 0) ? selectedMenuItem - 1 : MAX_MENU_ITEMS - 1;
    drawMainMenu();
    inputManager.clearButton(BTN_UP);
  }
  
  if (inputManager.wasPressed(BTN_DOWN)) {
    selectedMenuItem = (selectedMenuItem < MAX_MENU_ITEMS - 1) ? selectedMenuItem + 1 : 0;
    drawMainMenu();
    inputManager.clearButton(BTN_DOWN);
  }
  
  // Handle SELECT button to execute the selected action
  if (inputManager.wasPressed(BTN_SELECT)) {
    executeMenuAction(selectedMenuItem);
    inputManager.clearButton(BTN_SELECT);
  }

  vTaskDelay(pdMS_TO_TICKS(100));
}

void executeMenuAction(int menuItem) {
  switch (menuItem) {
    case 0: // WiFi Status
      currentMenu = MENU_WIFI_STATUS;
      displayWiFiStatus();
      break;
      
    case 1: // Scan Networks
      currentMenu = MENU_WIFI_SCAN;
      displayNetworkList();
      break;
      
    case 2: // Hotspot
      currentMenu = MENU_WIFI_STATUS;
      displayWiFiStatus();
      break;
      
    case 3: // Connected Clients
      currentMenu = MENU_CLIENTS;
      displayConnectedClients();
      break;
      
    case 4: // IoT Devices
      currentMenu = MENU_IOT_DEVICES;
      displayIoTDevices();
      break;
      
    case 5: // Settings
      currentMenu = MENU_SETTINGS;
      displaySettings();
      break;
  }
}

void forceSleep() {
  DisplayManager* display = DisplayManager::getInstance();
  if (display != nullptr) {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
      display->sleep();
      xSemaphoreGive(displayMutex);
    }
  }
}

void setSleepTimeout(unsigned long timeoutMs) {
  DisplayManager* display = DisplayManager::getInstance();
  if (display != nullptr) {
    display->setSleepTimeout(timeoutMs);
    DEBUG_PRINTF("Sleep timeout set to %lu ms\n", timeoutMs);
  }
}