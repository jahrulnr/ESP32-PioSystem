#include "menus.h"

MenuState currentMenu = MENU_MAIN;
int selectedMenuItem = 0;

void drawMainMenu() {
  displayManager.clearScreen();
  displayManager.drawTitle("PioSystem Menu");
  
  // Menu items
  const char* menuItems[] = {
    "WiFi Status",
    "Scan Networks",
    "Hotspot Status",
    "Connected Clients",
    "Settings"
  };
  
  int startY = 40;
  int itemHeight = 20;
  
  for (int i = 0; i < MAX_MENU_ITEMS; i++) {
    uint16_t textColor = (i == selectedMenuItem) ? TFT_YELLOW : TFT_WHITE;
    String menuText = (i == selectedMenuItem) ? "> " + String(menuItems[i]) : "  " + String(menuItems[i]);
    displayManager.drawCenteredText(menuText, startY + i * itemHeight, textColor);
  }
  
  // Draw hint for button usage
  displayManager.drawCenteredText("UP/DOWN: Navigate  SELECT: Choose", 200, TFT_LIGHTGREY, 1);
}