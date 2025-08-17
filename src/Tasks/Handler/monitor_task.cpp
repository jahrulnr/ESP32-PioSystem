#include "tasks.h"

TaskHandle_t memoryMonitorTaskHandle = NULL;

void memoryMonitorTask(void* parameter) {
  // Initial memory status after task creation
  vTaskDelay(pdMS_TO_TICKS(5000)); // Wait for system to stabilize
  printMemoryInfo("Initial memory status after FreeRTOS setup");
  
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t monitorFrequency = pdMS_TO_TICKS(30000); // 30 seconds
  
  for (;;) {
    // Wait for the next cycle
    vTaskDelayUntil(&lastWakeTime, monitorFrequency);
    
    // Print memory information
    printMemoryInfo("Periodic memory check");
    
    // Print additional task information
    DEBUG_PRINTF("Current task count: %d\n", uxTaskGetNumberOfTasks());
    DEBUG_PRINTF("Task high water mark: %d\n", uxTaskGetStackHighWaterMark(NULL));
  }
}

void printMemoryInfo(const char* message) {
  DEBUG_PRINTLN("\n====================================");
  DEBUG_PRINTLN(message);
  DEBUG_PRINTLN("------------------------------------");
  DEBUG_PRINTF("Total heap: %d\n", ESP.getHeapSize());
  DEBUG_PRINTF("Free heap: %d\n", ESP.getFreeHeap());
  DEBUG_PRINTF("Minimum free heap: %d\n", ESP.getMinFreeHeap());
  DEBUG_PRINTF("Heap fragmentation: %d%%\n", 100 - (ESP.getMaxAllocHeap() * 100) / ESP.getFreeHeap());
  DEBUG_PRINTF("Max allocatable: %d\n", ESP.getMaxAllocHeap());
  
  #ifdef BOARD_HAS_PSRAM
  DEBUG_PRINTF("Total PSRAM: %d\n", ESP.getPsramSize());
  DEBUG_PRINTF("Free PSRAM: %d\n", ESP.getFreePsram());
  DEBUG_PRINTF("Minimum free PSRAM: %d\n", ESP.getMinFreePsram());
  DEBUG_PRINTF("Max allocatable PSRAM: %d\n", ESP.getMaxAllocPsram());
  #endif
  
  DEBUG_PRINTLN("====================================\n");
}