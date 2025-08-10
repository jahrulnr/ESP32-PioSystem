#include "tasks.h"

void setupTasks() {
  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(
    inputTask,                // Task function
    "InputTask",              // Task name
    4096,                     // Stack size (bytes)
    NULL,                     // Parameters
    2,                        // Priority (1-24, higher number = higher priority)
    &inputTaskHandle,         // Task handle
    1                         // Core (0 or 1)
  );
  
  xTaskCreatePinnedToCore(
    displayUpdateTask,        // Task function
    "DisplayTask",            // Task name
    4096,                     // Stack size (bytes)
    NULL,                     // Parameters
    1,                        // Priority (1-24, higher number = higher priority)
    &displayUpdateTaskHandle, // Task handle
    1                         // Core (0 or 1)
  );
  
  xTaskCreatePinnedToCore(
    memoryMonitorTask,        // Task function
    "MemoryTask",             // Task name
    4096,                     // Stack size (bytes)
    NULL,                     // Parameters
    1,                        // Priority (1-24, higher number = higher priority)
    &memoryMonitorTaskHandle, // Task handle
    0                         // Core (0 or 1)
  );
  
  xTaskCreatePinnedToCore(
    connectToWiFi,            // Task function
    "connectToWiFiTask",      // Task name
    4096,                     // Stack size (bytes)
    NULL,                     // Parameters
    1,                        // Priority (1-24, higher number = higher priority)
    NULL,                     // Task handle
    0                         // Core (0 or 1)
  );
  
  xTaskCreatePinnedToCore(
    autosleepTask,            // Task function
    "AutosleepTask",          // Task name
    4096,                     // Stack size (bytes)
    NULL,                     // Parameters
    1,                        // Priority (1-24, higher number = higher priority)
    &autosleepTaskHandle,     // Task handle
    0                         // Core (0 or 1)
  );
}