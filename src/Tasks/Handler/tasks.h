#ifndef TASKS_HANDLER_H
#define TASKS_HANDLER_H

#include <Arduino.h>
#include "SerialDebug.h"
#include "input_manager.h"
#include "display_manager.h"
#include "../init.h"
#include "../Menus/menus.h"

extern InputManager inputManager;

extern TaskHandle_t inputTaskHandle;
extern TaskHandle_t displayUpdateTaskHandle;
extern TaskHandle_t memoryMonitorTaskHandle;
extern TaskHandle_t autosleepTaskHandle;

// Forward declaration for IoTDeviceManager
class IoTDeviceManager;
extern IoTDeviceManager* iotDeviceManager;

void setupTasks();

void handleButtonEvents();
void handleMenuNavigation();
void forceSleep();
void setSleepTimeout(unsigned long timeoutMs);

void inputTask(void* parameter);
void connectToWiFi(void* param);
void displayUpdateTask(void* parameter);
void memoryMonitorTask(void* parameter);
void autosleepTask(void* parameter);

void printMemoryInfo(const char* message);

#endif