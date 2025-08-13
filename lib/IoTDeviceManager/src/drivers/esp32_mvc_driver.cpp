#include "esp32_mvc_driver.h"
#include "SerialDebug.h"

std::vector<String> ESP32MVCDriver::getMVCEndpoints() const {
    return {
        "/api/v1/auth/user",
        "/api/v1/system/stats",
        "/api/wifi/status",
        "/api/v1/iot/devices"
    };
}

bool ESP32MVCDriver::probe(IoTDevice& device, HttpClientManager& httpClient) {
    DEBUG_PRINTF("ESP32MVCDriver: Probing device %s\n", device.ipAddress.c_str());
    
    std::vector<String> mvcEndpoints = getMVCEndpoints();
    int foundEndpoints = 0;
    
    for (const String& endpoint : mvcEndpoints) {
        if (checkEndpoint(device.baseUrl, endpoint, httpClient)) {
            foundEndpoints++;
            device.endpoints.push_back(ApiEndpoint(endpoint, "GET", "MVC API endpoint"));
        }
    }
    
    if (foundEndpoints >= 2) {
        uint32_t capabilities = static_cast<uint32_t>(DeviceCapability::NETWORKING) |
                               static_cast<uint32_t>(DeviceCapability::SYSTEM_CONTROL) |
                               static_cast<uint32_t>(DeviceCapability::AUTHENTICATION);
        
        // Check for WebSocket capability
        if (checkEndpoint(device.baseUrl, "/ws", httpClient)) {
            capabilities |= static_cast<uint32_t>(DeviceCapability::WEBSOCKET);
        }
        
        // Check for IoT device management capability
        if (checkEndpoint(device.baseUrl, "/api/v1/iot/devices", httpClient)) {
            device.endpoints.push_back(ApiEndpoint("/api/v1/iot/devices", "GET", "IoT device management"));
        }
        
        setDeviceProperties(device, DeviceType::ESP32_CONTROLLER, capabilities, "ESP32 Controller");
        
        DEBUG_PRINTF("ESP32MVCDriver: Successfully identified MVC device %s\n", device.ipAddress.c_str());
        return true;
    }
    
    return false;
}

JsonDocument ESP32MVCDriver::getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) {
    JsonDocument info;
    info["type"] = "ESP32_CONTROLLER";
    info["name"] = device.name;
    info["driver"] = getDriverName();
    
    JsonArray capArray = info["capabilities"].to<JsonArray>();
    std::vector<String> caps = DeviceTypeUtils::capabilitiesToStrings(device.capabilities);
    for (const String& cap : caps) {
        capArray.add(cap);
    }
    
    // Try to get system status
    HttpResponse response = httpClient.get(device.baseUrl + "/api/v1/system/stats");
    if (response.statusCode == 200) {
        JsonDocument systemStats;
        if (deserializeJson(systemStats, response.body) == DeserializationError::Ok) {
            info["system_stats"] = systemStats;
        }
    }
    
    return info;
}

bool ESP32MVCDriver::executeCommand(const IoTDevice& device, const String& command, 
                                   const JsonDocument& params, HttpClientManager& httpClient) {
    DEBUG_PRINTF("ESP32MVCDriver: Executing command '%s' on device %s\n", 
                command.c_str(), device.ipAddress.c_str());
    
    if (command == "system_restart") {
        String url = device.baseUrl + "/api/v1/system/restart";
        HttpResponse response = httpClient.post(url, "");
        return response.statusCode >= 200 && response.statusCode < 300;
    }
    else if (command == "get_wifi_status") {
        String url = device.baseUrl + "/api/wifi/status";
        HttpResponse response = httpClient.get(url);
        return response.statusCode >= 200 && response.statusCode < 300;
    }
    else if (command == "get_iot_devices") {
        String url = device.baseUrl + "/api/v1/iot/devices";
        HttpResponse response = httpClient.get(url);
        return response.statusCode >= 200 && response.statusCode < 300;
    }
    
    DEBUG_PRINTF("ESP32MVCDriver: Unknown command '%s'\n", command.c_str());
    return false;
}
