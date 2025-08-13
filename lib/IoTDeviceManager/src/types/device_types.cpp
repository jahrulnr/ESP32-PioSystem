#include "device_types.h"

String DeviceTypeUtils::deviceTypeToString(DeviceType type) {
    switch (type) {
        case DeviceType::ESP32_CAMERA: return "ESP32 Camera";
        case DeviceType::ESP32_SENSOR: return "ESP32 Sensor";
        case DeviceType::ESP32_CONTROLLER: return "ESP32 Controller";
        case DeviceType::ESP32_DISPLAY: return "ESP32 Display";
        case DeviceType::RASPBERRY_PI: return "Raspberry Pi";
        case DeviceType::ARDUINO_IOT: return "Arduino IoT";
        case DeviceType::CUSTOM_DEVICE: return "Custom Device";
        default: return "Unknown";
    }
}

std::vector<String> DeviceTypeUtils::capabilitiesToStrings(uint32_t capabilities) {
    std::vector<String> caps;
    
    if (capabilities & static_cast<uint32_t>(DeviceCapability::CAMERA)) caps.push_back("Camera");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::SENSORS)) caps.push_back("Sensors");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::DISPLAY_SCREEN)) caps.push_back("Display");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::ACTUATORS)) caps.push_back("Actuators");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::STORAGE)) caps.push_back("Storage");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::NETWORKING)) caps.push_back("Networking");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::AUTHENTICATION)) caps.push_back("Authentication");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::WEBSOCKET)) caps.push_back("WebSocket");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::FILE_UPLOAD)) caps.push_back("File Upload");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::SYSTEM_CONTROL)) caps.push_back("System Control");
    
    return caps;
}

String DeviceTypeUtils::createDeviceId(const String& macAddress) {
    // Remove colons and convert to lowercase for consistent ID
    String id = macAddress;
    id.replace(":", "");
    id.toLowerCase();
    return "iot_" + id;
}
