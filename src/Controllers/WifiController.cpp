#include "WifiController.h"
#include <ArduinoJson.h>

Response WifiController::status(Request& request) {
    JsonDocument doc;
    
    // Get WiFi status information
    WiFiMode_t mode = wifiManager->getMode();
    wl_status_t status = wifiManager->getStatus();
    
    // Add mode information
    switch (mode) {
        case WIFI_OFF: doc["mode"] = "off"; break;
        case WIFI_STA: doc["mode"] = "station"; break;
        case WIFI_AP: doc["mode"] = "access_point"; break;
        case WIFI_AP_STA: doc["mode"] = "bridge"; break;
        default: doc["mode"] = "unknown"; break;
    }
    
    // Add AP information if in AP or AP+STA mode
    if (mode == WIFI_AP || mode == WIFI_AP_STA) {
        doc["ap"]["ssid"] = WiFi.softAPSSID();
        doc["ap"]["ip"] = WiFi.softAPIP().toString();
        doc["ap"]["clients"] = WiFi.softAPgetStationNum();
    }
    
    // Add station information if in STA or AP+STA mode
    if ((mode == WIFI_STA || mode == WIFI_AP_STA) && status == WL_CONNECTED) {
        doc["station"]["ssid"] = WiFi.SSID();
        doc["station"]["ip"] = WiFi.localIP().toString();
        doc["station"]["rssi"] = WiFi.RSSI();
    }
    
    return json(request.getServerRequest(), doc);
}

Response WifiController::scan(Request& request) {
    // Start scan
    int networkCount = wifiManager->scanNetworks();
    
    if (networkCount <= 0) {
        // No networks found or error occurred
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = (networkCount == 0) ? "No networks found" : "Scan failed";
        return json(request.getServerRequest(), errorDoc);
    }
    
    // Get network list
    std::vector<NetworkInfo> networks = wifiManager->getNetworkList();
    
    // Create JSON response
    JsonDocument doc;  // Adjust size based on expected number of networks
    doc["status"] = "success";
    doc["count"] = networks.size();
    
    JsonArray networksArray = doc.createNestedArray("networks");
    
    for (const auto& network : networks) {
        JsonObject netObj = networksArray.createNestedObject();
        netObj["ssid"] = network.ssid;
        netObj["bssid"] = network.bssid;
        netObj["rssi"] = network.rssi;
        netObj["encryption"] = network.encryptionType;
        netObj["connected"] = network.isConnected;
    }
    
    return json(request.getServerRequest(), doc);
}

Response WifiController::startAP(Request& request) {
    // Extract parameters from request
    JsonDocument doc;
    deserializeJson(doc, request.getBody());
    
    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    int channel = doc["channel"] | 1;
    bool hidden = doc["hidden"] | false;
    int maxConnections = doc["maxConnections"] | 4;
    
    // Validate SSID
    if (ssid.length() == 0) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "SSID is required";
        return json(request.getServerRequest(), errorDoc);
    }
    
    // Start Hotspot
    bool success = wifiManager->startAccessPoint(ssid, password, channel, hidden, maxConnections);
    
    if (success) {
        JsonDocument responseDoc;
        responseDoc["status"] = "success";
        responseDoc["message"] = "Access point started successfully";
        responseDoc["ssid"] = WiFi.softAPSSID();
        responseDoc["ip"] = WiFi.softAPIP().toString();
        return json(request.getServerRequest(), responseDoc);
    } else {
        return error(request.getServerRequest(), "Failed to start access point");
    }
}

Response WifiController::updateAP(Request& request) {
    // First, stop the current AP
    wifiManager->stopAccessPoint();
    
    // Then start a new one with updated parameters
    return startAP(request);
}

Response WifiController::stopAP(Request& request) {
    bool success = wifiManager->stopAccessPoint();
    
    if (success) {
        JsonDocument doc;
        doc["status"] = "success";
        doc["message"] = "Access point stopped successfully";
        return json(request.getServerRequest(), doc);
    } else {
        return error(request.getServerRequest(), "Failed to stop access point");
    }
}

Response WifiController::getClients(Request& request) {
    JsonDocument doc;
    doc["status"] = "success";
    
    // Get client information from WiFi manager
    std::vector<ClientInfo> clients = wifiManager->getConnectedClients();
    doc["count"] = clients.size();
    
    JsonArray clientsArray = doc.createNestedArray("clients");
    
    for (const auto& client : clients) {
        JsonObject clientObj = clientsArray.createNestedObject();
        clientObj["mac_address"] = client.macAddress;
        clientObj["ip_address"] = client.ipAddress;
        clientObj["hostname"] = client.hostname;
        clientObj["rssi"] = client.rssi;
        clientObj["connection_time"] = client.connectionTime;
        clientObj["connection_duration"] = millis() - client.connectionTime;
    }
    
    return json(request.getServerRequest(), doc);
}

Response WifiController::disconnectClient(Request& request) {
    // Extract client MAC address from parameters
    String clientId = request.route("id");
    
    // Convert string MAC address to uint8_t array
    uint8_t mac[6];
    int values[6];
    
    if (sscanf(clientId.c_str(), "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) == 6) {
        for (int i = 0; i < 6; i++) {
            mac[i] = static_cast<uint8_t>(values[i]);
        }
        
        // Attempt to disconnect the client
        bool success = WiFi.softAPdisconnect(mac);
        
        if (success) {
            JsonDocument doc;
            doc["status"] = "success";
            doc["message"] = "Client disconnected successfully";
            return json(request.getServerRequest(), doc);
        }
    }
    
    return error(request.getServerRequest(), "Failed to disconnect client");
}
