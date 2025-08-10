#include "menus.h"

void displayNetworkList() {
  displayManager.clearScreen();
  displayManager.drawTitle("WiFi Networks");
  displayManager.drawCenteredText("Scanning...", 40, TFT_WHITE);

  int networkCount = wifiManager.scanNetworks();
  if (networkCount <= 0) {
    displayManager.drawCenteredText("No networks found", 70, TFT_RED);
    return;
  }

  displayManager.drawCenteredText("Scanning...", 40, COLOR_BG);

  std::vector<NetworkInfo> networks = wifiManager.getNetworkList();
  int y = 40;
  for (int i = 0; i < min(8, (int)networks.size()); i++) {
    int signalStrength = 0;
    if (networks[i].rssi > -50) signalStrength = 4;
    else if (networks[i].rssi > -65) signalStrength = 3;
    else if (networks[i].rssi > -75) signalStrength = 2;
    else if (networks[i].rssi > -85) signalStrength = 1;

    uint16_t color;
    switch (signalStrength) {
      case 4: color = TFT_GREEN; break;
      case 3: color = TFT_GREENYELLOW; break;
      case 2: color = TFT_YELLOW; break;
      case 1: color = TFT_ORANGE; break;
      default: color = TFT_RED; break;
    }

    String networkInfo = networks[i].ssid;
    if (networkInfo.length() > 16) {
      networkInfo = networkInfo.substring(0, 13) + "...";
    }
    networkInfo += " (";
    for (int j = 0; j < 4; j++) {
      networkInfo += (j < signalStrength) ? "|" : ".";
    }
    networkInfo += ")";
    if (networks[i].encryptionType != WIFI_AUTH_OPEN) {
      networkInfo += " [WPA/WPA2]";
    }
    displayManager.drawCenteredText(networkInfo, y, color);
    y += 15;
  }
  if (networks.size() > 8) {
    displayManager.drawCenteredText("+" + String(networks.size() - 8) + " more...", y, TFT_LIGHTGREY);
  }
}