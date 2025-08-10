#include <Arduino.h>
#include <TFT_eSPI.h>
#include <ESPAsyncWebServer.h>
#include <MVCFramework.h>
#include "wifi_manager.h"
#include "display_manager.h"
#include "file_manager.h"
#include "SerialDebug.h"
#include "input_manager.h"
#include "httpclient.h"
#include "iot_device_manager.h"
#include "Routes/routes.h"
#include "Tasks/init.h"
#include "Tasks/Menus/menus.h"
#include "Tasks/Handler/tasks.h"

Application* app;
HttpClientManager* httpClientManager;
IoTDeviceManager* iotDeviceManager;

void setup() {
  // Initialize serial debugging
  DEBUG_BEGIN(115200);
  DEBUG_PRINTLN("\n=== PioSystem Initialization ===");
  
  // Initialize system components
  gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
  disableLoopWDT();
  pinMode(11, OUTPUT);
  initialize();
    
  // Initialize MVC application
  app = Application::getInstance();
  app->boot();

  // Initialize HTTP client manager
  httpClientManager = new HttpClientManager();
  httpClientManager->setDebugEnabled(true);
  httpClientManager->begin();

  // Initialize IoT Device Manager
  iotDeviceManager = new IoTDeviceManager(&wifiManager, httpClientManager);
  iotDeviceManager->begin();
  
  // Set up device discovery callbacks
  iotDeviceManager->setDeviceDiscoveredCallback([](const IoTDevice& device) {
    DEBUG_PRINTF("New IoT device discovered: %s (%s) - %s\n", 
                device.name.c_str(), device.ipAddress.c_str(), 
                IoTDeviceManager::deviceTypeToString(device.type).c_str());
  });
  
  iotDeviceManager->setDeviceStatusCallback([](const IoTDevice& device, bool isOnline) {
    DEBUG_PRINTF("Device %s (%s) is now %s\n", 
                device.name.c_str(), device.ipAddress.c_str(),
                isOnline ? "online" : "offline");
  });
  
  // Start device discovery with 30 second interval
  iotDeviceManager->startDiscovery(30000);
    
  // Register routes
  Router* router = app->getRouter();
  registerWebRoutes(router);
  registerApiRoutes(router);
  registerWebSocketRoutes(router);
  registerWifiRoutes(router, &wifiManager);
  registerIoTRoutes(router, iotDeviceManager);
  
  // Run the application (initializes the web server)
  app->run();
  
  DEBUG_PRINTLN("=== PioSystem Ready ===");
  DEBUG_PRINTLN("IoT Device Discovery: Active");
  DEBUG_PRINTLN("Access IoT API at: http://192.168.4.1/api/v1/iot/devices");
  
  // Draw the initial menu
  drawMainMenu();
}

// Empty loop() function as required by Arduino framework
void loop() {
  vTaskDelay(pdMS_TO_TICKS(10));
  vTaskDelete(NULL); // Delete the loop task to free memory
}
