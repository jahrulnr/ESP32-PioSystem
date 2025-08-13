#include "generic_rest_driver.h"
#include "SerialDebug.h"

std::vector<String> GenericRESTDriver::getCommonEndpoints() const {
    return {
        "/", "/api", "/status", "/info", "/health", 
        "/device", "/config", "/settings"
    };
}

bool GenericRESTDriver::probe(IoTDevice& device, HttpClientManager& httpClient) {
    DEBUG_PRINTF("GenericRESTDriver: Probing device %s\n", device.ipAddress.c_str());
    
    std::vector<String> commonEndpoints = getCommonEndpoints();
    int foundEndpoints = 0;
    
    for (const String& endpoint : commonEndpoints) {
        if (checkEndpoint(device.baseUrl, endpoint, httpClient)) {
            foundEndpoints++;
            device.endpoints.push_back(ApiEndpoint(endpoint, "GET", "Generic endpoint"));
            
            if (foundEndpoints >= 2) break; // Found enough to confirm it's a device
        }
    }
    
    if (foundEndpoints >= 1) {
        // Basic networking capability for any device with HTTP server
        uint32_t capabilities = static_cast<uint32_t>(DeviceCapability::NETWORKING);
        
        // Detect additional capabilities
        detectCapabilities(device, httpClient);
        
        setDeviceProperties(device, DeviceType::CUSTOM_DEVICE, capabilities, "Generic IoT Device");
        
        DEBUG_PRINTF("GenericRESTDriver: Successfully identified generic device %s\n", device.ipAddress.c_str());
        return true;
    }
    
    return false;
}

void GenericRESTDriver::detectCapabilities(IoTDevice& device, HttpClientManager& httpClient) {
    uint32_t capabilities = static_cast<uint32_t>(DeviceCapability::NETWORKING);
    
    // Check for WebSocket
    if (checkEndpoint(device.baseUrl, "/ws", httpClient) || 
        checkEndpoint(device.baseUrl, "/websocket", httpClient)) {
        capabilities |= static_cast<uint32_t>(DeviceCapability::WEBSOCKET);
    }
    
    // Check for file upload
    if (checkEndpoint(device.baseUrl, "/upload", httpClient) || 
        checkEndpoint(device.baseUrl, "/api/upload", httpClient)) {
        capabilities |= static_cast<uint32_t>(DeviceCapability::FILE_UPLOAD);
    }
    
    // Check for authentication
    if (checkEndpoint(device.baseUrl, "/login", httpClient) || 
        checkEndpoint(device.baseUrl, "/auth", httpClient) ||
        checkEndpoint(device.baseUrl, "/api/auth", httpClient)) {
        capabilities |= static_cast<uint32_t>(DeviceCapability::AUTHENTICATION);
    }
    
    // Check for sensor endpoints
    if (checkEndpoint(device.baseUrl, "/sensors", httpClient) || 
        checkEndpoint(device.baseUrl, "/api/sensors", httpClient) ||
        checkEndpoint(device.baseUrl, "/api/v1/sensors", httpClient)) {
        capabilities |= static_cast<uint32_t>(DeviceCapability::SENSORS);
    }
    
    device.capabilities = capabilities;
}

JsonDocument GenericRESTDriver::getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) {
    JsonDocument info;
    info["type"] = "CUSTOM_DEVICE";
    info["name"] = device.name;
    info["driver"] = getDriverName();
    
    JsonArray capArray = info["capabilities"].to<JsonArray>();
    std::vector<String> caps = DeviceTypeUtils::capabilitiesToStrings(device.capabilities);
    for (const String& cap : caps) {
        capArray.add(cap);
    }
    
    // Try to get device info from common endpoints
    std::vector<String> infoEndpoints = {"/info", "/status", "/api/info", "/device"};
    
    for (const String& endpoint : infoEndpoints) {
        HttpResponse response = httpClient.get(device.baseUrl + endpoint);
        if (response.statusCode == 200) {
            JsonDocument deviceInfo;
            if (deserializeJson(deviceInfo, response.body) == DeserializationError::Ok) {
                info["device_info"] = deviceInfo;
                break;
            }
        }
    }
    
    return info;
}

bool GenericRESTDriver::executeCommand(const IoTDevice& device, const String& command, 
                                      const JsonDocument& params, HttpClientManager& httpClient) {
    DEBUG_PRINTF("GenericRESTDriver: Executing command '%s' on device %s\n", 
                command.c_str(), device.ipAddress.c_str());
    
    if (command == "get_status") {
        std::vector<String> statusEndpoints = {"/status", "/api/status", "/health"};
        
        for (const String& endpoint : statusEndpoints) {
            HttpResponse response = httpClient.get(device.baseUrl + endpoint);
            if (response.statusCode >= 200 && response.statusCode < 300) {
                return true;
            }
        }
        return false;
    }
    else if (command == "get_info") {
        std::vector<String> infoEndpoints = {"/info", "/api/info", "/device"};
        
        for (const String& endpoint : infoEndpoints) {
            HttpResponse response = httpClient.get(device.baseUrl + endpoint);
            if (response.statusCode >= 200 && response.statusCode < 300) {
                return true;
            }
        }
        return false;
    }
    
    DEBUG_PRINTF("GenericRESTDriver: Unknown command '%s'\n", command.c_str());
    return false;
}
