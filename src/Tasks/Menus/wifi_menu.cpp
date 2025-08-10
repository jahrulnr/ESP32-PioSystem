#include "menus.h"

void displayWiFiStatus() {
  displayManager.clearScreen();
  displayManager.drawTitle("WiFi Status");

  wl_status_t status = wifiManager.getStatus();
  WiFiMode_t mode = wifiManager.getMode();

  int y = 30;
  String modeStr;
  switch (mode) {
    case WIFI_OFF: modeStr = "OFF"; break;
    case WIFI_STA: modeStr = "Station"; break;
    case WIFI_AP: modeStr = "Access Point"; break;
    case WIFI_AP_STA: modeStr = "Bridge (AP+STA)"; break;
    default: modeStr = "Unknown"; break;
  }
  displayManager.drawCenteredText("Mode: " + modeStr, y, TFT_GREEN);
  y += 20;

  if (mode == WIFI_STA || mode == WIFI_AP_STA) {
    if (status == WL_CONNECTED) {
      displayManager.drawCenteredText("Connected: " + WiFi.SSID(), y, TFT_WHITE);
      y += 20;
      displayManager.drawCenteredText("IP: " + WiFi.localIP().toString(), y, TFT_WHITE);
      y += 20;
      displayManager.drawCenteredText("Signal: " + String(WiFi.RSSI()) + " dBm", y, TFT_WHITE);
      y += 20;
    } else {
      displayManager.drawCenteredText("Not connected", y, TFT_RED);
      y += 20;
    }
  }

  if (mode == WIFI_AP || mode == WIFI_AP_STA) {
    displayManager.drawCenteredText("AP SSID: " + String(WiFi.softAPSSID()), y, TFT_WHITE);
    y += 20;
    displayManager.drawCenteredText("AP IP: " + WiFi.softAPIP().toString(), y, TFT_WHITE);
    y += 20;
    
    int clientCount = WiFi.softAPgetStationNum();
    uint16_t clientColor = (clientCount > 0) ? TFT_GREEN : TFT_WHITE;
    displayManager.drawCenteredText("Clients: " + String(clientCount), y, clientColor);
    y += 20;
    
    // Display connected clients with hostnames if available
    if (clientCount > 0) {
      std::vector<ClientInfo> clients = wifiManager.getConnectedClients();
      displayManager.drawCenteredText("Connected Devices:", y, TFT_CYAN);
      y += 15;
      
      for (int i = 0; i < min(3, (int)clients.size()); i++) {
        String deviceInfo = clients[i].hostname.isEmpty() ? 
                            clients[i].ipAddress : 
                            clients[i].hostname;
        displayManager.drawCenteredText(deviceInfo, y, TFT_YELLOW);
        y += 15;
      }
      
      if (clients.size() > 3) {
        displayManager.drawCenteredText("+" + String(clients.size() - 3) + " more...", y, TFT_LIGHTGREY);
        y += 15;
      }
    }
    
    // Display web interface information
    displayManager.drawCenteredText("Web Interface:", y, TFT_CYAN);
    y += 15;
    displayManager.drawCenteredText("http://" + WiFi.softAPIP().toString(), y, TFT_CYAN);
  }
}