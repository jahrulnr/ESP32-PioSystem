#ifndef ROUTES_H
#define ROUTES_H

#include <MVCFramework.h>
#include "Routing/Router.h"
#include "../Controllers/AuthController.h"
#include "../Controllers/WifiController.h"
#include "../Controllers/WifiConfigController.h"
#include "../Controllers/ApiController.h"
#include "../Controllers/IoTDeviceController.h"
#include "iot_device_manager.h"

void registerWebRoutes(Router* router);
void registerApiRoutes(Router* router);
void registerWebSocketRoutes(Router* router);
void registerWifiRoutes(Router* router, WiFiManager* wifiManager);
void registerIoTRoutes(Router* router, IoTDeviceManager* iotManager);

#endif
