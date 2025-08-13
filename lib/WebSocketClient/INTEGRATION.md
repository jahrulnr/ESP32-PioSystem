# WebSocket Client Integration

This document shows how to integrate the WebSocket client into your main ESP32 application.

## Integration Steps

### 1. Include the Library

Add to your main.cpp or controller files:

```cpp
#include "lib/WebSocketClient/src/websocket_client.h"
```

### 2. Add to Task Initialization

In your `src/Tasks/init.cpp`, add WebSocket initialization:

```cpp
void initializeWebSocketClient() {
    DEBUG_PRINTLN("Initializing WebSocket client...");
    
    // Initialize with custom settings
    webSocketClient.begin(
        30000,  // Ping interval: 30 seconds
        10000,  // Pong timeout: 10 seconds
        3,      // Max reconnect attempts
        5000    // Reconnect delay: 5 seconds
    );
    
    webSocketClient.setDebugOutput(true);
    
    // Set callbacks
    webSocketClient.setMessageCallback([](const String& deviceId, WSMessageType type, const JsonDocument& data) {
        handleWebSocketMessage(deviceId, type, data);
    });
    
    webSocketClient.setConnectionCallback([](const String& deviceId, WSConnectionState state) {
        handleWebSocketConnection(deviceId, state);
    });
    
    DEBUG_PRINTLN("WebSocket client initialized");
}
```

### 3. Create WebSocket Task

Add a dedicated task for WebSocket operations:

```cpp
void webSocketTask(void *parameter) {
    DEBUG_PRINTLN("WebSocket task started on core " + String(xPortGetCoreID()));
    
    while (true) {
        // Update WebSocket connections
        webSocketClient.update();
        
        // Add any periodic WebSocket operations here
        
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms delay
    }
}

// In your task creation function:
xTaskCreatePinnedToCore(
    webSocketTask,
    "WebSocketTask",
    4096,
    NULL,
    2,  // Priority
    NULL,
    0   // Core 0
);
```

### 4. IoT Device Integration

Integrate with your existing IoT device discovery system:

```cpp
// In src/Controllers/IoTDeviceController.cpp
void IoTDeviceController::connectToDiscoveredDevices() {
    auto devices = iotDeviceManager->getDevices();
    
    for (const auto& device : devices) {
        if (device.second.online && device.second.hasWebSocket) {
            String deviceId = device.first;
            String host = device.second.ip.toString();
            
            DEBUG_PRINTF("Connecting WebSocket to device: %s at %s", 
                        deviceId.c_str(), host.c_str());
            
            webSocketClient.connectToDevice(deviceId, host, 81, "/ws", true);
        }
    }
}
```

### 5. Message Handlers

Create specific handlers for different message types:

```cpp
void handleWebSocketMessage(const String& deviceId, WSMessageType type, const JsonDocument& data) {
    DEBUG_PRINTF("WebSocket message from %s, type: %d", deviceId.c_str(), (int)type);
    
    switch (type) {
        case WSMessageType::DEVICE_STATUS:
            updateDeviceStatus(deviceId, data);
            break;
            
        case WSMessageType::CAMERA_STREAM:
            handleCameraStream(deviceId, data);
            break;
            
        case WSMessageType::SENSOR_DATA:
            handleSensorData(deviceId, data);
            break;
            
        case WSMessageType::COMMAND:
            handleRemoteCommand(deviceId, data);
            break;
            
        case WSMessageType::RESPONSE:
            handleCommandResponse(deviceId, data);
            break;
            
        default:
            DEBUG_PRINTF("Unhandled WebSocket message type: %d", (int)type);
    }
}

void handleWebSocketConnection(const String& deviceId, WSConnectionState state) {
    DEBUG_PRINTF("WebSocket connection %s: %d", deviceId.c_str(), (int)state);
    
    switch (state) {
        case WSConnectionState::CONNECTED:
            DEBUG_PRINTF("Connected to device: %s", deviceId.c_str());
            // Send initial device status
            sendDeviceCapabilities(deviceId);
            break;
            
        case WSConnectionState::DISCONNECTED:
            DEBUG_PRINTF("Disconnected from device: %s", deviceId.c_str());
            // Update device status in IoT manager
            updateDeviceOnlineStatus(deviceId, false);
            break;
            
        case WSConnectionState::RECONNECTING:
            DEBUG_PRINTF("Reconnecting to device: %s", deviceId.c_str());
            break;
            
        case WSConnectionState::ERROR_STATE:
            DEBUG_PRINTF("Connection error with device: %s", deviceId.c_str());
            updateDeviceOnlineStatus(deviceId, false);
            break;
    }
}
```

### 6. Camera Stream Integration

For camera streaming, integrate with your existing camera system:

```cpp
void handleCameraStream(const String& deviceId, const JsonDocument& data) {
    String imageUrl = data["data"]["url"];
    int width = data["data"]["width"];
    int height = data["data"]["height"];
    
    // Use your existing HTTP client to fetch the image
    if (httpClient->get(imageUrl)) {
        std::vector<uint8_t> imageData = httpClient->getBinaryResponse();
        
        // Display using your existing JPEG decoder
        displayJpegOnTFT(imageData.data(), imageData.size());
    }
}

// Send camera stream request
void requestCameraStream(const String& deviceId) {
    JsonDocument params;
    params["quality"] = 80;
    params["width"] = 240;
    params["height"] = 320;
    
    webSocketClient.sendCommand(deviceId, "start_stream", params);
}
```

### 7. Menu Integration

Add WebSocket commands to your menu system:

```cpp
// In src/Tasks/Menus/
void showWebSocketMenu() {
    std::vector<String> connectedDevices = webSocketClient.getConnectedDevices();
    
    for (size_t i = 0; i < connectedDevices.size(); i++) {
        String deviceId = connectedDevices[i];
        displayManager->tft.println(String(i + 1) + ". " + deviceId);
    }
    
    // Handle device selection and command sending
}

void sendCommandToSelectedDevice(const String& deviceId, const String& command) {
    JsonDocument params;
    
    if (command == "take_photo") {
        params["quality"] = 90;
        params["format"] = "jpeg";
    } else if (command == "get_sensors") {
        params["sensors"] = JsonArray();
        params["sensors"].add("temperature");
        params["sensors"].add("humidity");
    }
    
    webSocketClient.sendCommand(deviceId, command, params);
}
```

### 8. Configuration

Add WebSocket settings to your configuration system:

```cpp
// Add to your config structure
struct WebSocketConfig {
    bool enabled = true;
    int pingInterval = 30000;
    int pongTimeout = 10000;
    int maxReconnects = 3;
    int reconnectDelay = 5000;
    bool autoConnect = true;
};

// Save/load with your existing config system
void saveWebSocketConfig(const WebSocketConfig& config);
WebSocketConfig loadWebSocketConfig();
```

## API Routes Integration

Add WebSocket management to your API:

```cpp
// In src/Routes/websocket.cpp
void registerWebSocketRoutes(Router* router) {
    router->group("/api/v1/websocket", [&](Router& ws) {
        ws.middleware({"cors", "json"});
        
        ws.get("/connections", [](Request& request) -> Response {
            JsonDocument response;
            response["status"] = "success";
            
            std::vector<String> devices = webSocketClient.getConnectedDevices();
            JsonArray connections = response["data"]["connections"].to<JsonArray>();
            
            for (const String& deviceId : devices) {
                JsonObject conn = connections.createNestedObject();
                conn["device_id"] = deviceId;
                conn["state"] = "connected";
                
                WSDeviceConnection* info = webSocketClient.getConnectionInfo(deviceId);
                if (info) {
                    conn["host"] = info->host;
                    conn["port"] = info->port;
                    conn["connected_since"] = millis() - info->connectTime;
                }
            }
            
            return json(request.getServerRequest(), response);
        });
        
        ws.post("/connect", [](Request& request) -> Response {
            JsonDocument body = request.getJsonBody();
            String deviceId = body["device_id"];
            String host = body["host"];
            int port = body["port"] | 81;
            
            bool success = webSocketClient.connectToDevice(deviceId, host, port);
            
            JsonDocument response;
            response["status"] = success ? "success" : "error";
            response["message"] = success ? "Connection initiated" : "Failed to connect";
            
            return json(request.getServerRequest(), response);
        });
        
        ws.post("/disconnect", [](Request& request) -> Response {
            JsonDocument body = request.getJsonBody();
            String deviceId = body["device_id"];
            
            webSocketClient.disconnectFromDevice(deviceId);
            
            JsonDocument response;
            response["status"] = "success";
            response["message"] = "Device disconnected";
            
            return json(request.getServerRequest(), response);
        });
        
        ws.post("/command", [](Request& request) -> Response {
            JsonDocument body = request.getJsonBody();
            String deviceId = body["device_id"];
            String command = body["command"];
            JsonDocument params = body["parameters"];
            
            bool sent = webSocketClient.sendCommand(deviceId, command, params);
            
            JsonDocument response;
            response["status"] = sent ? "success" : "error";
            response["message"] = sent ? "Command sent" : "Failed to send command";
            
            return json(request.getServerRequest(), response);
        });
    });
}
```

This integration provides real-time communication between your ESP32 devices, enhancing your IoT system with immediate command execution, live sensor data streaming, and camera feed sharing.
