#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <functional>
#include <map>

using namespace websockets;

// WebSocket message types
enum class WSMessageType {
    PING,
    PONG,
    DEVICE_STATUS,
    COMMAND,
    RESPONSE,
    ERROR,
    CAMERA_STREAM,
    SENSOR_DATA,
    CUSTOM
};

// WebSocket connection state
enum class WSConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    ERROR_STATE
};

// Message callback function type
typedef std::function<void(const String& deviceId, WSMessageType type, const JsonDocument& data)> WSMessageCallback;
typedef std::function<void(const String& deviceId, WSConnectionState state)> WSConnectionCallback;

// WebSocket device connection info
struct WSDeviceConnection {
    String deviceId;
    String url;
    String host;
    int port;
    String path;
    WebsocketsClient client;
    WSConnectionState state;
    unsigned long lastPing;
    unsigned long lastPong;
    unsigned long connectTime;
    int reconnectAttempts;
    bool autoReconnect;
};

/**
 * @brief WebSocket Client Manager for ESP32 IoT device communication
 * 
 * Provides real-time communication capabilities between ESP32 devices
 * using WebSocket protocol with automatic reconnection and message handling.
 */
class WebSocketClientManager {
private:
    std::map<String, WSDeviceConnection*> connections;
    WSMessageCallback messageCallback;
    WSConnectionCallback connectionCallback;
    
    // Configuration
    unsigned long pingInterval;
    unsigned long pongTimeout;
    int maxReconnectAttempts;
    unsigned long reconnectDelay;
    
    // Task handles
    TaskHandle_t pingTaskHandle;
    TaskHandle_t reconnectTaskHandle;
    
    // Internal methods
    void setupClientHandlers(WSDeviceConnection* connection);
    void handleMessage(WSDeviceConnection* connection, WebsocketsMessage message);
    void handleConnectionEvent(WSDeviceConnection* connection, WebsocketsEvent event, String data);
    void sendPing(WSDeviceConnection* connection);
    void attemptReconnect(WSDeviceConnection* connection);
    WSMessageType parseMessageType(const String& typeStr);
    String messageTypeToString(WSMessageType type);
    
    // Static task functions
    static void pingTask(void* parameter);
    static void reconnectTask(void* parameter);

public:
    /**
     * @brief Constructor
     */
    WebSocketClientManager();
    
    /**
     * @brief Destructor
     */
    ~WebSocketClientManager();
    
    /**
     * @brief Initialize the WebSocket client manager
     * 
     * @param pingIntervalMs Ping interval in milliseconds (default: 30000)
     * @param pongTimeoutMs Pong timeout in milliseconds (default: 10000)
     * @param maxReconnects Maximum reconnection attempts (default: 5)
     * @param reconnectDelayMs Delay between reconnection attempts (default: 5000)
     */
    void begin(unsigned long pingIntervalMs = 30000, 
               unsigned long pongTimeoutMs = 10000,
               int maxReconnects = 5,
               unsigned long reconnectDelayMs = 5000);
    
    /**
     * @brief Connect to a WebSocket server on another device
     * 
     * @param deviceId Unique identifier for the device
     * @param host Host IP address or hostname
     * @param port WebSocket port (default: 81)
     * @param path WebSocket path (default: "/ws")
     * @param autoReconnect Enable automatic reconnection (default: true)
     * @return true if connection initiated successfully
     */
    bool connectToDevice(const String& deviceId, 
                        const String& host, 
                        int port = 81, 
                        const String& path = "/ws",
                        bool autoReconnect = true);
    
    /**
     * @brief Disconnect from a specific device
     * 
     * @param deviceId Device identifier
     */
    void disconnectFromDevice(const String& deviceId);
    
    /**
     * @brief Disconnect from all devices
     */
    void disconnectAll();
    
    /**
     * @brief Send a message to a specific device
     * 
     * @param deviceId Target device identifier
     * @param type Message type
     * @param data JSON data to send
     * @return true if message sent successfully
     */
    bool sendMessage(const String& deviceId, WSMessageType type, const JsonDocument& data);
    
    /**
     * @brief Send a command to a specific device
     * 
     * @param deviceId Target device identifier
     * @param command Command string
     * @param parameters Command parameters as JSON
     * @return true if command sent successfully
     */
    bool sendCommand(const String& deviceId, const String& command, const JsonDocument& parameters = JsonDocument());
    
    /**
     * @brief Send a response to a specific device
     * 
     * @param deviceId Target device identifier
     * @param requestId Original request ID
     * @param success Whether the operation was successful
     * @param data Response data
     * @return true if response sent successfully
     */
    bool sendResponse(const String& deviceId, const String& requestId, bool success, const JsonDocument& data = JsonDocument());
    
    /**
     * @brief Send ping to a specific device
     * 
     * @param deviceId Target device identifier
     * @return true if ping sent successfully
     */
    bool ping(const String& deviceId);
    
    /**
     * @brief Send ping to all connected devices
     */
    void pingAll();
    
    /**
     * @brief Get connection state for a device
     * 
     * @param deviceId Device identifier
     * @return Connection state
     */
    WSConnectionState getConnectionState(const String& deviceId);
    
    /**
     * @brief Check if connected to a device
     * 
     * @param deviceId Device identifier
     * @return true if connected
     */
    bool isConnected(const String& deviceId);
    
    /**
     * @brief Get list of connected device IDs
     * 
     * @return Vector of connected device IDs
     */
    std::vector<String> getConnectedDevices();
    
    /**
     * @brief Get connection info for a device
     * 
     * @param deviceId Device identifier
     * @return Connection info or nullptr if not found
     */
    WSDeviceConnection* getConnectionInfo(const String& deviceId);
    
    /**
     * @brief Set message callback function
     * 
     * @param callback Callback function for incoming messages
     */
    void setMessageCallback(WSMessageCallback callback);
    
    /**
     * @brief Set connection state callback function
     * 
     * @param callback Callback function for connection state changes
     */
    void setConnectionCallback(WSConnectionCallback callback);
    
    /**
     * @brief Update all connections (call in main loop)
     */
    void update();
    
    /**
     * @brief Get total number of active connections
     * 
     * @return Number of active connections
     */
    size_t getConnectionCount();
    
    /**
     * @brief Enable/disable debug output
     * 
     * @param enable Debug output state
     */
    void setDebugOutput(bool enable);
    
private:
    bool debugOutput;
    void debugPrint(const String& message);
    void debugPrintf(const char* format, ...);
};

// Global instance (optional - can be used as singleton)
extern WebSocketClientManager webSocketClient;

#endif // WEBSOCKET_CLIENT_H
