#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>
#include <MVCFramework.h>
#include "Database/Model.h"
#include <WiFi.h>

class WifiConfig : public Model {
private:
    String ssid;
    String password;
    int channel;
    bool hidden;
    int maxConnections;
    bool captivePortal;
    String hostname;

public:
    WifiConfig() 
        : channel(1), hidden(false), maxConnections(4), captivePortal(true) {
        // Generate a default hostname based on MAC address
        uint8_t mac[6];
        WiFi.macAddress(mac);
        hostname = String("PioSystem-") + String(mac[4], HEX) + String(mac[5], HEX);
    }
    
    // Getters
    String getSSID() const { return ssid; }
    String getPassword() const { return password; }
    int getChannel() const { return channel; }
    bool isHidden() const { return hidden; }
    int getMaxConnections() const { return maxConnections; }
    bool isCaptivePortal() const { return captivePortal; }
    String getHostname() const { return hostname; }
    
    // Setters
    void setSSID(const String& newSSID) { ssid = newSSID; }
    void setPassword(const String& newPassword) { password = newPassword; }
    void setChannel(int newChannel) { 
        if (newChannel >= 1 && newChannel <= 13) {
            channel = newChannel;
        }
    }
    void setHidden(bool isHidden) { hidden = isHidden; }
    void setMaxConnections(int maxClients) { 
        if (maxClients > 0 && maxClients <= 8) {
            maxConnections = maxClients;
        }
    }
    void setCaptivePortal(bool enabled) { captivePortal = enabled; }
    void setHostname(const String& name) { hostname = name; }
    
    // JSON serialization for API responses
    void toJson(JsonObject& json) {
        json["ssid"] = ssid;
        // Don't include the password in responses for security
        json["channel"] = channel;
        json["hidden"] = hidden;
        json["max_connections"] = maxConnections;
        json["captive_portal"] = captivePortal;
        json["hostname"] = hostname;
    }
    
    // Create from JSON for API requests
    static WifiConfig fromJson(const JsonObject& json) {
        WifiConfig config;
        
        if (json.containsKey("ssid"))
            config.ssid = json["ssid"].as<String>();
            
        if (json.containsKey("password"))
            config.password = json["password"].as<String>();
            
        if (json.containsKey("channel"))
            config.setChannel(json["channel"].as<int>());
            
        if (json.containsKey("hidden"))
            config.hidden = json["hidden"].as<bool>();
            
        if (json.containsKey("max_connections"))
            config.setMaxConnections(json["max_connections"].as<int>());
            
        if (json.containsKey("captive_portal"))
            config.captivePortal = json["captive_portal"].as<bool>();
            
        if (json.containsKey("hostname"))
            config.hostname = json["hostname"].as<String>();
            
        return config;
    }
    
    // Load configuration from persistent storage
    bool load() {
        // Implementation would use Preferences or FileManager to load saved config
        return false; // Return success/failure
    }
    
    // Save configuration to persistent storage
    bool save() {
        // Implementation would use Preferences or FileManager to save config
        return false; // Return success/failure
    }
};

#endif // WIFI_CONFIG_H
