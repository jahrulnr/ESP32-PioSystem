#include <WiFi.h>
#include <ArduinoJson.h>
#include "../src/websocket_client.h"

// WiFi credentials
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

// WebSocket client instance
WebSocketClientManager wsClient;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("WebSocket Client Basic Example");
    Serial.println("==============================");
    
    // Connect to WiFi
    Serial.printf("Connecting to WiFi: %s", ssid);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println();
    Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    
    // Initialize WebSocket client
    wsClient.begin(
        30000,  // Ping interval: 30 seconds
        10000,  // Pong timeout: 10 seconds
        5,      // Max reconnect attempts
        5000    // Reconnect delay: 5 seconds
    );
    
    wsClient.setDebugOutput(true);
    
    // Set message callback
    wsClient.setMessageCallback([](const String& deviceId, WSMessageType type, const JsonDocument& data) {
        Serial.printf("\n--- Message from %s ---\n", deviceId.c_str());
        Serial.printf("Type: %s\n", getMessageTypeName(type).c_str());
        Serial.printf("Data: ");
        serializeJsonPretty(data, Serial);
        Serial.println("\n");
        
        // Handle specific message types
        handleIncomingMessage(deviceId, type, data);
    });
    
    // Set connection state callback
    wsClient.setConnectionCallback([](const String& deviceId, WSConnectionState state) {
        Serial.printf("\n--- Connection State Change ---\n");
        Serial.printf("Device: %s\n", deviceId.c_str());
        Serial.printf("State: %s\n", getConnectionStateName(state).c_str());
        Serial.println();
        
        if (state == WSConnectionState::CONNECTED) {
            // Send initial status when connected
            sendDeviceStatus(deviceId);
        }
    });
    
    // Connect to example devices
    Serial.println("\nConnecting to devices...");
    
    // Example: Connect to camera device
    wsClient.connectToDevice("esp32_camera", "192.168.4.100", 81, "/ws");
    
    // Example: Connect to sensor device
    wsClient.connectToDevice("esp32_sensor", "192.168.4.101", 81, "/ws");
    
    Serial.println("Setup complete!");
    Serial.println("\nAvailable commands:");
    Serial.println("  status - Show connection status");
    Serial.println("  ping - Send ping to all devices");
    Serial.println("  command:<device>:<cmd> - Send command to device");
    Serial.println("  Example: command:esp32_camera:take_photo");
}

void loop() {
    // Update WebSocket connections
    wsClient.update();
    
    // Handle serial commands
    handleSerialCommands();
    
    // Send periodic sensor data
    static unsigned long lastSensorSend = 0;
    if (millis() - lastSensorSend > 15000) {  // Every 15 seconds
        sendSensorDataToAll();
        lastSensorSend = millis();
    }
    
    delay(100);
}

void handleIncomingMessage(const String& deviceId, WSMessageType type, const JsonDocument& data) {
    switch (type) {
        case WSMessageType::COMMAND: {
            String command = data["data"]["command"];
            String requestId = data["data"]["id"];
            
            Serial.printf("Executing command: %s\n", command.c_str());
            
            // Process command and send response
            JsonDocument response;
            bool success = executeCommand(command, data["data"]["parameters"], response);
            
            wsClient.sendResponse(deviceId, requestId, success, response);
            break;
        }
        
        case WSMessageType::DEVICE_STATUS: {
            Serial.printf("Device %s status updated\n", deviceId.c_str());
            // Handle device status update
            break;
        }
        
        case WSMessageType::SENSOR_DATA: {
            Serial.printf("Received sensor data from %s\n", deviceId.c_str());
            // Process sensor data
            break;
        }
        
        case WSMessageType::CAMERA_STREAM: {
            Serial.printf("Camera stream data from %s\n", deviceId.c_str());
            // Handle camera stream
            break;
        }
        
        default:
            Serial.printf("Unhandled message type: %d\n", (int)type);
    }
}

bool executeCommand(const String& command, const JsonDocument& parameters, JsonDocument& response) {
    if (command == "get_status") {
        response["device_type"] = "ESP32_CONTROLLER";
        response["uptime"] = millis();
        response["free_memory"] = ESP.getFreeHeap();
        response["wifi_rssi"] = WiFi.RSSI();
        response["temperature"] = 25.5;  // Mock temperature
        return true;
        
    } else if (command == "restart") {
        response["message"] = "Device will restart in 3 seconds";
        // Schedule restart
        return true;
        
    } else if (command == "ping") {
        response["message"] = "pong";
        response["timestamp"] = millis();
        return true;
        
    } else {
        response["error"] = "Unknown command: " + command;
        return false;
    }
}

void sendDeviceStatus(const String& targetDeviceId) {
    JsonDocument statusData;
    statusData["device_id"] = WiFi.macAddress();
    statusData["device_type"] = "ESP32_CONTROLLER";
    statusData["ip_address"] = WiFi.localIP().toString();
    statusData["uptime"] = millis();
    statusData["free_memory"] = ESP.getFreeHeap();
    statusData["wifi_rssi"] = WiFi.RSSI();
    statusData["capabilities"] = JsonArray();
    statusData["capabilities"].add("DISPLAY_SCREEN");
    statusData["capabilities"].add("BUTTON_INPUT");
    statusData["capabilities"].add("WIFI_CLIENT");
    
    wsClient.sendMessage(targetDeviceId, WSMessageType::DEVICE_STATUS, statusData);
    Serial.printf("Sent device status to %s\n", targetDeviceId.c_str());
}

void sendSensorDataToAll() {
    JsonDocument sensorData;
    sensorData["timestamp"] = millis();
    sensorData["sensors"]["temperature"] = 25.5 + random(-50, 50) / 10.0;
    sensorData["sensors"]["humidity"] = 60.0 + random(-100, 100) / 10.0;
    sensorData["sensors"]["battery"] = 85.5;
    sensorData["sensors"]["memory_free"] = ESP.getFreeHeap();
    sensorData["sensors"]["wifi_rssi"] = WiFi.RSSI();
    
    std::vector<String> connectedDevices = wsClient.getConnectedDevices();
    for (const String& deviceId : connectedDevices) {
        wsClient.sendMessage(deviceId, WSMessageType::SENSOR_DATA, sensorData);
    }
    
    if (!connectedDevices.empty()) {
        Serial.printf("Sent sensor data to %d devices\n", connectedDevices.size());
    }
}

void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "status") {
            showConnectionStatus();
        } else if (command == "ping") {
            wsClient.pingAll();
            Serial.println("Ping sent to all devices");
        } else if (command.startsWith("command:")) {
            handleSerialCommand(command);
        } else if (command == "help") {
            showHelp();
        } else {
            Serial.println("Unknown command. Type 'help' for available commands.");
        }
    }
}

void handleSerialCommand(const String& fullCommand) {
    // Parse: command:device:cmd
    int firstColon = fullCommand.indexOf(':', 8);
    int secondColon = fullCommand.indexOf(':', firstColon + 1);
    
    if (firstColon == -1 || secondColon == -1) {
        Serial.println("Invalid command format. Use: command:<device>:<cmd>");
        return;
    }
    
    String deviceId = fullCommand.substring(8, firstColon);
    String command = fullCommand.substring(firstColon + 1, secondColon);
    String params = fullCommand.substring(secondColon + 1);
    
    JsonDocument parameters;
    if (!params.isEmpty()) {
        deserializeJson(parameters, params);
    }
    
    bool sent = wsClient.sendCommand(deviceId, command, parameters);
    if (sent) {
        Serial.printf("Command '%s' sent to device '%s'\n", command.c_str(), deviceId.c_str());
    } else {
        Serial.printf("Failed to send command to device '%s' (not connected)\n", deviceId.c_str());
    }
}

void showConnectionStatus() {
    Serial.println("\n--- Connection Status ---");
    Serial.printf("Total connections: %d\n", wsClient.getConnectionCount());
    
    std::vector<String> connectedDevices = wsClient.getConnectedDevices();
    
    if (connectedDevices.empty()) {
        Serial.println("No devices connected");
    } else {
        Serial.println("Connected devices:");
        for (const String& deviceId : connectedDevices) {
            WSDeviceConnection* info = wsClient.getConnectionInfo(deviceId);
            if (info) {
                Serial.printf("  %s: %s:%d%s (connected for %lu ms)\n", 
                              deviceId.c_str(), 
                              info->host.c_str(), 
                              info->port, 
                              info->path.c_str(),
                              millis() - info->connectTime);
            }
        }
    }
    Serial.println();
}

void showHelp() {
    Serial.println("\n--- Available Commands ---");
    Serial.println("status                        - Show connection status");
    Serial.println("ping                          - Send ping to all devices");
    Serial.println("command:<device>:<cmd>        - Send command to device");
    Serial.println("command:<device>:<cmd>:<json> - Send command with parameters");
    Serial.println("help                          - Show this help");
    Serial.println("\nExample commands:");
    Serial.println("command:esp32_camera:take_photo");
    Serial.println("command:esp32_sensor:get_status");
    Serial.println("command:esp32_camera:set_quality:{\"quality\":80}");
    Serial.println();
}

String getMessageTypeName(WSMessageType type) {
    switch (type) {
        case WSMessageType::PING: return "PING";
        case WSMessageType::PONG: return "PONG";
        case WSMessageType::DEVICE_STATUS: return "DEVICE_STATUS";
        case WSMessageType::COMMAND: return "COMMAND";
        case WSMessageType::RESPONSE: return "RESPONSE";
        case WSMessageType::ERROR: return "ERROR";
        case WSMessageType::CAMERA_STREAM: return "CAMERA_STREAM";
        case WSMessageType::SENSOR_DATA: return "SENSOR_DATA";
        case WSMessageType::CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

String getConnectionStateName(WSConnectionState state) {
    switch (state) {
        case WSConnectionState::DISCONNECTED: return "DISCONNECTED";
        case WSConnectionState::CONNECTING: return "CONNECTING";
        case WSConnectionState::CONNECTED: return "CONNECTED";
        case WSConnectionState::RECONNECTING: return "RECONNECTING";
        case WSConnectionState::ERROR_STATE: return "ERROR_STATE";
        default: return "UNKNOWN";
    }
}
