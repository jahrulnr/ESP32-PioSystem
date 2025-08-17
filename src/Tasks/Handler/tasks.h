#ifndef TASKS_HANDLER_H
#define TASKS_HANDLER_H

#include <Arduino.h>
#include "SerialDebug.h"
#include "input_manager.h"
#include "display_manager.h"
#include "SerialDebug.h"
#include "../init.h"
#include "../Menus/menus.h"

extern InputManager inputManager;

extern TaskHandle_t inputTaskHandle;
extern TaskHandle_t displayUpdateTaskHandle;
extern TaskHandle_t microphoneListenerTaskHandle;
extern TaskHandle_t memoryMonitorTaskHandle;
extern TaskHandle_t autosleepTaskHandle;
extern TaskHandle_t cameraStreamTaskHandle;

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
void microphoneListenerTask(void* param);
void memoryMonitorTask(void* parameter);
void autosleepTask(void* parameter);

// Camera streaming functions
void startCameraStream(const String& deviceId);
void stopCameraStream();
bool isCameraStreamActive();
String getCurrentStreamingDevice();
void cameraStreamTask(void* parameter);
void updateCameraStreamDisplay(bool success, const String& errorMsg = "");
bool triggerManualCapture();

void printMemoryInfo(const char* message);

#endif