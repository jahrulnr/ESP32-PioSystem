# WebSocket Client Example

This example demonstrates how to use the WebSocketClient library to connect to other ESP32 devices and exchange messages.

## Hardware Requirements
- ESP32-S3 development board
- WiFi connection

## Software Dependencies
- ArduinoWebsockets library (automatically installed via platformio.ini)
- ArduinoJson library
- WiFiManager (for network connectivity)

## Usage Example

```cpp
#include <WiFi.h>
#include <ArduinoJson.h>
#include "websocket_client.h"

// Create WebSocket client instance
WebSocketClientManager wsClient;

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi first
    WiFi.begin("YourWiFiSSID", "YourWiFiPassword");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected!");
    
    // Initialize WebSocket client
    wsClient.begin();
    wsClient.setDebugOutput(true);
    
    // Set message callback
    wsClient.setMessageCallback([](const String& deviceId, WSMessageType type, const JsonDocument& data) {
        Serial.printf("Message from %s: Type=%d\n", deviceId.c_str(), (int)type);
        
        switch (type) {
            case WSMessageType::DEVICE_STATUS:
                handleDeviceStatus(deviceId, data);
                break;
            case WSMessageType::COMMAND:
                handleCommand(deviceId, data);
                break;
            case WSMessageType::SENSOR_DATA:
                handleSensorData(deviceId, data);
                break;
            case WSMessageType::CAMERA_STREAM:
                handleCameraStream(deviceId, data);
                break;
            default:
                Serial.printf("Unhandled message type: %d\n", (int)type);
        }
    });
    
    // Set connection state callback
    wsClient.setConnectionCallback([](const String& deviceId, WSConnectionState state) {
        Serial.printf("Device %s connection state: %d\n", deviceId.c_str(), (int)state);
        
        switch (state) {
            case WSConnectionState::CONNECTED:
                Serial.printf("Connected to device %s\n", deviceId.c_str());
                // Send initial device status
                sendDeviceStatus(deviceId);
                break;
            case WSConnectionState::DISCONNECTED:
                Serial.printf("Disconnected from device %s\n", deviceId.c_str());
                break;
            case WSConnectionState::RECONNECTING:
                Serial.printf("Reconnecting to device %s\n", deviceId.c_str());
                break;
            case WSConnectionState::ERROR_STATE:
                Serial.printf("Connection error with device %s\n", deviceId.c_str());
                break;
        }
    });
    
    // Connect to other devices
    connectToDevices();
}

void loop() {
    // Update WebSocket connections
    wsClient.update();
    
    // Example: Send sensor data every 10 seconds
    static unsigned long lastSensorSend = 0;
    if (millis() - lastSensorSend > 10000) {
        sendSensorDataToAll();
        lastSensorSend = millis();
    }
    
    delay(100);
}

void connectToDevices() {
    // Connect to camera device
    wsClient.connectToDevice("esp32_camera_01", "192.168.4.100", 81, "/ws");
    
    // Connect to sensor device
    wsClient.connectToDevice("esp32_sensor_01", "192.168.4.101", 81, "/ws");
    
    // Connect to display device
    wsClient.connectToDevice("esp32_display_01", "192.168.4.102", 81, "/ws");
}

void handleDeviceStatus(const String& deviceId, const JsonDocument& data) {
    Serial.printf("Device status from %s:\n", deviceId.c_str());
    serializeJsonPretty(data["data"], Serial);
    Serial.println();
    
    // Extract device info
    String deviceType = data["data"]["type"];
    float battery = data["data"]["battery"];
    bool online = data["data"]["online"];
    
    Serial.printf("Type: %s, Battery: %.1f%%, Online: %s\n", 
                  deviceType.c_str(), battery, online ? "Yes" : "No");
}

void handleCommand(const String& deviceId, const JsonDocument& data) {
    String command = data["data"]["command"];
    String requestId = data["data"]["id"];
    
    Serial.printf("Command from %s: %s (ID: %s)\n", 
                  deviceId.c_str(), command.c_str(), requestId.c_str());
    
    // Process command
    JsonDocument response;
    bool success = false;
    
    if (command == "get_status") {
        response["battery"] = 85.5;
        response["temperature"] = 24.3;
        response["uptime"] = millis();
        success = true;
    } else if (command == "take_photo") {
        response["message"] = "Photo captured";
        response["filename"] = "photo_" + String(millis()) + ".jpg";
        success = true;
    } else if (command == "restart") {
        response["message"] = "Restarting device...";
        success = true;
        // Schedule restart after response
    } else {
        response["error"] = "Unknown command: " + command;
    }
    
    // Send response
    wsClient.sendResponse(deviceId, requestId, success, response);
}

void handleSensorData(const String& deviceId, const JsonDocument& data) {
    Serial.printf("Sensor data from %s:\n", deviceId.c_str());
    
    JsonObject sensors = data["data"]["sensors"];
    for (JsonPair sensor : sensors) {
        Serial.printf("  %s: %s\n", sensor.key().c_str(), sensor.value().as<String>().c_str());
    }
}

void handleCameraStream(const String& deviceId, const JsonDocument& data) {
    String imageUrl = data["data"]["url"];
    int width = data["data"]["width"];
    int height = data["data"]["height"];
    String format = data["data"]["format"];
    
    Serial.printf("Camera stream from %s: %dx%d %s at %s\n", 
                  deviceId.c_str(), width, height, format.c_str(), imageUrl.c_str());
    
    // Process camera stream data
    // You could download and display the image here
}

void sendDeviceStatus(const String& targetDeviceId) {
    JsonDocument statusData;
    statusData["type"] = "ESP32_CONTROLLER";
    statusData["battery"] = 85.5;
    statusData["online"] = true;
    statusData["capabilities"] = JsonArray();
    statusData["capabilities"].add("DISPLAY_SCREEN");
    statusData["capabilities"].add("BUTTON_INPUT");
    statusData["capabilities"].add("WIFI_HOTSPOT");
    
    wsClient.sendMessage(targetDeviceId, WSMessageType::DEVICE_STATUS, statusData);
}

void sendSensorDataToAll() {
    JsonDocument sensorData;
    sensorData["sensors"]["temperature"] = 24.3;
    sensorData["sensors"]["humidity"] = 65.2;
    sensorData["sensors"]["battery"] = 85.5;
    sensorData["sensors"]["uptime"] = millis();
    
    // Send to all connected devices
    std::vector<String> connectedDevices = wsClient.getConnectedDevices();
    for (const String& deviceId : connectedDevices) {
        wsClient.sendMessage(deviceId, WSMessageType::SENSOR_DATA, sensorData);
    }
}

void sendCommandToDevice(const String& deviceId, const String& command) {
    JsonDocument commandData;
    commandData["command"] = command;
    commandData["timestamp"] = millis();
    
    if (command == "take_photo") {
        commandData["parameters"]["quality"] = 80;
        commandData["parameters"]["resolution"] = "800x600";
    } else if (command == "get_sensors") {
        commandData["parameters"]["include"] = JsonArray();
        commandData["parameters"]["include"].add("temperature");
        commandData["parameters"]["include"].add("humidity");
    }
    
    wsClient.sendCommand(deviceId, command, commandData["parameters"]);
}
```

## Message Types

The WebSocket client supports various message types:

- **PING/PONG**: Keep-alive messages
- **DEVICE_STATUS**: Device information and capabilities
- **COMMAND**: Command execution requests
- **RESPONSE**: Command execution responses
- **SENSOR_DATA**: Sensor readings and telemetry
- **CAMERA_STREAM**: Camera image data and metadata
- **CUSTOM**: Application-specific messages

## Connection Management

The client automatically handles:
- Connection establishment
- Automatic reconnection on failure
- Ping/pong keep-alive messages
- Connection state monitoring
- Multiple device connections

## Error Handling

The library includes comprehensive error handling:
- Connection timeouts
- Message parsing errors
- Network disconnections
- Automatic retry mechanisms

## Performance Considerations

- The library uses FreeRTOS tasks for background operations
- Ping and reconnection tasks run on Core 0
- Message callbacks are executed synchronously
- Use appropriate buffer sizes for large messages
- Consider message frequency to avoid network congestion
