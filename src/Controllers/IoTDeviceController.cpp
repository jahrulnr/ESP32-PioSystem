#include "IoTDeviceController.h"
#include "SerialDebug.h"

Response IoTDeviceController::getAllDevices(Request& request) {
    JsonDocument doc;
    doc["status"] = "success";
    
    std::vector<IoTDevice> devices = iotManager->getDevices();
    JsonArray devicesArray = doc["devices"].to<JsonArray>();
    
    for (const auto& device : devices) {
        JsonObject deviceObj = devicesArray.add<JsonObject>();
        deviceToJson(device, deviceObj);
    }
    
    doc["total"] = devices.size();
    
    return json(request.getServerRequest(), doc);
}

Response IoTDeviceController::getOnlineDevices(Request& request) {
    JsonDocument doc;
    doc["status"] = "success";
    
    std::vector<IoTDevice> devices = iotManager->getOnlineDevices();
    JsonArray devicesArray = doc["devices"].to<JsonArray>();
    
    for (const auto& device : devices) {
        JsonObject deviceObj = devicesArray.add<JsonObject>();
        deviceToJson(device, deviceObj);
    }
    
    doc["total"] = devices.size();
    
    return json(request.getServerRequest(), doc);
}

Response IoTDeviceController::getDevice(Request& request) {
    String deviceId = request.route("id");
    
    if (deviceId.isEmpty()) {
        return error(request.getServerRequest(), "Device ID is required");
    }
    
    IoTDevice* device = iotManager->getDevice(deviceId);
    if (device == nullptr) {
        return error(request.getServerRequest(), "Device not found");
    }
    
    JsonDocument doc;
    doc["status"] = "success";
    JsonObject deviceObj = doc["device"].to<JsonObject>();
    deviceToJson(*device, deviceObj);
    
    return json(request.getServerRequest(), doc);
}

Response IoTDeviceController::getDeviceInfo(Request& request) {
    String deviceId = request.route("id");
    
    if (deviceId.isEmpty()) {
        return error(request.getServerRequest(), "Device ID is required");
    }
    
    IoTDevice* device = iotManager->getDevice(deviceId);
    if (device == nullptr) {
        return error(request.getServerRequest(), "Device not found");
    }
    
    if (!device->isOnline) {
        return error(request.getServerRequest(), "Device is offline");
    }
    
    // This will make an HTTP request to the device, so it might take some time
    JsonDocument deviceInfo = iotManager->executeDeviceCommand(deviceId, "get_info");
    
    JsonDocument doc;
    doc["status"] = "success";
    doc["device_id"] = deviceId;
    doc["device_info"] = deviceInfo;
    
    return json(request.getServerRequest(), doc);
}

Response IoTDeviceController::executeCommand(Request& request) {
    String deviceId = request.route("id");
    
    if (deviceId.isEmpty()) {
        return error(request.getServerRequest(), "Device ID is required");
    }
    
    // Get command from request body
    String command = request.input("command");
    if (command.isEmpty()) {
        return error(request.getServerRequest(), "Command is required");
    }
    
    // Get parameters from request body
    JsonDocument params;
    String paramsStr = request.input("parameters");
    if (!paramsStr.isEmpty()) {
        DeserializationError error = deserializeJson(params, paramsStr);
        if (error) {
            return this->error(request.getServerRequest(), "Invalid parameters JSON");
        }
    }
    
    DEBUG_PRINTF("IoTDeviceController: Executing command '%s' on device '%s'\n", 
                command.c_str(), deviceId.c_str());
    
    JsonDocument result = iotManager->executeDeviceCommand(deviceId, command, params);
    
    JsonDocument doc;
    if (result["error"].is<String>()) {
        doc["status"] = "error";
        doc["message"] = result["error"];
    } else {
        doc["status"] = "success";
        doc["result"] = result;
    }
    
    return json(request.getServerRequest(), doc);
}

Response IoTDeviceController::refreshDevice(Request& request) {
    String deviceId = request.route("id");
    
    if (deviceId.isEmpty()) {
        return error(request.getServerRequest(), "Device ID is required");
    }
    
    bool success = iotManager->refreshDevice(deviceId);
    
    JsonDocument doc;
    if (success) {
        doc["status"] = "success";
        doc["message"] = "Device information refreshed";
    } else {
        doc["status"] = "error";
        doc["message"] = "Failed to refresh device information";
    }
    
    return json(request.getServerRequest(), doc);
}

Response IoTDeviceController::getDevicesByType(Request& request) {
    String typeStr = request.route("type");
    
    if (typeStr.isEmpty()) {
        return error(request.getServerRequest(), "Device type is required");
    }
    
    DeviceType type = stringToDeviceType(typeStr);
    if (type == DeviceType::UNKNOWN && typeStr != "unknown") {
        return error(request.getServerRequest(), "Invalid device type");
    }
    
    std::vector<IoTDevice*> devices = iotManager->getDevicesByType(type);
    
    JsonDocument doc;
    doc["status"] = "success";
    doc["device_type"] = typeStr;
    
    JsonArray devicesArray = doc["devices"].to<JsonArray>();
    for (const auto* device : devices) {
        JsonObject deviceObj = devicesArray.add<JsonObject>();
        deviceToJson(*device, deviceObj);
    }
    
    doc["total"] = devices.size();
    
    return json(request.getServerRequest(), doc);
}

Response IoTDeviceController::getDevicesByCapability(Request& request) {
    String capStr = request.route("capability");
    
    if (capStr.isEmpty()) {
        return error(request.getServerRequest(), "Device capability is required");
    }
    
    DeviceCapability capability = stringToCapability(capStr);
    std::vector<IoTDevice*> devices = iotManager->getDevicesWithCapability(capability);
    
    JsonDocument doc;
    doc["status"] = "success";
    doc["capability"] = capStr;
    
    JsonArray devicesArray = doc["devices"].to<JsonArray>();
    for (const auto* device : devices) {
        JsonObject deviceObj = devicesArray.add<JsonObject>();
        deviceToJson(*device, deviceObj);
    }
    
    doc["total"] = devices.size();
    
    return json(request.getServerRequest(), doc);
}

Response IoTDeviceController::scanDevices(Request& request) {
    DEBUG_PRINTLN("IoTDeviceController: Manual device scan requested");
    
    int newDevices = iotManager->scanForDevices();
    
    JsonDocument doc;
    doc["status"] = "success";
    doc["message"] = "Device scan completed";
    doc["new_devices_found"] = newDevices;
    doc["total_devices"] = iotManager->getDevices().size();
    
    return json(request.getServerRequest(), doc);
}

Response IoTDeviceController::getStatistics(Request& request) {
    JsonDocument stats = iotManager->getStatistics();
    
    JsonDocument doc;
    doc["status"] = "success";
    doc["statistics"] = stats;
    
    return json(request.getServerRequest(), doc);
}

Response IoTDeviceController::getDiscoveryStatus(Request& request) {
    JsonDocument stats = iotManager->getStatistics();
    
    JsonDocument doc;
    doc["status"] = "success";
    doc["discovery_enabled"] = stats["discovery_enabled"];
    doc["discovery_interval_ms"] = stats["discovery_interval_ms"];
    doc["last_scan_duration_ms"] = stats["last_scan_duration_ms"];
    doc["total_scans"] = stats["total_scans"];
    
    return json(request.getServerRequest(), doc);
}

Response IoTDeviceController::startDiscovery(Request& request) {
    // Get interval from request (optional)
    String intervalStr = request.input("interval");
    unsigned long interval = 30000; // Default 30 seconds
    
    if (!intervalStr.isEmpty()) {
        interval = intervalStr.toInt();
        if (interval < 5000) interval = 5000; // Minimum 5 seconds
        if (interval > 300000) interval = 300000; // Maximum 5 minutes
    }
    
    iotManager->startDiscovery(interval);
    
    JsonDocument doc;
    doc["status"] = "success";
    doc["message"] = "Device discovery started";
    doc["interval_ms"] = interval;
    
    return json(request.getServerRequest(), doc);
}

Response IoTDeviceController::stopDiscovery(Request& request) {
    iotManager->stopDiscovery();
    
    JsonDocument doc;
    doc["status"] = "success";
    doc["message"] = "Device discovery stopped";
    
    return json(request.getServerRequest(), doc);
}

void IoTDeviceController::deviceToJson(const IoTDevice& device, JsonObject& json) {
    json["id"] = device.id;
    json["name"] = device.name;
    json["mac_address"] = device.macAddress;
    json["ip_address"] = device.ipAddress;
    json["hostname"] = device.hostname;
    json["type"] = IoTDeviceManager::deviceTypeToString(device.type);
    json["manufacturer"] = device.manufacturer;
    json["model"] = device.model;
    json["firmware_version"] = device.firmwareVersion;
    json["api_version"] = device.apiVersion;
    json["base_url"] = device.baseUrl;
    json["is_online"] = device.isOnline;
    json["has_http_server"] = device.hasHttpServer;
    json["last_seen"] = device.lastSeen;
    json["discovered_at"] = device.discoveredAt;
    
    // Capabilities
    JsonArray capArray = json["capabilities"].to<JsonArray>();
    std::vector<String> capabilities = IoTDeviceManager::capabilitiesToStrings(device.capabilities);
    for (const String& cap : capabilities) {
        capArray.add(cap);
    }
    
    // Endpoints
    JsonArray endpointsArray = json["endpoints"].to<JsonArray>();
    for (const auto& endpoint : device.endpoints) {
        JsonObject endpointObj = endpointsArray.add<JsonObject>();
        endpointObj["path"] = endpoint.path;
        endpointObj["method"] = endpoint.method;
        endpointObj["description"] = endpoint.description;
        endpointObj["requires_auth"] = endpoint.requiresAuth;
    }
    
    // Metadata (if any)
    if (device.metadata.size() > 0) {
        json["metadata"] = device.metadata;
    }
}

DeviceType IoTDeviceController::stringToDeviceType(const String& typeStr) {
    String lower = typeStr;
    lower.toLowerCase();
    
    if (lower == "esp32_camera" || lower == "esp32-camera") return DeviceType::ESP32_CAMERA;
    if (lower == "esp32_sensor" || lower == "esp32-sensor") return DeviceType::ESP32_SENSOR;
    if (lower == "esp32_controller" || lower == "esp32-controller") return DeviceType::ESP32_CONTROLLER;
    if (lower == "esp32_display" || lower == "esp32-display") return DeviceType::ESP32_DISPLAY;
    if (lower == "raspberry_pi" || lower == "raspberry-pi") return DeviceType::RASPBERRY_PI;
    if (lower == "arduino_iot" || lower == "arduino-iot") return DeviceType::ARDUINO_IOT;
    if (lower == "custom_device" || lower == "custom-device") return DeviceType::CUSTOM_DEVICE;
    if (lower == "unknown") return DeviceType::UNKNOWN;
    
    return DeviceType::UNKNOWN;
}

DeviceCapability IoTDeviceController::stringToCapability(const String& capStr) {
    String lower = capStr;
    lower.toLowerCase();
    
    if (lower == "camera") return DeviceCapability::CAMERA;
    if (lower == "sensors") return DeviceCapability::SENSORS;
    if (lower == "display") return DeviceCapability::DISPLAY_SCREEN;
    if (lower == "actuators") return DeviceCapability::ACTUATORS;
    if (lower == "storage") return DeviceCapability::STORAGE;
    if (lower == "networking") return DeviceCapability::NETWORKING;
    if (lower == "authentication") return DeviceCapability::AUTHENTICATION;
    if (lower == "websocket") return DeviceCapability::WEBSOCKET;
    if (lower == "file_upload" || lower == "file-upload") return DeviceCapability::FILE_UPLOAD;
    if (lower == "system_control" || lower == "system-control") return DeviceCapability::SYSTEM_CONTROL;
    
    return DeviceCapability::NETWORKING; // Default
}
