#include "tasks.h"

TaskHandle_t autosleepTaskHandle = NULL;

void autosleepTask(void* parameter) {
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t checkFrequency = pdMS_TO_TICKS(1000); // Check every 1 second
  
  DEBUG_PRINTLN("Autosleep task started");
  
  for (;;) {
    // Wait for the next cycle
    vTaskDelayUntil(&lastWakeTime, checkFrequency);
    
    // Check if display manager is available and screen timeout needs to be checked
    DisplayManager* display = DisplayManager::getInstance();
    if (display != nullptr) {
      // Only acquire mutex if screen is on to avoid unnecessary blocking
      if (display->isScreenOn()) {
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
          // Check sleep timeout using DisplayManager's method
          display->checkSleepTimeout();
          xSemaphoreGive(displayMutex);
        }
      }
    }
  }
}
