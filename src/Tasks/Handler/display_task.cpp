#include "tasks.h"

TaskHandle_t displayUpdateTaskHandle = NULL;

void displayUpdateTask(void* parameter) {
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t updateFrequency = pdMS_TO_TICKS(5000); // 5 seconds
  
  for (;;) {
    // Wait for the next cycle
    vTaskDelayUntil(&lastWakeTime, updateFrequency);
    
    if (currentMenu == MENU_WIFI_STATUS || currentMenu == MENU_CLIENTS || currentMenu == MENU_HAI) {
      // Check for client info updates
      std::vector<ClientInfo> clients = wifiManager.getConnectedClients();
      if (!clients.empty()) {
        // Print client information to serial for debugging
        DEBUG_PRINTLN("Connected clients:");
        for (const auto& client : clients) {
          DEBUG_PRINTF("MAC: %s, IP: %s, Hostname: %s\n", 
                     client.macAddress.c_str(), 
                     client.ipAddress.c_str(), 
                     client.hostname.c_str());
        }
      }
      
      // Update the display if we can get the mutex
      if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (currentMenu == MENU_WIFI_STATUS) {
          displayWiFiStatus();
        } else if (currentMenu == MENU_CLIENTS) {
          displayConnectedClients();
        } else if (currentMenu == MENU_HAI) {
          displayHAI();
        }
        xSemaphoreGive(displayMutex);
      }
    }
  }
}