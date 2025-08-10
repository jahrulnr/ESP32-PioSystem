#include "menus.h"

void displayConnectedClients() {
  displayManager.clearScreen();
  displayManager.drawTitle("Connected Clients");
  
  // Get client information
  std::vector<ClientInfo> clients = wifiManager.getConnectedClients();
  int y = 30;
  
  if (clients.empty()) {
    displayManager.drawCenteredText("No clients connected", y, TFT_RED);
  } else {
    displayManager.drawCenteredText(String(clients.size()) + " clients connected", y, TFT_GREEN);
    y += 20;
    
    for (size_t i = 0; i < clients.size(); i++) {
      // Display hostname if available, otherwise show IP
      String deviceName = clients[i].hostname.isEmpty() ? 
                           clients[i].ipAddress : 
                           clients[i].hostname;
      
      // Format client information
      String clientInfo = String(i+1) + ". " + deviceName;
      displayManager.drawCenteredText(clientInfo, y, TFT_WHITE);
      y += 20;
      
      // Display MAC address with smaller font
      displayManager.drawCenteredText("MAC: " + clients[i].macAddress, y, TFT_LIGHTGREY, 1);
      y += 15;
    }
  }
  
  // Show button hint
  displayManager.drawCenteredText("Press BACK to return", 200, TFT_LIGHTGREY, 1);
}