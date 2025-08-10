#include "WifiClient.h"

// Custom fill method to update properties from attributes
void WifiClient::fill(const std::map<String, String>& data) {
    // Call parent fill method to update attributes map
    Model::fill(data);
    
    // Direct attribute map debug
    Serial.println("WifiClient::fill - Raw attributes from database:");
    for (const auto& pair : attributes) {
        Serial.printf("  %s: %s\n", pair.first.c_str(), pair.second.c_str());
    }
    
    // Explicit attribute extraction before sync
    if (data.find("hostname") != data.end()) {
        Serial.printf("Found hostname in incoming data: %s\n", data.at("hostname").c_str());
        setAttribute("hostname", data.at("hostname"));
    }
    
    // Update class properties from attributes
    syncAttributesToProperties();
    
    // Debug output after sync
    Serial.println("WifiClient::fill - Properties after sync:");
    Serial.printf("  macAddress: %s\n", macAddress.c_str());
    Serial.printf("  ipAddress: %s\n", ipAddress.c_str());
    Serial.printf("  hostname: %s\n", hostname.c_str());
    Serial.printf("  connectionTime: %lu\n", connectionTime);
    Serial.printf("  rssi: %d\n", rssi);
}

// Static methods for retrieving WiFi clients
WifiClient* WifiClient::findById(const String& id) {
    if (!database) {
        Serial.println("ERROR: Database is null in findById()");
        return nullptr;
    }
    
    // Create the table if it doesn't exist
    if (!database->tableExists("wifi_networks")) {
        Serial.println("Table 'wifi_networks' does not exist in findById(). Creating it...");
        std::vector<String> columns = {
            "mac_address", "ip_address", "hostname", "connection_time", "rssi", 
            "password", "is_saved_network"
        };
        
        if (!database->createTable("wifi_networks", columns)) {
            Serial.println("ERROR: Failed to create wifi_networks table in findById()");
            return nullptr;
        }
        Serial.println("Successfully created wifi_networks table");
    }
    
    Serial.printf("Looking for network with ID: %s\n", id.c_str());
    
    auto record = database->find("wifi_networks", id);
    if (record.empty()) {
        Serial.println("No network found with this ID");
        return nullptr;
    }
    
    Serial.println("Network found with matching ID:");
    for (const auto& pair : record) {
        Serial.printf("  %s: %s\n", pair.first.c_str(), pair.second.c_str());
    }
    
    WifiClient* client = new WifiClient();
    client->fill(record);
    
    // Map database fields to model properties
    if (record.find("mac_address") != record.end()) {
        client->setMacAddress(record.at("mac_address"));
    }
    
    if (record.find("ip_address") != record.end()) {
        client->setIpAddress(record.at("ip_address"));
    }
    
    if (record.find("hostname") != record.end()) {
        client->setHostname(record.at("hostname"));
    }
    
    if (record.find("connection_time") != record.end()) {
        client->setConnectionTime(record.at("connection_time").toInt());
    }
    
    if (record.find("rssi") != record.end()) {
        client->setRSSI(record.at("rssi").toInt());
    }
    
    client->syncOriginal();
    client->exists = true;
    
    return client;
}

WifiClient* WifiClient::findByHostname(const String& hostname) {
    if (!database) {
        Serial.println("ERROR: Database is null in findByHostname()");
        return nullptr;
    }
    
    // Create the table if it doesn't exist
    if (!database->tableExists("wifi_networks")) {
        Serial.println("Table 'wifi_networks' does not exist in findByHostname(). Creating it...");
        std::vector<String> columns = {
            "mac_address", "ip_address", "hostname", "connection_time", "rssi", 
            "password", "is_saved_network"
        };
        
        if (!database->createTable("wifi_networks", columns)) {
            Serial.println("ERROR: Failed to create wifi_networks table in findByHostname()");
            return nullptr;
        }
        Serial.println("Successfully created wifi_networks table");
    }
    
    Serial.printf("Looking for network with hostname: %s\n", hostname.c_str());
    
    std::map<String, String> where;
    where["hostname"] = hostname;
    where["is_saved_network"] = "1"; // Only saved networks
    
    auto record = database->findWhere("wifi_networks", where);
    if (record.empty()) {
        Serial.println("No network found with this hostname");
        return nullptr;
    }
    
    Serial.println("Network found with matching hostname:");
    for (const auto& pair : record) {
        Serial.printf("  %s: %s\n", pair.first.c_str(), pair.second.c_str());
    }
    
    WifiClient* client = new WifiClient();
    client->fill(record);
    
    // Map database fields to model properties
    if (record.find("mac_address") != record.end()) {
        client->setMacAddress(record.at("mac_address"));
    }
    
    if (record.find("ip_address") != record.end()) {
        client->setIpAddress(record.at("ip_address"));
    }
    
    if (record.find("hostname") != record.end()) {
        client->setHostname(record.at("hostname"));
    }
    
    if (record.find("connection_time") != record.end()) {
        client->setConnectionTime(record.at("connection_time").toInt());
    }
    
    if (record.find("rssi") != record.end()) {
        client->setRSSI(record.at("rssi").toInt());
    }
    
    client->syncOriginal();
    client->exists = true;
    
    return client;
}

WifiClient* WifiClient::findByMacAddress(const String& macAddress) {
    if (!database) {
        Serial.println("ERROR: Database is null in findByMacAddress()");
        return nullptr;
    }
    
    // Create the table if it doesn't exist
    if (!database->tableExists("wifi_networks")) {
        Serial.println("Table 'wifi_networks' does not exist in findByMacAddress(). Creating it...");
        std::vector<String> columns = {
            "mac_address", "ip_address", "hostname", "connection_time", "rssi", 
            "password", "is_saved_network"
        };
        
        if (!database->createTable("wifi_networks", columns)) {
            Serial.println("ERROR: Failed to create wifi_networks table in findByMacAddress()");
            return nullptr;
        }
        Serial.println("Successfully created wifi_networks table");
    }
    
    Serial.printf("Looking for network with MAC address: %s\n", macAddress.c_str());
    
    std::map<String, String> where;
    where["mac_address"] = macAddress;
    
    auto record = database->findWhere("wifi_networks", where);
    if (record.empty()) {
        Serial.println("No network found with this MAC address");
        return nullptr;
    }
    
    Serial.println("Network found with matching MAC address:");
    for (const auto& pair : record) {
        Serial.printf("  %s: %s\n", pair.first.c_str(), pair.second.c_str());
    }
    
    WifiClient* client = new WifiClient();
    client->fill(record);
    
    // Map database fields to model properties
    if (record.find("mac_address") != record.end()) {
        client->setMacAddress(record.at("mac_address"));
    }
    
    if (record.find("ip_address") != record.end()) {
        client->setIpAddress(record.at("ip_address"));
    }
    
    if (record.find("hostname") != record.end()) {
        client->setHostname(record.at("hostname"));
    }
    
    if (record.find("connection_time") != record.end()) {
        client->setConnectionTime(record.at("connection_time").toInt());
    }
    
    if (record.find("rssi") != record.end()) {
        client->setRSSI(record.at("rssi").toInt());
    }
    
    client->syncOriginal();
    client->exists = true;
    
    return client;
}

std::vector<WifiClient*> WifiClient::all() {
    if (!database) {
        Serial.println("ERROR: Database is null in all()");
        return std::vector<WifiClient*>();
    }
    
    // Create the table if it doesn't exist
    if (!database->tableExists("wifi_networks")) {
        Serial.println("Table 'wifi_networks' does not exist in all(). Creating it...");
        std::vector<String> columns = {
            "mac_address", "ip_address", "hostname", "connection_time", "rssi", 
            "password", "is_saved_network"
        };
        
        if (!database->createTable("wifi_networks", columns)) {
            Serial.println("ERROR: Failed to create wifi_networks table in all()");
            return std::vector<WifiClient*>();
        }
        Serial.println("Successfully created wifi_networks table");
    }
    
    Serial.println("Retrieving all WiFi networks");
    
    auto records = database->select("wifi_networks");
    Serial.printf("Found %d network records\n", records.size());
    
    std::vector<WifiClient*> clients;
    
    for (const auto& record : records) {
        WifiClient* client = new WifiClient();
        client->fill(record);
        
        // Map database fields to model properties
        if (record.find("mac_address") != record.end()) {
            client->setMacAddress(record.at("mac_address"));
        }
        
        if (record.find("ip_address") != record.end()) {
            client->setIpAddress(record.at("ip_address"));
        }
        
        if (record.find("hostname") != record.end()) {
            client->setHostname(record.at("hostname"));
        }
        
        if (record.find("connection_time") != record.end()) {
            client->setConnectionTime(record.at("connection_time").toInt());
        }
        
        if (record.find("rssi") != record.end()) {
            client->setRSSI(record.at("rssi").toInt());
        }
        
        client->syncOriginal();
        client->exists = true;
        clients.push_back(client);
    }
    
    return clients;
}

std::vector<WifiClient*> WifiClient::savedNetworks() {
    if (!database) {
        Serial.println("ERROR: Database is null in savedNetworks()");
        return std::vector<WifiClient*>();
    }
    
    Serial.println("Looking for saved networks...");
    // Check if table exists
    if (!database->tableExists("wifi_networks")) {
        Serial.println("Table 'wifi_networks' does not exist. Creating it...");
        // Create the table with appropriate columns
        std::vector<String> columns = {
            "mac_address", "ip_address", "hostname", "connection_time", "rssi", 
            "password", "is_saved_network"
        };
        
        if (database->createTable("wifi_networks", columns)) {
            Serial.println("Successfully created wifi_networks table");
        } else {
            Serial.println("ERROR: Failed to create wifi_networks table");
            return std::vector<WifiClient*>();
        }
    }
    
    // Print table columns
    std::vector<String> columns = database->getTableColumns("wifi_networks");
    Serial.println("Table columns:");
    for (const auto& col : columns) {
        Serial.print("  ");
        Serial.println(col);
    }
    
    std::map<String, String> where;
    where["is_saved_network"] = "1";
    
    Serial.println("Querying database for saved networks...");
    auto records = database->select("wifi_networks", where);
    Serial.printf("Found %d records matching the query\n", records.size());
    
    std::vector<WifiClient*> clients;
    
    for (const auto& record : records) {
        // Print record for debugging
        Serial.println("\nRecord found in savedNetworks():");
        for (const auto& pair : record) {
            Serial.printf("  %s: %s\n", pair.first.c_str(), pair.second.c_str());
        }
        
        WifiClient* client = new WifiClient();
        client->fill(record);
        
        // Double-check that hostname is correctly set after fill
        Serial.println("After fill:");
        Serial.printf("  Hostname property: %s\n", client->getHostname().c_str());
        Serial.printf("  Hostname attribute: %s\n", client->getAttribute("hostname").c_str());
        
        client->syncOriginal();
        client->exists = true;
        clients.push_back(client);
    }
    
    Serial.printf("Returning %d client objects\n", clients.size());
    return clients;
}
