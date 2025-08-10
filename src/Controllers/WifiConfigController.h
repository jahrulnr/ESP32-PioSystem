#ifndef WIFI_CONFIG_CONTROLLER_H
#define WIFI_CONFIG_CONTROLLER_H

#include <Arduino.h>
#include <MVCFramework.h>
#include "wifi_manager.h"
#include "../Models/WifiClient.h"
#include "Database/CsvDatabase.h"

class WifiConfigController : public Controller {
private:
    WiFiManager* wifiManager;

public:
    WifiConfigController(WiFiManager* manager) : wifiManager(manager) {
        // Set up the table for WifiClient models to store network configurations
        // Note: We don't need to create a CsvDatabase instance since
        // the Model class already has a static database instance
        std::vector<String> columns = {"id", "mac_address", "ip_address", "connection_time", 
                                      "hostname", "rssi", "password", "is_saved_network"};
        Model::createTable("wifi_networks", columns);
    }

    ~WifiConfigController() {
        // No need to clean up the database as it's handled by the Model class
    }

    // GET /api/wifi/config - Get saved WiFi configurations
    Response getConfigurations(Request& request);
    
    // POST /api/wifi/config - Save a new WiFi configuration
    Response saveConfiguration(Request& request);
    
    // DELETE /api/wifi/config/:id - Delete a saved WiFi configuration
    Response deleteConfiguration(Request& request);
    
    // POST /api/wifi/config/:id/connect - Connect to a saved configuration
    Response connectToSaved(Request& request);
    
    // POST /api/wifi/scan - Scan for available networks
    Response scanNetworks(Request& request);
    
    // POST /api/wifi/connect - Connect to a specific network
    Response connectToNetwork(Request& request);
    
    // Method to clean up invalid entries in the database
    Response cleanUpNetworks(Request& request);

private:
    // Helper methods
    bool saveNetworkToModel(const String& ssid, const String& password, const String& id = "");
    bool deleteNetworkFromModel(const String& id);
    JsonDocument getSavedNetworks();
    int removeInvalidNetworks();
};

#endif // WIFI_CONFIG_CONTROLLER_H
