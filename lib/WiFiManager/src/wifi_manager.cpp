#include "wifi_manager.h"
#include <esp_wifi.h>
#include <esp_netif.h>
#include <lwip/dns.h>
#include <mdns.h>
#include <WiFi.h>

WiFiManager::WiFiManager(const String& apSSID, const String& apPassword) :
    _apSSID(apSSID),
    _apPassword(apPassword),
    _currentMode(WIFI_OFF),
    _isBridgeModeEnabled(false),
    _dnsServer(nullptr),
    _eventCallback(nullptr)
{
    // Initialize mDNS for hostname resolution
    mdns_init();
}

WiFiManager::~WiFiManager() {
    if (_dnsServer) {
        _dnsServer->stop();
        delete _dnsServer;
        _dnsServer = nullptr;
    }
    
    // Disconnect and turn off WiFi
    WiFi.disconnect(true);
    begin(WIFI_OFF);
}

bool WiFiManager::begin(WiFiMode_t mode) {
    // Initialize WiFi with the specified mode
    if (!WiFi.mode(mode)) {
        Serial.println("Failed to set WiFi mode");
        return false;
    }
    
    _currentMode = mode;
    
    // Register event handlers
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        this->handleEvent(event, info);
    });
    
    // Initialize DNS server for captive portal if in AP mode
    if (mode == WIFI_AP || mode == WIFI_AP_STA) {
        if (_dnsServer == nullptr) {
            _dnsServer = new DNSServer();
        }
    }
    
    return true;
}

int16_t WiFiManager::scanNetworks(bool async) {
    return WiFi.scanNetworks(async);
}

int16_t WiFiManager::scanComplete() {
    return WiFi.scanComplete();
}

std::vector<NetworkInfo> WiFiManager::getNetworkList() {
    std::vector<NetworkInfo> networks;
    
    int numNetworks = WiFi.scanComplete();
    if (numNetworks <= 0) {
        return networks;
    }
    
    // Get the current connection info to mark connected network
    String currentSSID = WiFi.SSID();
    
    for (int i = 0; i < numNetworks; i++) {
        NetworkInfo network;
        network.ssid = WiFi.SSID(i);
        network.bssid = WiFi.BSSIDstr(i);
        network.rssi = WiFi.RSSI(i);
        network.encryptionType = WiFi.encryptionType(i);
        network.isConnected = (network.ssid == currentSSID);
        
        networks.push_back(network);
    }
    
    return networks;
}

bool WiFiManager::connectToNetwork(const String& ssid, const String& password, uint32_t timeout) {
    // Save original mode to restore if connection fails
    WiFiMode_t originalMode = _currentMode;
    
    // If we're in AP-only mode, switch to AP+STA
    if (_currentMode == WIFI_AP) {
        if (!begin(WIFI_AP_STA)) {
            Serial.println("Failed to switch to AP+STA mode");
            return false;
        }
        _currentMode = WIFI_AP_STA;
        _isBridgeModeEnabled = true;
    }
    // If we're in OFF mode, switch to STA
    else if (_currentMode == WIFI_OFF) {
        if (!begin(WIFI_STA)) {
            Serial.println("Failed to switch to STA mode");
            return false;
        }
        _currentMode = WIFI_STA;
    }
    
    // Attempt to connect
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection with timeout
    uint32_t startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > timeout) {
            Serial.println("Connection timeout");
            
            // Restore original mode if connection failed
            if (originalMode != _currentMode) {
                begin(originalMode);
                _currentMode = originalMode;
                _isBridgeModeEnabled = (_currentMode == WIFI_AP_STA);
            }
            
            return false;
        }
        delay(100);
    }
    
    return true;
}

bool WiFiManager::startAccessPoint(const String& ssid, const String& password, int channel, bool hidden, int maxConnections) {
    // Use default credentials if not provided
    String apSSID = (ssid.length() > 0) ? ssid : _apSSID;
    String apPassword = (password.length() > 0) ? password : _apPassword;

    // If we're in STA-only mode, switch to AP+STA to maintain any existing connection
    if (_currentMode == WIFI_STA) {
        if (!begin(WIFI_AP_STA)) {
            Serial.println("Failed to switch to AP+STA mode");
            return false;
        }
        _currentMode = WIFI_AP_STA;
        _isBridgeModeEnabled = true;
    }
    // If we're in OFF mode, switch to AP
    else if (_currentMode == WIFI_OFF) {
        if (!begin(WIFI_AP)) {
            Serial.println("Failed to switch to AP mode");
            return false;
        }
        _currentMode = WIFI_AP;
    }
    
    // Configure and start the access point
    bool result = WiFi.softAP(apSSID.c_str(), apPassword.c_str(), channel, hidden, maxConnections);
    
    // Initialize DNS server for captive portal
    if (result && _dnsServer == nullptr) {
        _dnsServer = new DNSServer();
        // For a captive portal, you'd typically set this up to redirect all DNS requests
        // to your device's IP address:
        // _dnsServer->start(53, "*", WiFi.softAPIP());
    }
    
    return result;
}

bool WiFiManager::enableBridgeMode(bool enable) {
    if (enable) {
        // If we're in AP-only mode, we need to switch to AP+STA
        if (_currentMode == WIFI_AP) {
            if (!begin(WIFI_AP_STA)) {
                Serial.println("Failed to switch to AP+STA mode");
                return false;
            }
            _currentMode = WIFI_AP_STA;
        }
        // If we're in STA-only mode, we need to start the AP as well
        else if (_currentMode == WIFI_STA) {
            if (!startAccessPoint()) {
                Serial.println("Failed to start access point for bridge mode");
                return false;
            }
            _currentMode = WIFI_AP_STA;
        }
    } else {
        // If we're in bridge mode, decide which single mode to keep
        if (_currentMode == WIFI_AP_STA) {
            // If we have a WiFi connection, keep STA mode
            if (WiFi.status() == WL_CONNECTED) {
                WiFi.softAPdisconnect(true);
                if (!begin(WIFI_STA)) {
                    Serial.println("Failed to switch to STA-only mode");
                    return false;
                }
                _currentMode = WIFI_STA;
            }
            // Otherwise, keep AP mode
            else {
                WiFi.disconnect(true);
                if (!begin(WIFI_AP)) {
                    Serial.println("Failed to switch to AP-only mode");
                    return false;
                }
                _currentMode = WIFI_AP;
            }
        }
    }
    
    _isBridgeModeEnabled = (_currentMode == WIFI_AP_STA);
    return true;
}

bool WiFiManager::disconnect() {
    bool result = WiFi.disconnect(false);
    
    // If we're in bridge mode, stay in that mode
    if (_currentMode == WIFI_AP_STA) {
        // No mode change needed
    }
    // If we're in STA-only mode, go to OFF mode
    else if (_currentMode == WIFI_STA) {
        if (begin(WIFI_OFF)) {
            _currentMode = WIFI_OFF;
        } else {
            Serial.println("Failed to switch to OFF mode");
        }
    }
    
    return result;
}

bool WiFiManager::stopAccessPoint() {
    bool result = WiFi.softAPdisconnect(true);
    
    // If we're in bridge mode, switch to STA-only mode
    if (_currentMode == WIFI_AP_STA) {
        if (WiFi.status() == WL_CONNECTED) {
            if (begin(WIFI_STA)) {
                _currentMode = WIFI_STA;
                _isBridgeModeEnabled = false;
            } else {
                Serial.println("Failed to switch to STA-only mode");
            }
        } else {
            // No WiFi connection, go to OFF mode
            if (begin(WIFI_OFF)) {
                _currentMode = WIFI_OFF;
                _isBridgeModeEnabled = false;
            } else {
                Serial.println("Failed to switch to OFF mode");
            }
        }
    }
    // If we're in AP-only mode, go to OFF mode
    else if (_currentMode == WIFI_AP) {
        if (begin(WIFI_OFF)) {
            _currentMode = WIFI_OFF;
        } else {
            Serial.println("Failed to switch to OFF mode");
        }
    }
    
    // Clean up DNS server
    if (_dnsServer) {
        _dnsServer->stop();
        delete _dnsServer;
        _dnsServer = nullptr;
    }
    
    return result;
}

wl_status_t WiFiManager::getStatus() {
    return WiFi.status();
}

WiFiMode_t WiFiManager::getMode() {
    return _currentMode;
}

NetworkInfo WiFiManager::getCurrentConnection() {
    NetworkInfo info;
    
    if (WiFi.status() == WL_CONNECTED) {
        info.ssid = WiFi.SSID();
        info.bssid = WiFi.BSSIDstr();
        info.rssi = WiFi.RSSI();
        info.encryptionType = 0; // Not available for current connection
        info.isConnected = true;
    } else {
        info.isConnected = false;
    }
    
    return info;
}

void WiFiManager::setEventCallback(std::function<void(WiFiEvent_t event, WiFiEventInfo_t info)> callback) {
    _eventCallback = callback;
    WiFi.onEvent(_eventCallback);
}

void WiFiManager::handleEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    // Process events internally as needed
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("WiFi connected. IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("WiFi lost connection");
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            {
                // For AP IP assignment events, we need to get the IP from the event
                uint32_t ipAddr = info.got_ip.ip_info.ip.addr;
                IPAddress clientIP(ipAddr & 0xFF, (ipAddr >> 8) & 0xFF, (ipAddr >> 16) & 0xFF, (ipAddr >> 24) & 0xFF);
                Serial.printf("Client assigned IP: %s\n", clientIP.toString().c_str());
                bool assigned = false;
                for (auto& client : _connectedClients) {
                    if (client.ipAddress == "0.0.0.0") {
                        client.ipAddress = clientIP.toString();
                        Serial.printf("Assigned IP %s to client %s\n", client.ipAddress.c_str(), client.macAddress.c_str());
                        assigned = true;
                        break;
                    }
                }
                
                // If no existing client found to assign IP to, create a new entry with unknown MAC
                if (!assigned) {
                    uint8_t unknownMac[6] = {0, 0, 0, 0, 0, 0};
                    updateClientInfo(unknownMac, clientIP, true);
                }
            }
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            {
                Serial.println("Client connected to AP");
                // Track the connected client
                updateClientInfo(info.wifi_ap_staconnected.mac, IPAddress(0,0,0,0), true);
            }
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            {
                Serial.println("Client disconnected from AP");
                // Remove the disconnected client from tracking
                updateClientInfo(info.wifi_ap_stadisconnected.mac, IPAddress(0,0,0,0), false);
            }
            break;
    }
    
    // Call user callback if registered
    if (_eventCallback) {
        _eventCallback(event, info);
    }
}

std::vector<ClientInfo> WiFiManager::getConnectedClients() {
    std::vector<ClientInfo> clients;
    
    // If we're not in AP mode, return empty list
    if (_currentMode != WIFI_AP && _currentMode != WIFI_AP_STA) {
        return clients;
    }
    
    // Get the list of stations connected to the AP
    wifi_sta_list_t stationList;
    esp_err_t err = esp_wifi_ap_get_sta_list(&stationList);
    if (err != ESP_OK) {
        Serial.printf("Failed to get station list: %d\n", err);
        return clients;
    }
    
    // For each station in the list, get the details from the network interface
    for (int i = 0; i < stationList.num; i++) {
        ClientInfo client;
        
        // Format MAC address
        char macStr[18] = { 0 };
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                stationList.sta[i].mac[0], stationList.sta[i].mac[1], stationList.sta[i].mac[2],
                stationList.sta[i].mac[3], stationList.sta[i].mac[4], stationList.sta[i].mac[5]);
        client.macAddress = String(macStr);
        
        // Get IP address from DHCP lease table
        // In ESP-IDF v6 we need to look up the IP from the AP interface
        client.ipAddress = "0.0.0.0"; // Default if we can't find it
        
        // Now look for the IP in our tracked clients
        for (const auto& trackedClient : _connectedClients) {
            if (trackedClient.macAddress.equalsIgnoreCase(client.macAddress)) {
                if (trackedClient.ipAddress != "0.0.0.0") {
                    client.ipAddress = trackedClient.ipAddress;
                }
                break;
            }
        }
        
        // If we couldn't find the IP in our tracked clients, we can try to use
        // the DHCP lease table. In a real implementation, this would require
        // platform-specific code that would change between ESP-IDF versions.
        // For simplicity, we'll use WiFi.softAPIP() network with last byte from MAC
        // if (client.ipAddress == "0.0.0.0") {
        //     IPAddress baseIp = WiFi.softAPIP();
        //     // Use the last byte of the MAC as a probable last byte for the IP
        //     // This is just a heuristic and might not be accurate
        //     IPAddress probableIp(baseIp[0], baseIp[1], baseIp[2], stationList.sta[i].mac[5]);
        //     client.ipAddress = probableIp.toString();
        // }
        
        // Get hostname (may require additional lookup)
        client.hostname = getClientHostname(client.ipAddress);
        
        // Signal strength is available in the station list
        client.rssi = stationList.sta[i].rssi;
        client.connectionTime = 0;
        
        // Look for matching client in our tracking list to get additional info
        for (auto& trackedClient : _connectedClients) {
            if (trackedClient.macAddress.equalsIgnoreCase(client.macAddress)) {
                // Keep the connection time from our tracking
                client.connectionTime = trackedClient.connectionTime;
                
                // Update the IP address in our tracking if it was unknown before
                if (trackedClient.ipAddress == "0.0.0.0") {
                    trackedClient.ipAddress = client.ipAddress;
                    
                    // Try to resolve hostname now that we have IP
                    if (trackedClient.hostname.isEmpty()) {
                        trackedClient.hostname = getClientHostname(client.ipAddress);
                    }
                }
                
                // Use the hostname from our tracking if available
                if (!trackedClient.hostname.isEmpty()) {
                    client.hostname = trackedClient.hostname;
                }
                
                break;
            }
        }
        
        clients.push_back(client);
    }
    
    return clients;
}

String WiFiManager::getClientHostname(const String& ipAddress) {
    String hostname = "";
    
    // Try to resolve hostname using mDNS if IP is valid
    IPAddress ip;
    if (ip.fromString(ipAddress)) {
        // First, check if we can find the hostname in our cached client list
        for (const auto& client : _connectedClients) {
            if (client.ipAddress == ipAddress && !client.hostname.isEmpty()) {
                return client.hostname;
            }
        }
        
        // With ESP-IDF v6, the mDNS API has changed significantly
        // We'll use a simpler approach - just use MAC-based hostnames
        // We look up the MAC address from our tracked clients
        for (const auto& client : _connectedClients) {
            if (client.ipAddress == ipAddress) {
                // Generate a hostname based on MAC address
                // Extract just the last 6 chars of the MAC (last 3 bytes)
                if (client.macAddress.length() >= 17) {  // MACs are like "AA:BB:CC:DD:EE:FF"
                    String macSuffix = client.macAddress.substring(client.macAddress.length() - 8);
                    macSuffix.replace(":", "");  // Remove colons
                    hostname = "esp-" + macSuffix;
                    return hostname;
                }
            }
        }
        
        // If all else fails, use IP as hostname
        hostname = ipAddress;
    }
    
    return hostname;
}

bool WiFiManager::disconnectClient(const String& macAddress) {
    // Convert string MAC address to uint8_t array
    uint8_t mac[6];
    int values[6];
    
    if (sscanf(macAddress.c_str(), "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) == 6) {
        for (int i = 0; i < 6; i++) {
            mac[i] = static_cast<uint8_t>(values[i]);
        }
        
        // Attempt to disconnect the client
        return WiFi.softAPdisconnect(mac);
    }
    
    return false;
}

void WiFiManager::updateClientInfo(const uint8_t* mac, const IPAddress& ip, bool isConnecting) {
    // Format MAC address to string
    char macStr[18] = { 0 };
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    String macAddress = String(macStr);
    
    if (isConnecting) {
        // Check if this client is already in our list
        bool found = false;
        for (auto& client : _connectedClients) {
            if (client.macAddress.equalsIgnoreCase(macAddress)) {
                // Update the client's info
                client.ipAddress = ip.toString();
                client.rssi = WiFi.RSSI(); // This is not accurate for the specific client
                found = true;
                break;
            }
        }
        
        if (!found) {
            // Add new client to our list
            ClientInfo newClient;
            newClient.macAddress = macAddress;
            newClient.ipAddress = ip.toString();
            newClient.hostname = ""; // Will try to resolve later
            newClient.rssi = WiFi.RSSI(); // Not accurate for specific client
            newClient.connectionTime = millis();
            _connectedClients.push_back(newClient);
        }
    } else {
        // Remove disconnected client from our list
        for (auto it = _connectedClients.begin(); it != _connectedClients.end(); ) {
            if (it->macAddress.equalsIgnoreCase(macAddress)) {
                it = _connectedClients.erase(it);
            } else {
                ++it;
            }
        }
    }
}
