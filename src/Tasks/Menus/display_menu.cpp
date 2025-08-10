#include "menus.h"

void displaySettings() {
  displayManager.clearScreen();
  displayManager.drawTitle("Settings");
  
  // Show current settings
  int y = 40;
  displayManager.drawCenteredText("Device Name: OSSystem", y, TFT_WHITE);
  y += 20;
  
  WiFiMode_t mode = wifiManager.getMode();
  String modeStr;
  switch (mode) {
    case WIFI_OFF: modeStr = "OFF"; break;
    case WIFI_STA: modeStr = "Station"; break;
    case WIFI_AP: modeStr = "Access Point"; break;
    case WIFI_AP_STA: modeStr = "Bridge Mode"; break;
    default: modeStr = "Unknown"; break;
  }
  
  displayManager.drawCenteredText("WiFi Mode: " + modeStr, y, TFT_WHITE);
  y += 20;
  
  // Show button hint
  displayManager.drawCenteredText("Press BACK to return", 200, TFT_LIGHTGREY, 1);
}