#ifndef DEVICE_TYPES_H
#define DEVICE_TYPES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

/**
 * @brief IoT Device Type enumeration
 */
enum class DeviceType {
    UNKNOWN,
    ESP32_CAMERA,
    ESP32_SENSOR,
    ESP32_CONTROLLER,
    ESP32_DISPLAY,
    RASPBERRY_PI,
    ARDUINO_IOT,
    CUSTOM_DEVICE
};

/**
 * @brief Device capability flags
 */
enum class DeviceCapability {
    CAMERA = 1 << 0,
    SENSORS = 1 << 1,
    DISPLAY_SCREEN = 1 << 2,  // Renamed to avoid conflict with ESP32 DISPLAY macro
    ACTUATORS = 1 << 3,
    STORAGE = 1 << 4,
    NETWORKING = 1 << 5,
    AUTHENTICATION = 1 << 6,
    WEBSOCKET = 1 << 7,
    FILE_UPLOAD = 1 << 8,
    SYSTEM_CONTROL = 1 << 9
};

/**
 * @brief API endpoint information
 */
struct ApiEndpoint {
    String path;
    String method;
    String description;
    bool requiresAuth;
    JsonDocument parameters;
    
    ApiEndpoint() : requiresAuth(false) {}
    ApiEndpoint(const String& p, const String& m, const String& desc = "", bool auth = false) 
        : path(p), method(m), description(desc), requiresAuth(auth) {}
};

/**
 * @brief IoT Device information structure
 */
struct IoTDevice {
    String id;                          // Unique device identifier (MAC-based)
    String name;                        // Device friendly name
    String macAddress;                  // MAC address
    String ipAddress;                   // Current IP address
    String hostname;                    // Device hostname
    DeviceType type;                    // Device type
    uint32_t capabilities;              // Bitmask of DeviceCapability flags
    String manufacturer;                // Device manufacturer
    String model;                       // Device model
    String firmwareVersion;             // Firmware version
    String apiVersion;                  // API version
    String baseUrl;                     // Base API URL (http://ip:port)
    bool isOnline;                      // Current online status
    bool hasHttpServer;                 // Has HTTP server on port 80
    unsigned long lastSeen;             // Last successful communication
    unsigned long discoveredAt;         // When device was first discovered
    std::vector<ApiEndpoint> endpoints; // Available API endpoints
    JsonDocument metadata;              // Additional device-specific data
    
    IoTDevice() : type(DeviceType::UNKNOWN), capabilities(0), isOnline(false), 
                  hasHttpServer(false), lastSeen(0), discoveredAt(0) {}
};

/**
 * @brief Utility functions for device types
 */
class DeviceTypeUtils {
public:
    /**
     * @brief Convert device type to string
     */
    static String deviceTypeToString(DeviceType type);
    
    /**
     * @brief Convert capability flags to string array
     */
    static std::vector<String> capabilitiesToStrings(uint32_t capabilities);
    
    /**
     * @brief Create device ID from MAC address
     */
    static String createDeviceId(const String& macAddress);
};

#endif // DEVICE_TYPES_H
