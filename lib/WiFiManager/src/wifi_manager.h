#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <vector>
#include <functional>

/**
 * @brief WiFi network information structure
 */
struct NetworkInfo {
    String ssid;
    String bssid;
    int32_t rssi;
    uint8_t encryptionType;
    bool isConnected;
};

/**
 * @brief WiFi client information structure
 */
struct ClientInfo {
    String macAddress;
    String ipAddress;
    String hostname;
    int32_t rssi;
    unsigned long connectionTime;
};

/**
 * @brief WiFi Manager class for ESP32
 * 
 * Provides smartphone-like WiFi management capabilities:
 * - Scan for available networks
 * - Connect to a selected network
 * - Create an access point (hotspot)
 * - Bridge between WiFi client and AP modes
 */
class WiFiManager {
public:
    /**
     * @brief Construct a new WiFi Manager object
     * 
     * @param apSSID Default SSID for the access point mode (can be changed later)
     * @param apPassword Default password for the access point mode (can be changed later)
     */
    WiFiManager(const String& apSSID = "ESP32_AP", const String& apPassword = "password");
    
    /**
     * @brief Destroy the WiFi Manager object
     */
    ~WiFiManager();

    /**
     * @brief Initialize the WiFi manager
     * 
     * @param mode Initial WiFi mode (WIFI_STA, WIFI_AP, or WIFI_AP_STA)
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool begin(WiFiMode_t mode = WIFI_STA);

    /**
     * @brief Scan for available WiFi networks
     * 
     * @param async Set to true for asynchronous scanning (non-blocking)
     * @return Number of networks found or WIFI_SCAN_RUNNING if async scan started
     */
    int16_t scanNetworks(bool async = false);

    /**
     * @brief Check if an asynchronous scan is complete
     * 
     * @return Number of networks found, WIFI_SCAN_RUNNING, or WIFI_SCAN_FAILED
     */
    int16_t scanComplete();

    /**
     * @brief Get the list of available networks from the last scan
     * 
     * @return std::vector<NetworkInfo> List of network information
     */
    std::vector<NetworkInfo> getNetworkList();

    /**
     * @brief Connect to a specific WiFi network
     * 
     * @param ssid Network SSID
     * @param password Network password
     * @param timeout Connection timeout in milliseconds
     * @return true if connection successful
     * @return false if connection failed
     */
    bool connectToNetwork(const String& ssid, const String& password, uint32_t timeout = 30000);

    /**
     * @brief Start an access point (hotspot)
     * 
     * @param ssid AP SSID (defaults to constructor value if empty)
     * @param password AP password (defaults to constructor value if empty)
     * @param channel WiFi channel (1-13)
     * @param hidden Whether the AP SSID is hidden
     * @param maxConnections Maximum number of client connections
     * @return true if AP started successfully
     * @return false if AP failed to start
     */
    bool startAccessPoint(const String& ssid = "", const String& password = "", int channel = 1, 
                          bool hidden = false, int maxConnections = 4);

    /**
     * @brief Enable bridging between WiFi client and AP modes
     * 
     * When enabled, the device will work in WIFI_AP_STA mode,
     * allowing it to be connected to a WiFi network while also
     * serving as an access point.
     * 
     * @param enable Whether to enable bridging
     * @return true if mode change was successful
     * @return false if mode change failed
     */
    bool enableBridgeMode(bool enable = true);

    /**
     * @brief Disconnect from the current WiFi network
     * 
     * @return true if disconnection was successful
     * @return false if disconnection failed
     */
    bool disconnect();

    /**
     * @brief Stop the access point
     * 
     * @return true if AP was stopped successfully
     * @return false if stopping the AP failed
     */
    bool stopAccessPoint();

    /**
     * @brief Get the current connection status
     * 
     * @return wl_status_t Current WiFi status
     */
    wl_status_t getStatus();

    /**
     * @brief Get the current WiFi mode
     * 
     * @return WiFiMode_t Current mode
     */
    WiFiMode_t getMode();

    /**
     * @brief Get information about the current connection
     * 
     * @return NetworkInfo Structure with current connection details
     */
    NetworkInfo getCurrentConnection();

    /**
     * @brief Get information about connected clients
     * 
     * @return std::vector<ClientInfo> List of connected client information
     */
    std::vector<ClientInfo> getConnectedClients();

    /**
     * @brief Get hostname of a client by IP address
     * 
     * @param ipAddress IP address of the client
     * @return String Hostname of the client (empty if not found)
     */
    String getClientHostname(const String& ipAddress);

    /**
     * @brief Disconnect a specific client by MAC address
     * 
     * @param macAddress MAC address of the client to disconnect
     * @return true if disconnection was successful
     * @return false if disconnection failed
     */
    bool disconnectClient(const String& macAddress);

    /**
     * @brief Set a callback function for WiFi events
     * 
     * @param callback Function to call when WiFi events occur
     */
    void setEventCallback(std::function<void(WiFiEvent_t event, WiFiEventInfo_t info)> callback);

    /**
     * @brief Handle WiFi events (called internally)
     * 
     * @param event WiFi event type
     * @param info WiFi event information
     */
    void handleEvent(WiFiEvent_t event, WiFiEventInfo_t info);
    
    // Internal method to update client information
    void updateClientInfo(const uint8_t* mac, const IPAddress& ip, bool isConnecting = true);

private:
    // Default AP credentials
    String _apSSID;
    String _apPassword;

    // Current WiFi state
    WiFiMode_t _currentMode;
    bool _isBridgeModeEnabled;
    
    // DNS server for captive portal functionality
    DNSServer* _dnsServer;
    
    // Callback for WiFi events
    std::function<void(WiFiEvent_t event, WiFiEventInfo_t info)> _eventCallback;
    
    // Client tracking
    std::vector<ClientInfo> _connectedClients;
};

#endif // WIFI_MANAGER_H
