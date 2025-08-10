#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include <Arduino.h>
#include <MVCFramework.h>
#include "wifi_manager.h"

class WifiController : public Controller {
private:
    WiFiManager* wifiManager;

public:
    WifiController(WiFiManager* manager) : wifiManager(manager) {}

    // GET /api/wifi/status - Get current WiFi status
    Response status(Request& request);
    
    // GET /api/wifi/scan - Scan for available networks
    Response scan(Request& request);
    
    // POST /api/wifi/ap - Start access point with provided config
    Response startAP(Request& request);
    
    // PUT /api/wifi/ap - Update access point configuration
    Response updateAP(Request& request);
    
    // DELETE /api/wifi/ap - Stop access point
    Response stopAP(Request& request);
    
    // GET /api/wifi/clients - Get list of connected clients
    Response getClients(Request& request);
    
    // POST /api/wifi/client/:id/disconnect - Disconnect a specific client
    Response disconnectClient(Request& request);
};

#endif // WIFI_CONTROLLER_H
