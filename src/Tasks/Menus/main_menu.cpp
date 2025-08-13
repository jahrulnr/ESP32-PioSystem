#include "menus.h"

MenuState currentMenu = MENU_MAIN;
int selectedMenuItem = 0;
int selectedDeviceIndex = 0;

void drawMainMenu() {
  displayManager.clearScreen();
  displayManager.drawTitle("PioSystem Menu");
  
  // Menu items
  const char* menuItems[] = {
    "WiFi Status",
    "Scan Networks",
    "Connected Clients",
    "IoT Devices",
    "HAI Assistant",
    "Settings"
  };
  
  int startY = 40;
  int itemHeight = 20;
  
  const int numMenuItems = sizeof(menuItems) / sizeof(menuItems[0]);
  // keep use _min. this is the recommendation from Arduino.h
  for (int i = 0; i < _min(6, numMenuItems); i++) {
    uint16_t textColor = (i == selectedMenuItem) ? TFT_YELLOW : TFT_WHITE;
    String menuText = (i == selectedMenuItem) ? "> " + String(menuItems[i]) : "  " + String(menuItems[i]);
    displayManager.drawCenteredText(menuText, startY + i * itemHeight, textColor);
  }
  
  // Draw hint for button usage
  displayManager.drawCenteredText("UP/DOWN: Navigate  SELECT: Choose", 200, TFT_LIGHTGREY, 1);
}