/**
 * @file basic_usage.cpp
 * @brief Basic IoT Device Manager usage example
 * 
 * This example shows how to:
 * 1. Initialize the IoT Device Manager
 * 2. Start device discovery
 * 3. Handle discovered devices
 * 4. Query device information
 */

#include "iot_device_manager.h"
#include "wifi_manager.h"
#include "httpclient.h"
#include "SerialDebug.h"

// Global instances (normally these would be in your main application)
WiFiManager wifiManager("MyIoTHub", "password123");
HttpClientManager httpClient;
IoTDeviceManager* iotManager;

void deviceDiscoveredCallback(const IoTDevice& device) {
    DEBUG_PRINTF("New device discovered: %s\n", device.name.c_str());
    DEBUG_PRINTF("  IP: %s\n", device.ipAddress.c_str());
    DEBUG_PRINTF("  Type: %s\n", IoTDeviceManager::deviceTypeToString(device.type).c_str());
    DEBUG_PRINTF("  Capabilities: %d\n", device.capabilities);
    
    // Print capabilities as strings
    std::vector<String> caps = IoTDeviceManager::capabilitiesToStrings(device.capabilities);
    DEBUG_PRINTF("  Capabilities: ");
    for (size_t i = 0; i < caps.size(); i++) {
        DEBUG_PRINTF("%s", caps[i].c_str());
        if (i < caps.size() - 1) DEBUG_PRINTF(", ");
    }
    DEBUG_PRINTF("\n");
}

void deviceStatusCallback(const IoTDevice& device, bool isOnline) {
    DEBUG_PRINTF("Device %s is now %s\n", 
                device.name.c_str(), isOnline ? "online" : "offline");
}

void setup() {
    DEBUG_BEGIN(115200);
    DEBUG_PRINTLN("IoT Device Manager Basic Example");
    
    // Initialize WiFi in AP mode
    if (wifiManager.startAccessPoint()) {
        DEBUG_PRINTLN("WiFi AP started successfully");
        DEBUG_PRINTF("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    }
    
    // Initialize HTTP client
    httpClient.begin();
    
    // Create and initialize IoT Device Manager
    iotManager = new IoTDeviceManager(&wifiManager, &httpClient);
    
    if (iotManager->begin()) {
        DEBUG_PRINTLN("IoT Device Manager initialized");
        
        // Set up callbacks
        iotManager->setDeviceDiscoveredCallback(deviceDiscoveredCallback);
        iotManager->setDeviceStatusCallback(deviceStatusCallback);
        
        // Start discovery with 30 second intervals
        iotManager->startDiscovery(30000);
        DEBUG_PRINTLN("Device discovery started");
    } else {
        DEBUG_PRINTLN("Failed to initialize IoT Device Manager");
    }
}

void loop() {
    // Print statistics every 60 seconds
    static unsigned long lastStatsTime = 0;
    if (millis() - lastStatsTime >= 60000) {
        printDeviceStatistics();
        lastStatsTime = millis();
    }
    
    // Check for camera devices and request photos every 2 minutes
    static unsigned long lastCameraCheck = 0;
    if (millis() - lastCameraCheck >= 120000) {
        requestPhotosFromCameras();
        lastCameraCheck = millis();
    }
    
    delay(1000);
}

void printDeviceStatistics() {
    JsonDocument stats = iotManager->getStatistics();
    
    DEBUG_PRINTLN("\n=== Device Statistics ===");
    DEBUG_PRINTF("Total devices: %d\n", stats["total_devices"].as<int>());
    DEBUG_PRINTF("Online devices: %d\n", stats["online_devices"].as<int>());
    DEBUG_PRINTF("Total scans: %d\n", stats["total_scans"].as<int>());
    DEBUG_PRINTF("Discovery enabled: %s\n", stats["discovery_enabled"].as<bool>() ? "yes" : "no");
    
    // Print device types
    JsonObject types = stats["device_types"].as<JsonObject>();
    DEBUG_PRINTLN("Device types:");
    for (JsonPair type : types) {
        DEBUG_PRINTF("  %s: %d\n", type.key().c_str(), type.value().as<int>());
    }
    
    // List all discovered devices
    std::vector<IoTDevice> devices = iotManager->getDevices();
    DEBUG_PRINTLN("All devices:");
    for (const auto& device : devices) {
        DEBUG_PRINTF("  %s (%s) - %s - %s\n", 
                    device.name.c_str(), 
                    device.ipAddress.c_str(),
                    IoTDeviceManager::deviceTypeToString(device.type).c_str(),
                    device.isOnline ? "online" : "offline");
    }
    DEBUG_PRINTLN("========================\n");
}

void requestPhotosFromCameras() {
    // Find all camera devices
    std::vector<IoTDevice*> cameras = iotManager->getDevicesWithCapability(DeviceCapability::CAMERA);
    
    if (cameras.empty()) {
        DEBUG_PRINTLN("No camera devices found");
        return;
    }
    
    DEBUG_PRINTF("Found %d camera device(s), requesting photos...\n", cameras.size());
    
    for (auto* camera : cameras) {
        if (!camera->isOnline) {
            DEBUG_PRINTF("Camera %s is offline, skipping\n", camera->name.c_str());
            continue;
        }
        
        // Create capture parameters
        JsonDocument params;
        params["quality"] = 80;
        params["format"] = "jpeg";
        
        // Execute capture command
        JsonDocument response = iotManager->executeDeviceCommand(camera->id, "capture", params);
        
        if (response["success"].as<bool>()) {
            DEBUG_PRINTF("Photo capture successful on %s\n", camera->name.c_str());
        } else {
            DEBUG_PRINTF("Photo capture failed on %s: %s\n", 
                        camera->name.c_str(), 
                        response["error"].as<String>().c_str());
        }
    }
}

void listOnlineDevices() {
    std::vector<IoTDevice> onlineDevices = iotManager->getOnlineDevices();
    
    DEBUG_PRINTF("Online devices (%d):\n", onlineDevices.size());
    for (const auto& device : onlineDevices) {
        DEBUG_PRINTF("  %s (%s) - Last seen: %lu ms ago\n",
                    device.name.c_str(),
                    device.ipAddress.c_str(),
                    millis() - device.lastSeen);
    }
}

void demonstrateDeviceQueries() {
    // Get devices by type
    std::vector<IoTDevice*> controllers = iotManager->getDevicesByType(DeviceType::ESP32_CONTROLLER);
    DEBUG_PRINTF("Found %d ESP32 controllers\n", controllers.size());
    
    // Get devices with specific capabilities
    std::vector<IoTDevice*> webSocketDevices = iotManager->getDevicesWithCapability(DeviceCapability::WEBSOCKET);
    DEBUG_PRINTF("Found %d devices with WebSocket capability\n", webSocketDevices.size());
    
    // Get device by IP
    IoTDevice* device = iotManager->getDeviceByIP("192.168.4.100");
    if (device != nullptr) {
        DEBUG_PRINTF("Found device at 192.168.4.100: %s\n", device->name.c_str());
    }
}
