#include "esp32_camera_driver.h"
#include "SerialDebug.h"

std::vector<String> ESP32CameraDriver::getCameraEndpoints() const {
    return {
        "/api/v1/camera/status",
        "/api/v1/camera/capture", 
        "/api/v1/camera/settings",
        "/api/v1/camera/stream"
    };
}

bool ESP32CameraDriver::probe(IoTDevice& device, HttpClientManager& httpClient) {
    DEBUG_PRINTF("ESP32CameraDriver: Probing device %s\n", device.ipAddress.c_str());
    
    std::vector<String> cameraEndpoints = getCameraEndpoints();
    int foundEndpoints = 0;
    
    for (const String& endpoint : cameraEndpoints) {
        if (checkEndpoint(device.baseUrl, endpoint, httpClient)) {
            foundEndpoints++;
            device.endpoints.push_back(ApiEndpoint(endpoint, "GET", "Camera API endpoint"));
        }
    }
    
    if (foundEndpoints >= 2) {
        uint32_t capabilities = static_cast<uint32_t>(DeviceCapability::CAMERA) |
                               static_cast<uint32_t>(DeviceCapability::NETWORKING);
        
        // Check for WebSocket capability
        if (checkEndpoint(device.baseUrl, "/ws", httpClient)) {
            capabilities |= static_cast<uint32_t>(DeviceCapability::WEBSOCKET);
        }
        
        setDeviceProperties(device, DeviceType::ESP32_CAMERA, capabilities, "ESP32 Camera");
        
        DEBUG_PRINTF("ESP32CameraDriver: Successfully identified camera device %s\n", device.ipAddress.c_str());
        return true;
    }
    
    return false;
}

JsonDocument ESP32CameraDriver::getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) {
    JsonDocument info;
    info["type"] = "ESP32_CAMERA";
    info["name"] = device.name;
    info["driver"] = getDriverName();
    
    JsonArray capArray = info["capabilities"].to<JsonArray>();
    std::vector<String> caps = DeviceTypeUtils::capabilitiesToStrings(device.capabilities);
    for (const String& cap : caps) {
        capArray.add(cap);
    }
    
    // Try to get camera status
    HttpResponse response = httpClient.get(device.baseUrl + "/api/v1/camera/status");
    if (response.statusCode == 200) {
        JsonDocument cameraStatus;
        if (deserializeJson(cameraStatus, response.body) == DeserializationError::Ok) {
            info["camera_status"] = cameraStatus;
        }
    }
    
    return info;
}

bool ESP32CameraDriver::executeCommand(const IoTDevice& device, const String& command, 
                                      const JsonDocument& params, HttpClientManager& httpClient) {
    DEBUG_PRINTF("ESP32CameraDriver: Executing command '%s' on device %s\n", 
                command.c_str(), device.ipAddress.c_str());
    
    if (command == "capture") {
        String url = device.baseUrl + "/api/v1/camera/capture";
        if (params["quality"]) {
            url += "?quality=" + params["quality"].as<String>();
        }
        
        HttpResponse response = httpClient.post(url, "");
        return response.statusCode >= 200 && response.statusCode < 300;
    }
    else if (command == "start_stream") {
        String url = device.baseUrl + "/api/v1/camera/stream";
        HttpResponse response = httpClient.post(url, "");
        return response.statusCode >= 200 && response.statusCode < 300;
    }
    else if (command == "stop_stream") {
        String url = device.baseUrl + "/api/v1/camera/stream";
        HttpResponse response = httpClient.del(url);
        return response.statusCode >= 200 && response.statusCode < 300;
    }
    
    DEBUG_PRINTF("ESP32CameraDriver: Unknown command '%s'\n", command.c_str());
    return false;
}
