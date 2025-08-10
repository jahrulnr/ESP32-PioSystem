#ifndef WIFI_CLIENT_H
#define WIFI_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <MVCFramework.h>
#include "Database/Model.h"

class WifiClient : public Model {
private:
    String macAddress;
    String ipAddress;
    unsigned long connectionTime;
    String hostname;
    int rssi; // Signal strength

public:
    WifiClient() : Model("wifi_networks"), connectionTime(0), rssi(0) {
        // Initialize properties and attributes
        setAttribute("connection_time", "0");
        setAttribute("rssi", "0");
    }
    
    WifiClient(const String& mac, const String& ip) 
        : Model("wifi_networks"), macAddress(mac), ipAddress(ip), connectionTime(millis()), rssi(0) {
        // Initialize attributes with property values
        syncPropertiesToAttributes();
    }
    
    // Synchronize class properties to attributes map
    void syncPropertiesToAttributes() {
        setAttribute("mac_address", macAddress);
        setAttribute("ip_address", ipAddress);
        setAttribute("connection_time", String(connectionTime));
        setAttribute("hostname", hostname);
        setAttribute("rssi", String(rssi));
    }
    
    // Synchronize attributes map to class properties
    void syncAttributesToProperties() {
        macAddress = getAttribute("mac_address");
        ipAddress = getAttribute("ip_address");
        connectionTime = getAttribute("connection_time").toInt();
        hostname = getAttribute("hostname");
        rssi = getAttribute("rssi").toInt();
    }
    
    // Override fill method to update properties from attributes
    void fill(const std::map<String, String>& data);
    
    // Getters
    String getMacAddress() const { return macAddress; }
    String getIpAddress() const { return ipAddress; }
    unsigned long getConnectionTime() const { return connectionTime; }
    
    // Special handling for hostname (SSID)
    String getHostname() const { 
        // First check property
        if (!hostname.isEmpty()) {
            return hostname;
        }
        // Fallback to attribute
        return getAttribute("hostname"); 
    }
    int getRSSI() const { return rssi; }
    
    // Setters
    void setMacAddress(const String& value) { macAddress = value; setAttribute("mac_address", value); }
    void setIpAddress(const String& value) { ipAddress = value; setAttribute("ip_address", value); }
    void setConnectionTime(unsigned long value) { connectionTime = value; setAttribute("connection_time", String(value)); }
    void setHostname(const String& value) { 
        hostname = value; 
        setAttribute("hostname", value);
        // Debug output to verify hostname is set
        Serial.printf("WifiClient::setHostname - Setting hostname to: %s\n", value.c_str());
        Serial.printf("WifiClient::setHostname - Attribute value after set: %s\n", getAttribute("hostname").c_str());
    }
    
    void setRSSI(int strength) { 
        rssi = strength; 
        setAttribute("rssi", String(strength));
    }
    
    // JSON serialization for API responses
    void toJson(JsonObject& json) const {
        json["mac_address"] = macAddress;
        json["ip_address"] = ipAddress;
        json["connection_time"] = connectionTime;
        json["connection_duration"] = millis() - connectionTime;
        json["hostname"] = hostname;
        json["rssi"] = rssi;
    }
    
    // Create from JSON for API requests
    static WifiClient fromJson(const JsonObject& json) {
        WifiClient client;
        
        if (json.containsKey("mac_address"))
            client.macAddress = json["mac_address"].as<String>();
            
        if (json.containsKey("ip_address"))
            client.ipAddress = json["ip_address"].as<String>();
            
        if (json.containsKey("connection_time"))
            client.connectionTime = json["connection_time"].as<unsigned long>();
            
        if (json.containsKey("hostname"))
            client.hostname = json["hostname"].as<String>();
            
        if (json.containsKey("rssi"))
            client.rssi = json["rssi"].as<int>();
            
        return client;
    }
    
    // Static methods for retrieving WiFi clients
    static WifiClient* findById(const String& id);
    static WifiClient* findByHostname(const String& hostname);
    static WifiClient* findByMacAddress(const String& macAddress);
    static std::vector<WifiClient*> all();
    static std::vector<WifiClient*> savedNetworks();
};

#endif // WIFI_CLIENT_H
