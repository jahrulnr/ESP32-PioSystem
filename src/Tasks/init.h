#ifndef TASKS_INIT_H
#define TASKS_INIT_H

#include <MVCFramework.h>
#include <ESPAsyncWebServer.h>
#include "display_manager.h"
#include "wifi_manager.h"
#include "httpclient.h"
#include "iot_device_manager.h"
#include "Handler/tasks.h"
#include "Menus/menus.h"

extern TFT_eSPI tft;
extern DisplayManager displayManager;
extern WiFiManager wifiManager;
extern AsyncWebServer server;

extern SemaphoreHandle_t displayMutex;

void initialize();

#endif