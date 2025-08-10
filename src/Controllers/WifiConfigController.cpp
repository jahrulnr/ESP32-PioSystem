#include "WifiConfigController.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "../Models/WifiClient.h"

Response WifiConfigController::getConfigurations(Request& request) {
    JsonDocument doc;
    doc["status"] = "success";
    doc["networks"] = getSavedNetworks();
    
    // Add current connection info if connected
    if (WiFi.status() == WL_CONNECTED) {
        JsonObject currentConnection = doc["current_connection"].to<JsonObject>();
        currentConnection["ssid"] = WiFi.SSID();
        currentConnection["ip"] = WiFi.localIP().toString();
        currentConnection["rssi"] = WiFi.RSSI();
        currentConnection["mac"] = WiFi.macAddress();
    }
    
    return json(request.getServerRequest(), doc);
}

Response WifiConfigController::saveConfiguration(Request& request) {
    JsonDocument doc;
    deserializeJson(doc, request.getBody());
    
    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    String id = doc["id"] | "";
    
    if (ssid.isEmpty()) {
        JsonDocument errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "SSID is required";
        return json(request.getServerRequest(), errorDoc);
    }
    
    // Save network to model
    bool success = saveNetworkToModel(ssid, password, id);
    
    if (success) {
        JsonDocument successDoc;
        successDoc["status"] = "success";
        successDoc["message"] = "WiFi configuration saved successfully";
        return json(request.getServerRequest(), successDoc);
    } else {
        return error(request.getServerRequest(), "Failed to save WiFi configuration");
    }
}

Response WifiConfigController::deleteConfiguration(Request& request) {
    String id = request.route("id");
    
    if (id.isEmpty()) {
        return error(request.getServerRequest(), "Network ID is required");
    }
    
    bool success = deleteNetworkFromModel(id);
    
    if (success) {
        JsonDocument successDoc;
        successDoc["status"] = "success";
        successDoc["message"] = "WiFi configuration deleted successfully";
        return json(request.getServerRequest(), successDoc);
    } else {
        return error(request.getServerRequest(), "Failed to delete WiFi configuration");
    }
}

Response WifiConfigController::connectToSaved(Request& request) {
    String id = request.route("id");
    
    if (id.isEmpty()) {
        return error(request.getServerRequest(), "Network ID is required");
    }
    
    // Find the network model by ID
    WifiClient* client = WifiClient::findById(id);
    if (client == nullptr) {
        return error(request.getServerRequest(), "Network configuration not found");
    }
    
    // Get SSID and password
    String ssid = client->getHostname();  // SSID is stored as hostname
    String password = client->getAttribute("password");
    
    // Clean up
    delete client;
    
    if (ssid.isEmpty()) {
        return error(request.getServerRequest(), "Network configuration invalid (missing SSID)");
    }
    
    // Try to connect
    bool success = wifiManager->connectToNetwork(ssid, password);
    
    if (success) {
        JsonDocument successDoc;
        successDoc["status"] = "success";
        successDoc["message"] = "Connected to " + ssid + " successfully";
        successDoc["connection"] = JsonObject();
        successDoc["connection"]["ssid"] = ssid;
        successDoc["connection"]["ip"] = WiFi.localIP().toString();
        return json(request.getServerRequest(), successDoc);
    } else {
        return error(request.getServerRequest(), "Failed to connect to " + ssid);
    }
}

Response WifiConfigController::scanNetworks(Request& request) {
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
    JsonDocument doc;
    doc["status"] = "success";
    doc["count"] = networks.size();
    
    JsonArray networksArray = doc["networks"].to<JsonArray>();
    
    // Get all saved networks using the WifiClient model's static method
    std::vector<WifiClient*> savedNetworks = WifiClient::savedNetworks();
    std::map<String, String> savedNetworkMap; // Map of SSID to ID
    
    // Create a map for faster lookup when checking if networks are saved
    for (WifiClient* client : savedNetworks) {
        savedNetworkMap[client->getHostname()] = client->getAttribute("id");
        delete client; // Clean up as we go
    }
    
    // Process the network list
    for (const auto& network : networks) {
        JsonObject netObj = networksArray.add<JsonObject>();
        netObj["ssid"] = network.ssid;
        netObj["bssid"] = network.bssid;
        netObj["rssi"] = network.rssi;
        netObj["encryption"] = network.encryptionType;
        netObj["connected"] = network.isConnected;
        
        // Check if this network is saved - O(1) lookup in the map
        auto it = savedNetworkMap.find(network.ssid);
        bool isSaved = (it != savedNetworkMap.end());
        
        netObj["saved"] = isSaved;
        if (isSaved) {
            netObj["id"] = it->second;  // Use the ID from our map
        }
    }
    
    return json(request.getServerRequest(), doc);
}

Response WifiConfigController::connectToNetwork(Request& request) {
    JsonDocument doc;
    deserializeJson(doc, request.getBody());
    
    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    bool saveNetwork = doc["save"] | false;
    
    if (ssid.isEmpty()) {
        return error(request.getServerRequest(), "SSID is required");
    }
    
    // Try to connect
    bool success = wifiManager->connectToNetwork(ssid, password);
    
    if (success) {
        // Save network if requested
        if (saveNetwork) {
            saveNetworkToModel(ssid, password);
        }
        
        JsonDocument successDoc;
        successDoc["status"] = "success";
        successDoc["message"] = "Connected to " + ssid + " successfully";
        successDoc["connection"] = JsonObject();
        successDoc["connection"]["ssid"] = ssid;
        successDoc["connection"]["ip"] = WiFi.localIP().toString();
        return json(request.getServerRequest(), successDoc);
    } else {
        return error(request.getServerRequest(), "Failed to connect to " + ssid);
    }
}

bool WifiConfigController::saveNetworkToModel(const String& ssid, const String& password, const String& id) {
    String networkId = id;
    
    // If no ID provided, generate one based on the SSID
    if (networkId.isEmpty()) {
        networkId = String(random(10000, 99999));
    }
    
    // Create a WifiClient instance - for saved networks we use a pseudo MAC address
    // based on the network ID to ensure uniqueness
    WifiClient* client = new WifiClient();
    
    // We need to find if this network is already saved
    // First, try to find by ID
    bool networkExists = false;
    if (!networkId.isEmpty()) {
        WifiClient* existingModel = WifiClient::findById(networkId);
        if (existingModel != nullptr) {
            // Copy any existing attributes we want to preserve
            client->fill(existingModel->toMap());
            delete existingModel;
            networkExists = true;
        }
    }
    
    // If not found by ID, try to find by SSID (hostname)
    if (!networkExists) {
        WifiClient* existingModel = WifiClient::findByHostname(ssid);
        if (existingModel != nullptr) {
            // Copy any existing attributes we want to preserve
            client->fill(existingModel->toMap());
            delete existingModel;
            networkExists = true;
        }
    }
    
    // Set properties
    client->setAttribute("id", networkId);
    
    // Set hostname explicitly in both property and attribute
    client->setHostname(ssid); 
    client->setAttribute("password", password);
    client->setAttribute("is_saved_network", "1");  // Mark as a saved network
    
    // Generate a pseudo MAC address if we don't have one
    if (client->getMacAddress().isEmpty()) {
        // Create a MAC address based on the SSID and ID to ensure uniqueness
        String pseudoMac = "02:";  // Locally administered MAC
        for (int i = 0; i < 5; i++) {
            int val = random(0, 256);
            pseudoMac += String(val, HEX);
            if (i < 4) pseudoMac += ":";
        }
        client->setMacAddress(pseudoMac);
    }
    
    // Set connection time if not set
    if (client->getConnectionTime() == 0) {
        client->setConnectionTime(millis());
    }
    
    // Make sure we map the class properties to the database columns
    client->setAttribute("mac_address", client->getMacAddress());
    client->setAttribute("ip_address", client->getIpAddress());
    client->setAttribute("hostname", client->getHostname());
    client->setAttribute("connection_time", String(client->getConnectionTime()));
    client->setAttribute("rssi", String(client->getRSSI()));
    
    // Save the model
    bool success = client->save();
    
    // Clean up
    delete client;
    
    return success;
}

bool WifiConfigController::deleteNetworkFromModel(const String& id) {
    // Try to find by ID first
    WifiClient* client = WifiClient::findById(id);
    
    // If not found by ID, try to find by SSID (hostname)
    if (client == nullptr) {
        client = WifiClient::findByHostname(id);
        
        if (client == nullptr) {
            return false;
        }
    }
    
    // Delete the model
    bool success = client->delete_();
    
    // Clean up
    delete client;
    
    return success;
}

JsonDocument WifiConfigController::getSavedNetworks() {
    // Create a document to hold the array to ensure proper memory management
    JsonDocument doc;  // Local document, not static
    JsonArray networksArray = doc.to<JsonArray>();
    
    // Get all saved networks using the WifiClient model's static method
    std::vector<WifiClient*> savedNetworks = WifiClient::savedNetworks();
    
    // Convert to JSON objects
    for (WifiClient* client : savedNetworks) {
        // Only add networks that have a valid hostname/SSID
        if (!client->getHostname().isEmpty()) {
            JsonObject networkObj = networksArray.add<JsonObject>();
            
            for (const auto& pair : client->toMap()) {
                Serial.printf("  %s: %s\n", pair.first.c_str(), pair.second.c_str());
            }
            
            // Map WifiClient fields to network config fields using getters
            networkObj["id"] = client->getAttribute("id");
            networkObj["ssid"] = client->getHostname();  // Using the getter for hostname
            
            networkObj["password"] = client->getAttribute("password");
            
            // Add optional fields if available
            if (client->getAttribute("connected") == "1") {
                networkObj["connected"] = true;
            } else {
                networkObj["connected"] = false;
            }
            
            if (client->getRSSI() != 0) {
                networkObj["signal"] = client->getRSSI();  // Using the getter for RSSI
            }
        }
        
        // Clean up
        delete client;
    }
    
    return doc;
}

Response WifiConfigController::cleanUpNetworks(Request& request) {
    int removedCount = removeInvalidNetworks();
    
    JsonDocument response;
    response["status"] = "success";
    response["removed_count"] = removedCount;
    response["message"] = String("Removed ") + removedCount + " invalid network entries";
    
    // Get the current networks after cleanup
    JsonDocument networks = getSavedNetworks();
    response["networks_count"] = networks.size();
    
    return json(request.getServerRequest(), response);
}

int WifiConfigController::removeInvalidNetworks() {
    // Get all saved networks
    std::vector<WifiClient*> savedNetworks = WifiClient::savedNetworks();
    int removedCount = 0;
    
    for (WifiClient* client : savedNetworks) {
        bool isValid = true;
        String reason;
        
        // Check for empty hostname
        if (client->getHostname().isEmpty()) {
            isValid = false;
            reason = "Empty hostname";
        }
        
        // If not valid, delete it
        if (!isValid) {
            String id = client->getAttribute("id");
            Serial.printf("Removing invalid network ID %s: %s\n", 
                         id.c_str(), reason.c_str());
            
            if (client->delete_()) {
                removedCount++;
                Serial.println("  Successfully removed");
            } else {
                Serial.println("  Failed to remove");
            }
        }
        
        // Clean up
        delete client;
    }
    
    return removedCount;
}
