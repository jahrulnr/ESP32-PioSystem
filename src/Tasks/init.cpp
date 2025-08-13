#include "init.h"

TFT_eSPI tft = TFT_eSPI();
DisplayManager displayManager(&tft);
SemaphoreHandle_t displayMutex = NULL;

WiFiManager wifiManager("PioSystem", "tes12345");
AsyncWebServer server(80);
HttpClientManager* httpClientManager;
IoTDeviceManager* iotDeviceManager;

void initialize() {
  // Create synchronization primitives first
  displayMutex = xSemaphoreCreateMutex();
  if (displayMutex == NULL) {
    DEBUG_PRINTLN("Failed to create display mutex");
  }

  // Initialize display
  displayManager.init();
  displayManager.clearScreen();
  displayManager.drawTitle("PioSystem Initializing...");
  displayManager.drawCenteredText("Please wait...", 40, TFT_WHITE, 2);
  
  // Initialize input manager
  inputManager.init();
  inputManager.setHoldThreshold(1000); // 1 second hold threshold
  
  // Initialize file system for web assets
  if (!SPIFFS.begin(true)) {
    DEBUG_PRINTLN("Failed to mount file system");
  } else {
    DEBUG_PRINTLN("File system mounted successfully");
		// List all files in SPIFFS for debugging
		DEBUG_PRINTLN("SPIFFS files:");
		File root = SPIFFS.open("/");
		File file = root.openNextFile();
		while (file) {
			DEBUG_PRINTF("  %s (%d bytes)\n", file.name(), file.size());
			file = root.openNextFile();
		}
		
		// Make sure database directory exists
		if (!SPIFFS.exists("/database")) {
			DEBUG_PRINTLN("Creating database directory...");
			SPIFFS.mkdir("/database");
		}
  }
  
  // Start WiFi manager in AP mode for hotspot functionality
  if (wifiManager.startAccessPoint()) {
    DEBUG_PRINTLN("WiFi AP started successfully");
    DEBUG_PRINTLN("AP IP address: " + WiFi.softAPIP().toString());
    
    // Register WiFi event handler
    wifiManager.setEventCallback([](WiFiEvent_t event, WiFiEventInfo_t info) {
      uint32_t ipAddr = info.got_ip.ip_info.ip.addr;
      IPAddress clientIP(ipAddr);
      char macStr[18] = { 0 };
      switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
          sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
            info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
            info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);
          
          DEBUG_PRINTF("Client connected: %s\n", macStr);
          break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
          sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            info.wifi_ap_stadisconnected.mac[0], info.wifi_ap_stadisconnected.mac[1],
            info.wifi_ap_stadisconnected.mac[2], info.wifi_ap_stadisconnected.mac[3],
            info.wifi_ap_stadisconnected.mac[4], info.wifi_ap_stadisconnected.mac[5]);
            
          DEBUG_PRINTF("Client disconnected: %s\n", macStr);
          break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
          sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            info.wifi_ap_staipassigned.mac[0], info.wifi_ap_staipassigned.mac[1],
            info.wifi_ap_staipassigned.mac[2], info.wifi_ap_staipassigned.mac[3],
            info.wifi_ap_staipassigned.mac[4], info.wifi_ap_staipassigned.mac[5]);
          DEBUG_PRINTF("Client assigned: %s\n", macStr);
          break;
      }
    });
  } else {
    DEBUG_PRINTLN("Failed to start WiFi AP");
  }

  // Initialize HTTP client manager
  httpClientManager = new HttpClientManager();
  httpClientManager->setDebugEnabled(true);
  httpClientManager->begin();

  // Initialize IoT Device Manager
  iotDeviceManager = new IoTDeviceManager(&wifiManager, httpClientManager);
  iotDeviceManager->begin();

  setupTasks();
}