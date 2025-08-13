/**
 * @file custom_driver.cpp
 * @brief Example of creating a custom device driver
 * 
 * This example shows how to:
 * 1. Create a custom device driver for a specific IoT device
 * 2. Implement device probing and capability detection
 * 3. Handle device-specific commands
 * 4. Register and use the custom driver
 */

#include "iot_device_manager.h"
#include "drivers/device_driver.h"
#include "SerialDebug.h"

/**
 * @brief Custom driver for Arduino IoT devices with sensor capabilities
 * 
 * This driver handles Arduino-based IoT devices that expose sensor data
 * through a REST API with specific endpoints.
 */
class ArduinoSensorDriver : public DeviceDriver {
public:
    DeviceType getDeviceType() const override {
        return DeviceType::ARDUINO_IOT;
    }
    
    String getDriverName() const override {
        return "ArduinoSensorDriver";
    }
    
    bool canHandle(const IoTDevice& device) const override {
        return device.type == DeviceType::ARDUINO_IOT;
    }
    
    bool probe(IoTDevice& device, HttpClientManager& httpClient) override {
        DEBUG_PRINTF("ArduinoSensorDriver: Probing device %s\n", device.ipAddress.c_str());
        
        // Check for Arduino-specific endpoints
        std::vector<String> arduinoEndpoints = {
            "/api/sensors",
            "/api/arduino/info",
            "/sensors/temperature",
            "/sensors/humidity"
        };
        
        int foundEndpoints = 0;
        for (const String& endpoint : arduinoEndpoints) {
            if (checkEndpoint(device.baseUrl, endpoint, httpClient)) {
                foundEndpoints++;
                device.endpoints.push_back(ApiEndpoint(endpoint, "GET", "Arduino sensor endpoint"));
            }
        }
        
        // Need at least 2 Arduino-specific endpoints to confirm it's an Arduino device
        if (foundEndpoints >= 2) {
            uint32_t capabilities = static_cast<uint32_t>(DeviceCapability::SENSORS) |
                                   static_cast<uint32_t>(DeviceCapability::NETWORKING);
            
            // Check for additional capabilities
            if (checkEndpoint(device.baseUrl, "/api/actuators", httpClient)) {
                capabilities |= static_cast<uint32_t>(DeviceCapability::ACTUATORS);
                device.endpoints.push_back(ApiEndpoint("/api/actuators", "POST", "Actuator control"));
            }
            
            if (checkEndpoint(device.baseUrl, "/api/storage", httpClient)) {
                capabilities |= static_cast<uint32_t>(DeviceCapability::STORAGE);
            }
            
            // Set device properties
            setDeviceProperties(device, DeviceType::ARDUINO_IOT, capabilities, "Arduino Sensor Device");
            
            // Try to get device info to set more specific name
            HttpResponse infoResponse = httpClient.get(device.baseUrl + "/api/arduino/info");
            if (infoResponse.statusCode == 200) {
                JsonDocument info;
                if (deserializeJson(info, infoResponse.body) == DeserializationError::Ok) {
                    if (info.containsKey("device_name")) {
                        device.name = info["device_name"].as<String>();
                    }
                    if (info.containsKey("model")) {
                        device.model = info["model"].as<String>();
                    }
                    if (info.containsKey("firmware_version")) {
                        device.firmwareVersion = info["firmware_version"].as<String>();
                    }
                }
            }
            
            DEBUG_PRINTF("ArduinoSensorDriver: Successfully identified Arduino device %s\n", 
                        device.ipAddress.c_str());
            return true;
        }
        
        return false;
    }
    
    JsonDocument getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) override {
        JsonDocument info;
        info["type"] = "ARDUINO_IOT";
        info["name"] = device.name;
        info["driver"] = getDriverName();
        info["model"] = device.model;
        info["firmware_version"] = device.firmwareVersion;
        
        // Add capabilities
        JsonArray capArray = info["capabilities"].to<JsonArray>();
        std::vector<String> caps = DeviceTypeUtils::capabilitiesToStrings(device.capabilities);
        for (const String& cap : caps) {
            capArray.add(cap);
        }
        
        // Get current sensor readings
        HttpResponse sensorsResponse = httpClient.get(device.baseUrl + "/api/sensors");
        if (sensorsResponse.statusCode == 200) {
            JsonDocument sensorData;
            if (deserializeJson(sensorData, sensorsResponse.body) == DeserializationError::Ok) {
                info["sensor_data"] = sensorData;
            }
        }
        
        // Get available endpoints
        JsonArray endpointsArray = info["endpoints"].to<JsonArray>();
        for (const auto& endpoint : device.endpoints) {
            JsonObject ep = endpointsArray.createNestedObject();
            ep["path"] = endpoint.path;
            ep["method"] = endpoint.method;
            ep["description"] = endpoint.description;
        }
        
        return info;
    }
    
    bool executeCommand(const IoTDevice& device, const String& command, 
                       const JsonDocument& params, HttpClientManager& httpClient) override {
        DEBUG_PRINTF("ArduinoSensorDriver: Executing command '%s' on device %s\n", 
                    command.c_str(), device.ipAddress.c_str());
        
        if (command == "read_sensors") {
            String url = device.baseUrl + "/api/sensors";
            HttpResponse response = httpClient.get(url);
            return response.statusCode >= 200 && response.statusCode < 300;
        }
        else if (command == "read_temperature") {
            String url = device.baseUrl + "/sensors/temperature";
            HttpResponse response = httpClient.get(url);
            return response.statusCode >= 200 && response.statusCode < 300;
        }
        else if (command == "read_humidity") {
            String url = device.baseUrl + "/sensors/humidity";
            HttpResponse response = httpClient.get(url);
            return response.statusCode >= 200 && response.statusCode < 300;
        }
        else if (command == "control_actuator") {
            if (!params.containsKey("actuator") || !params.containsKey("value")) {
                DEBUG_PRINTLN("ArduinoSensorDriver: Missing actuator or value parameter");
                return false;
            }
            
            String url = device.baseUrl + "/api/actuators";
            
            // Create control payload
            JsonDocument payload;
            payload["actuator"] = params["actuator"];
            payload["value"] = params["value"];
            
            String payloadStr;
            serializeJson(payload, payloadStr);
            
            HttpResponse response = httpClient.post(url, payloadStr);
            return response.statusCode >= 200 && response.statusCode < 300;
        }
        else if (command == "reset_device") {
            String url = device.baseUrl + "/api/system/reset";
            HttpResponse response = httpClient.post(url, "");
            return response.statusCode >= 200 && response.statusCode < 300;
        }
        
        DEBUG_PRINTF("ArduinoSensorDriver: Unknown command '%s'\n", command.c_str());
        return false;
    }
};

// Example usage in main application
void setupCustomDriver() {
    DEBUG_PRINTLN("Setting up custom Arduino sensor driver");
    
    // Assuming you have these already initialized
    extern WiFiManager wifiManager;
    extern HttpClientManager httpClient;
    extern IoTDeviceManager* iotManager;
    
    // Create and register the custom driver
    ArduinoSensorDriver* arduinoDriver = new ArduinoSensorDriver();
    iotManager->registerDriver(arduinoDriver);
    
    DEBUG_PRINTLN("Custom driver registered successfully");
}

void demonstrateCustomDriver() {
    extern IoTDeviceManager* iotManager;
    
    // Find Arduino devices
    std::vector<IoTDevice*> arduinoDevices = iotManager->getDevicesByType(DeviceType::ARDUINO_IOT);
    
    DEBUG_PRINTF("Found %d Arduino devices\n", arduinoDevices.size());
    
    for (auto* device : arduinoDevices) {
        if (!device->isOnline) continue;
        
        DEBUG_PRINTF("Testing Arduino device: %s\n", device->name.c_str());
        
        // Read sensor data
        JsonDocument response = iotManager->executeDeviceCommand(device->id, "read_sensors", JsonDocument());
        if (response["success"].as<bool>()) {
            DEBUG_PRINTLN("Successfully read sensor data");
        }
        
        // Control an actuator (if available)
        if (device->capabilities & static_cast<uint32_t>(DeviceCapability::ACTUATORS)) {
            JsonDocument actuatorParams;
            actuatorParams["actuator"] = "led";
            actuatorParams["value"] = true;
            
            response = iotManager->executeDeviceCommand(device->id, "control_actuator", actuatorParams);
            if (response["success"].as<bool>()) {
                DEBUG_PRINTLN("Successfully controlled actuator");
            }
        }
        
        // Get detailed device info
        JsonDocument deviceInfo;
        // Note: This would require adding a getDeviceInfo method to IoTDeviceManager
        // For now, we can access the driver directly through the manager
    }
}

// Advanced: Creating a driver that handles multiple device types
class MultiProtocolDriver : public DeviceDriver {
private:
    DeviceType detectedType;
    
public:
    DeviceType getDeviceType() const override {
        return detectedType;
    }
    
    String getDriverName() const override {
        return "MultiProtocolDriver";
    }
    
    bool canHandle(const IoTDevice& device) const override {
        // This driver can handle multiple types after probing
        return device.type == DeviceType::ARDUINO_IOT || 
               device.type == DeviceType::RASPBERRY_PI ||
               device.type == DeviceType::CUSTOM_DEVICE;
    }
    
    bool probe(IoTDevice& device, HttpClientManager& httpClient) override {
        // First, try to detect if it's a Raspberry Pi
        if (checkEndpoint(device.baseUrl, "/api/pi/info", httpClient) ||
            checkEndpoint(device.baseUrl, "/raspberry", httpClient)) {
            
            detectedType = DeviceType::RASPBERRY_PI;
            uint32_t capabilities = static_cast<uint32_t>(DeviceCapability::NETWORKING) |
                                   static_cast<uint32_t>(DeviceCapability::SYSTEM_CONTROL);
            
            setDeviceProperties(device, DeviceType::RASPBERRY_PI, capabilities, "Raspberry Pi");
            return true;
        }
        
        // Check for Arduino characteristics
        if (checkEndpoint(device.baseUrl, "/arduino", httpClient) ||
            checkEndpoint(device.baseUrl, "/api/arduino", httpClient)) {
            
            detectedType = DeviceType::ARDUINO_IOT;
            uint32_t capabilities = static_cast<uint32_t>(DeviceCapability::SENSORS) |
                                   static_cast<uint32_t>(DeviceCapability::NETWORKING);
            
            setDeviceProperties(device, DeviceType::ARDUINO_IOT, capabilities, "Arduino Device");
            return true;
        }
        
        return false;
    }
    
    JsonDocument getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) override {
        JsonDocument info;
        info["driver"] = getDriverName();
        info["detected_type"] = DeviceTypeUtils::deviceTypeToString(device.type);
        
        // Type-specific info gathering
        if (device.type == DeviceType::RASPBERRY_PI) {
            HttpResponse response = httpClient.get(device.baseUrl + "/api/pi/info");
            if (response.statusCode == 200) {
                JsonDocument piInfo;
                deserializeJson(piInfo, response.body);
                info["pi_info"] = piInfo;
            }
        }
        
        return info;
    }
    
    bool executeCommand(const IoTDevice& device, const String& command, 
                       const JsonDocument& params, HttpClientManager& httpClient) override {
        // Route commands based on device type
        if (device.type == DeviceType::RASPBERRY_PI) {
            return executeRaspberryPiCommand(device, command, params, httpClient);
        } else if (device.type == DeviceType::ARDUINO_IOT) {
            return executeArduinoCommand(device, command, params, httpClient);
        }
        
        return false;
    }
    
private:
    bool executeRaspberryPiCommand(const IoTDevice& device, const String& command, 
                                  const JsonDocument& params, HttpClientManager& httpClient) {
        if (command == "system_info") {
            HttpResponse response = httpClient.get(device.baseUrl + "/api/pi/system");
            return response.statusCode >= 200 && response.statusCode < 300;
        }
        return false;
    }
    
    bool executeArduinoCommand(const IoTDevice& device, const String& command, 
                              const JsonDocument& params, HttpClientManager& httpClient) {
        if (command == "digital_read") {
            String pin = params["pin"].as<String>();
            String url = device.baseUrl + "/api/arduino/digital/" + pin;
            HttpResponse response = httpClient.get(url);
            return response.statusCode >= 200 && response.statusCode < 300;
        }
        return false;
    }
};
