#include <Arduino.h>
#include <TFT_eSPI.h>
#include <ESPAsyncWebServer.h>
#include <MVCFramework.h>
#include "wifi_manager.h"
#include "display_manager.h"
#include "file_manager.h"
#include "SerialDebug.h"
#include "input_manager.h"
#include "Routes/routes.h"
#include "Tasks/init.h"
#include "Tasks/Menus/menus.h"
#include "Tasks/Handler/tasks.h"

Application* app;
HttpClientManager* httpClientManager;

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

  httpClientManager = new HttpClientManager();
  httpClientManager->setDebugEnabled(true);
  httpClientManager->begin();
    
  // Register routes
  Router* router = app->getRouter();
  registerWebRoutes(router);
  registerApiRoutes(router);
  registerWebSocketRoutes(router);
  registerWifiRoutes(router, &wifiManager);
  
  // Run the application (initializes the web server)
  app->run();
  
  // Draw the initial menu
  drawMainMenu();
}

// Empty loop() function as required by Arduino framework
void loop() {
  vTaskDelay(pdMS_TO_TICKS(10));
  vTaskDelete(NULL); // Delete the loop task to free memory
}
