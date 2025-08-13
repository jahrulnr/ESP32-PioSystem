#include "websocket_client.h"
#include "SerialDebug.h"

// Global instance
WebSocketClientManager webSocketClient;

WebSocketClientManager::WebSocketClientManager() 
    : pingInterval(30000)
    , pongTimeout(10000)
    , maxReconnectAttempts(5)
    , reconnectDelay(5000)
    , pingTaskHandle(nullptr)
    , reconnectTaskHandle(nullptr)
    , debugOutput(false)
{
}

WebSocketClientManager::~WebSocketClientManager() {
    disconnectAll();
    
    if (pingTaskHandle) {
        vTaskDelete(pingTaskHandle);
    }
    if (reconnectTaskHandle) {
        vTaskDelete(reconnectTaskHandle);
    }
    
    // Clean up connections
    for (auto& pair : connections) {
        delete pair.second;
    }
    connections.clear();
}

void WebSocketClientManager::begin(unsigned long pingIntervalMs, 
                                  unsigned long pongTimeoutMs,
                                  int maxReconnects,
                                  unsigned long reconnectDelayMs) {
    this->pingInterval = pingIntervalMs;
    this->pongTimeout = pongTimeoutMs;
    this->maxReconnectAttempts = maxReconnects;
    this->reconnectDelay = reconnectDelayMs;
    
    // Create ping task
    xTaskCreatePinnedToCore(
        pingTask,
        "WSPingTask",
        4096,
        this,
        1,
        &pingTaskHandle,
        0  // Core 0
    );
    
    // Create reconnect task
    xTaskCreatePinnedToCore(
        reconnectTask,
        "WSReconnectTask",
        4096,
        this,
        1,
        &reconnectTaskHandle,
        0  // Core 0
    );
    
    debugPrint("WebSocket Client Manager initialized");
}

bool WebSocketClientManager::connectToDevice(const String& deviceId, 
                                            const String& host, 
                                            int port, 
                                            const String& path,
                                            bool autoReconnect) {
    debugPrintf("Connecting to device %s at %s:%d%s", deviceId.c_str(), host.c_str(), port, path.c_str());
    
    // Check if already connected
    if (connections.find(deviceId) != connections.end()) {
        debugPrintf("Device %s already has a connection", deviceId.c_str());
        disconnectFromDevice(deviceId);
    }
    
    // Create new connection
    WSDeviceConnection* connection = new WSDeviceConnection();
    connection->deviceId = deviceId;
    connection->host = host;
    connection->port = port;
    connection->path = path;
    connection->url = "ws://" + host + ":" + String(port) + path;
    connection->state = WSConnectionState::CONNECTING;
    connection->lastPing = 0;
    connection->lastPong = millis();
    connection->connectTime = millis();
    connection->reconnectAttempts = 0;
    connection->autoReconnect = autoReconnect;
    
    // Setup client handlers
    setupClientHandlers(connection);
    
    // Attempt connection
    bool connected = connection->client.connect(connection->url);
    
    if (connected) {
        connection->state = WSConnectionState::CONNECTED;
        connection->connectTime = millis();
        connections[deviceId] = connection;
        
        debugPrintf("Connected to device %s", deviceId.c_str());
        
        if (connectionCallback) {
            connectionCallback(deviceId, WSConnectionState::CONNECTED);
        }
        
        return true;
    } else {
        connection->state = WSConnectionState::ERROR_STATE;
        debugPrintf("Failed to connect to device %s", deviceId.c_str());
        
        if (connectionCallback) {
            connectionCallback(deviceId, WSConnectionState::ERROR_STATE);
        }
        
        if (autoReconnect) {
            connections[deviceId] = connection;
        } else {
            delete connection;
        }
        
        return false;
    }
}

void WebSocketClientManager::setupClientHandlers(WSDeviceConnection* connection) {
    connection->client.onMessage([this, connection](WebsocketsMessage message) {
        handleMessage(connection, message);
    });
    
    connection->client.onEvent([this, connection](WebsocketsEvent event, String data) {
        handleConnectionEvent(connection, event, data);
    });
}

void WebSocketClientManager::handleMessage(WSDeviceConnection* connection, WebsocketsMessage message) {
    debugPrintf("Received message from %s: %s", connection->deviceId.c_str(), message.data().c_str());
    
    if (message.isText()) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message.data());
        
        if (error) {
            debugPrintf("Failed to parse JSON message from %s: %s", connection->deviceId.c_str(), error.c_str());
            return;
        }
        
        // Check for pong response
        if (doc["type"] == "pong") {
            connection->lastPong = millis();
            debugPrintf("Received pong from %s", connection->deviceId.c_str());
            return;
        }
        
        // Parse message type
        WSMessageType type = parseMessageType(doc["type"] | "custom");
        
        if (messageCallback) {
            messageCallback(connection->deviceId, type, doc);
        }
    }
}

void WebSocketClientManager::handleConnectionEvent(WSDeviceConnection* connection, WebsocketsEvent event, String data) {
    switch (event) {
        case WebsocketsEvent::ConnectionOpened:
            debugPrintf("WebSocket connection opened to %s", connection->deviceId.c_str());
            connection->state = WSConnectionState::CONNECTED;
            connection->reconnectAttempts = 0;
            connection->lastPong = millis();
            
            if (connectionCallback) {
                connectionCallback(connection->deviceId, WSConnectionState::CONNECTED);
            }
            break;
            
        case WebsocketsEvent::ConnectionClosed:
            debugPrintf("WebSocket connection closed to %s", connection->deviceId.c_str());
            connection->state = WSConnectionState::DISCONNECTED;
            
            if (connectionCallback) {
                connectionCallback(connection->deviceId, WSConnectionState::DISCONNECTED);
            }
            
            if (connection->autoReconnect && connection->reconnectAttempts < maxReconnectAttempts) {
                connection->state = WSConnectionState::RECONNECTING;
                if (connectionCallback) {
                    connectionCallback(connection->deviceId, WSConnectionState::RECONNECTING);
                }
            }
            break;
            
        case WebsocketsEvent::GotPing:
            debugPrintf("Received ping from %s", connection->deviceId.c_str());
            connection->client.pong();
            break;
            
        case WebsocketsEvent::GotPong:
            debugPrintf("Received pong from %s", connection->deviceId.c_str());
            connection->lastPong = millis();
            break;
    }
}

void WebSocketClientManager::disconnectFromDevice(const String& deviceId) {
    auto it = connections.find(deviceId);
    if (it != connections.end()) {
        debugPrintf("Disconnecting from device %s", deviceId.c_str());
        
        WSDeviceConnection* connection = it->second;
        connection->client.close();
        connection->state = WSConnectionState::DISCONNECTED;
        
        delete connection;
        connections.erase(it);
        
        if (connectionCallback) {
            connectionCallback(deviceId, WSConnectionState::DISCONNECTED);
        }
    }
}

void WebSocketClientManager::disconnectAll() {
    debugPrint("Disconnecting from all devices");
    
    for (auto& pair : connections) {
        pair.second->client.close();
        if (connectionCallback) {
            connectionCallback(pair.first, WSConnectionState::DISCONNECTED);
        }
        delete pair.second;
    }
    connections.clear();
}

bool WebSocketClientManager::sendMessage(const String& deviceId, WSMessageType type, const JsonDocument& data) {
    auto it = connections.find(deviceId);
    if (it == connections.end() || it->second->state != WSConnectionState::CONNECTED) {
        debugPrintf("Cannot send message to %s: not connected", deviceId.c_str());
        return false;
    }
    
    JsonDocument message;
    message["type"] = messageTypeToString(type);
    message["timestamp"] = millis();
    message["data"] = data;
    
    String messageStr;
    serializeJson(message, messageStr);
    
    bool sent = it->second->client.send(messageStr);
    if (sent) {
        debugPrintf("Sent message to %s: %s", deviceId.c_str(), messageStr.c_str());
    } else {
        debugPrintf("Failed to send message to %s", deviceId.c_str());
    }
    
    return sent;
}

bool WebSocketClientManager::sendCommand(const String& deviceId, const String& command, const JsonDocument& parameters) {
    JsonDocument data;
    data["command"] = command;
    data["parameters"] = parameters;
    data["id"] = String(millis()) + "_" + deviceId;
    
    return sendMessage(deviceId, WSMessageType::COMMAND, data);
}

bool WebSocketClientManager::sendResponse(const String& deviceId, const String& requestId, bool success, const JsonDocument& data) {
    JsonDocument responseData;
    responseData["requestId"] = requestId;
    responseData["success"] = success;
    responseData["data"] = data;
    
    return sendMessage(deviceId, WSMessageType::RESPONSE, responseData);
}

bool WebSocketClientManager::ping(const String& deviceId) {
    auto it = connections.find(deviceId);
    if (it == connections.end() || it->second->state != WSConnectionState::CONNECTED) {
        return false;
    }
    
    JsonDocument pingData;
    pingData["timestamp"] = millis();
    
    bool sent = sendMessage(deviceId, WSMessageType::PING, pingData);
    if (sent) {
        it->second->lastPing = millis();
    }
    
    return sent;
}

void WebSocketClientManager::pingAll() {
    for (auto& pair : connections) {
        if (pair.second->state == WSConnectionState::CONNECTED) {
            ping(pair.first);
        }
    }
}

WSConnectionState WebSocketClientManager::getConnectionState(const String& deviceId) {
    auto it = connections.find(deviceId);
    if (it != connections.end()) {
        return it->second->state;
    }
    return WSConnectionState::DISCONNECTED;
}

bool WebSocketClientManager::isConnected(const String& deviceId) {
    return getConnectionState(deviceId) == WSConnectionState::CONNECTED;
}

std::vector<String> WebSocketClientManager::getConnectedDevices() {
    std::vector<String> connectedDevices;
    for (auto& pair : connections) {
        if (pair.second->state == WSConnectionState::CONNECTED) {
            connectedDevices.push_back(pair.first);
        }
    }
    return connectedDevices;
}

WSDeviceConnection* WebSocketClientManager::getConnectionInfo(const String& deviceId) {
    auto it = connections.find(deviceId);
    if (it != connections.end()) {
        return it->second;
    }
    return nullptr;
}

void WebSocketClientManager::setMessageCallback(WSMessageCallback callback) {
    this->messageCallback = callback;
}

void WebSocketClientManager::setConnectionCallback(WSConnectionCallback callback) {
    this->connectionCallback = callback;
}

void WebSocketClientManager::update() {
    for (auto& pair : connections) {
        WSDeviceConnection* connection = pair.second;
        
        if (connection->state == WSConnectionState::CONNECTED) {
            connection->client.poll();
            
            // Check for pong timeout
            if (connection->lastPing > 0 && 
                (millis() - connection->lastPong) > pongTimeout) {
                debugPrintf("Pong timeout for device %s", connection->deviceId.c_str());
                connection->state = WSConnectionState::ERROR_STATE;
                
                if (connectionCallback) {
                    connectionCallback(connection->deviceId, WSConnectionState::ERROR_STATE);
                }
                
                if (connection->autoReconnect) {
                    connection->state = WSConnectionState::RECONNECTING;
                }
            }
        }
    }
}

size_t WebSocketClientManager::getConnectionCount() {
    size_t count = 0;
    for (auto& pair : connections) {
        if (pair.second->state == WSConnectionState::CONNECTED) {
            count++;
        }
    }
    return count;
}

void WebSocketClientManager::setDebugOutput(bool enable) {
    this->debugOutput = enable;
}

// Static task functions
void WebSocketClientManager::pingTask(void* parameter) {
    WebSocketClientManager* manager = static_cast<WebSocketClientManager*>(parameter);
    
    while (true) {
        manager->pingAll();
        vTaskDelay(pdMS_TO_TICKS(manager->pingInterval));
    }
}

void WebSocketClientManager::reconnectTask(void* parameter) {
    WebSocketClientManager* manager = static_cast<WebSocketClientManager*>(parameter);
    
    while (true) {
        for (auto& pair : manager->connections) {
            WSDeviceConnection* connection = pair.second;
            
            if (connection->state == WSConnectionState::RECONNECTING) {
                manager->attemptReconnect(connection);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(manager->reconnectDelay));
    }
}

void WebSocketClientManager::attemptReconnect(WSDeviceConnection* connection) {
    if (connection->reconnectAttempts >= maxReconnectAttempts) {
        debugPrintf("Max reconnect attempts reached for device %s", connection->deviceId.c_str());
        connection->state = WSConnectionState::ERROR_STATE;
        
        if (connectionCallback) {
            connectionCallback(connection->deviceId, WSConnectionState::ERROR_STATE);
        }
        return;
    }
    
    connection->reconnectAttempts++;
    debugPrintf("Reconnect attempt %d/%d for device %s", 
                connection->reconnectAttempts, maxReconnectAttempts, connection->deviceId.c_str());
    
    connection->state = WSConnectionState::CONNECTING;
    if (connectionCallback) {
        connectionCallback(connection->deviceId, WSConnectionState::CONNECTING);
    }
    
    setupClientHandlers(connection);
    bool connected = connection->client.connect(connection->url);
    
    if (connected) {
        connection->state = WSConnectionState::CONNECTED;
        connection->connectTime = millis();
        connection->lastPong = millis();
        connection->reconnectAttempts = 0;
        
        debugPrintf("Reconnected to device %s", connection->deviceId.c_str());
        
        if (connectionCallback) {
            connectionCallback(connection->deviceId, WSConnectionState::CONNECTED);
        }
    } else {
        connection->state = WSConnectionState::RECONNECTING;
        debugPrintf("Reconnect failed for device %s", connection->deviceId.c_str());
    }
}

WSMessageType WebSocketClientManager::parseMessageType(const String& typeStr) {
    if (typeStr == "ping") return WSMessageType::PING;
    if (typeStr == "pong") return WSMessageType::PONG;
    if (typeStr == "device_status") return WSMessageType::DEVICE_STATUS;
    if (typeStr == "command") return WSMessageType::COMMAND;
    if (typeStr == "response") return WSMessageType::RESPONSE;
    if (typeStr == "error") return WSMessageType::ERROR;
    if (typeStr == "camera_stream") return WSMessageType::CAMERA_STREAM;
    if (typeStr == "sensor_data") return WSMessageType::SENSOR_DATA;
    return WSMessageType::CUSTOM;
}

String WebSocketClientManager::messageTypeToString(WSMessageType type) {
    switch (type) {
        case WSMessageType::PING: return "ping";
        case WSMessageType::PONG: return "pong";
        case WSMessageType::DEVICE_STATUS: return "device_status";
        case WSMessageType::COMMAND: return "command";
        case WSMessageType::RESPONSE: return "response";
        case WSMessageType::ERROR: return "error";
        case WSMessageType::CAMERA_STREAM: return "camera_stream";
        case WSMessageType::SENSOR_DATA: return "sensor_data";
        case WSMessageType::CUSTOM: return "custom";
        default: return "unknown";
    }
}

void WebSocketClientManager::debugPrint(const String& message) {
    if (debugOutput) {
        DEBUG_PRINTF("[WebSocketClient] %s\n", message.c_str());
    }
}

void WebSocketClientManager::debugPrintf(const char* format, ...) {
    if (debugOutput) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        DEBUG_PRINTF("[WebSocketClient] %s\n", buffer);
    }
}
