#ifndef IOT_DEVICE_MANAGER_H
#define IOT_DEVICE_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <map>
#include <functional>
#include "httpclient.h"
#include "wifi_manager.h"

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
 * @brief Device driver interface for handling device-specific operations
 */
class DeviceDriver {
public:
    virtual ~DeviceDriver() = default;
    virtual DeviceType getDeviceType() const = 0;
    virtual String getDriverName() const = 0;
    virtual bool canHandle(const IoTDevice& device) const = 0;
    virtual bool probe(IoTDevice& device, HttpClientManager& httpClient) = 0;
    virtual JsonDocument getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) = 0;
    virtual bool executeCommand(const IoTDevice& device, const String& command, 
                               const JsonDocument& params, HttpClientManager& httpClient) = 0;
};

/**
 * @brief IoT Device Manager for discovering and managing connected devices
 */
class IoTDeviceManager {
public:
    /**
     * @brief Construct a new IoT Device Manager
     * 
     * @param wifiManager Pointer to WiFiManager instance
     * @param httpClient Pointer to HttpClientManager instance
     */
    IoTDeviceManager(WiFiManager* wifiManager, HttpClientManager* httpClient);
    
    /**
     * @brief Destroy the IoT Device Manager
     */
    ~IoTDeviceManager();

    /**
     * @brief Initialize the device manager
     * 
     * @return true if initialization successful
     */
    bool begin();

    /**
     * @brief Register a device driver
     * 
     * @param driver Pointer to device driver instance
     */
    void registerDriver(DeviceDriver* driver);

    /**
     * @brief Start device discovery process
     * 
     * @param scanIntervalMs Interval between discovery scans in milliseconds
     */
    void startDiscovery(unsigned long scanIntervalMs = 30000);

    /**
     * @brief Stop device discovery process
     */
    void stopDiscovery();

    /**
     * @brief Manually scan for devices (blocking)
     * 
     * @return Number of devices discovered
     */
    int scanForDevices();

    /**
     * @brief Start manual scan for devices (non-blocking)
     * Forces immediate discovery scan regardless of timer
     */
    void startManualScan();

    /**
     * @brief Check if discovery is currently active
     * 
     * @return true if discovery is running
     */
    bool isDiscoveryActive() const;

    /**
     * @brief Get total number of discovery scans performed
     * 
     * @return unsigned long Number of scans
     */
    unsigned long getScanCount() const;

    /**
     * @brief Get discovered devices (alias for backward compatibility)
     * 
     * @return std::vector<IoTDevice> List of devices
     */
    std::vector<IoTDevice> getDiscoveredDevices() const;

    /**
     * @brief Get list of all discovered devices
     * 
     * @return std::vector<IoTDevice> List of devices
     */
    std::vector<IoTDevice> getDevices() const;

    /**
     * @brief Get list of online devices only
     * 
     * @return std::vector<IoTDevice> List of online devices
     */
    std::vector<IoTDevice> getOnlineDevices() const;

    /**
     * @brief Get device by ID
     * 
     * @param deviceId Device ID
     * @return IoTDevice* Pointer to device or nullptr if not found
     */
    IoTDevice* getDevice(const String& deviceId);

    /**
     * @brief Get device by IP address
     * 
     * @param ipAddress IP address
     * @return IoTDevice* Pointer to device or nullptr if not found
     */
    IoTDevice* getDeviceByIP(const String& ipAddress);

    /**
     * @brief Get devices by type
     * 
     * @param type Device type
     * @return std::vector<IoTDevice*> List of device pointers
     */
    std::vector<IoTDevice*> getDevicesByType(DeviceType type);

    /**
     * @brief Get devices with specific capability
     * 
     * @param capability Device capability flag
     * @return std::vector<IoTDevice*> List of device pointers
     */
    std::vector<IoTDevice*> getDevicesWithCapability(DeviceCapability capability);

    /**
     * @brief Refresh device information
     * 
     * @param deviceId Device ID
     * @return true if refresh successful
     */
    bool refreshDevice(const String& deviceId);

    /**
     * @brief Execute command on device
     * 
     * @param deviceId Device ID
     * @param command Command to execute
     * @param parameters Command parameters
     * @return JsonDocument Response from device
     */
    JsonDocument executeDeviceCommand(const String& deviceId, const String& command, 
                                     const JsonDocument& parameters = JsonDocument());

    /**
     * @brief Set device discovery callback
     * 
     * @param callback Function to call when new device is discovered
     */
    void setDeviceDiscoveredCallback(std::function<void(const IoTDevice&)> callback);

    /**
     * @brief Set device status change callback
     * 
     * @param callback Function to call when device status changes
     */
    void setDeviceStatusCallback(std::function<void(const IoTDevice&, bool)> callback);

    /**
     * @brief Get device statistics
     * 
     * @return JsonDocument Statistics about discovered devices
     */
    JsonDocument getStatistics() const;

    /**
     * @brief Convert device type to string
     * 
     * @param type Device type
     * @return String Device type name
     */
    static String deviceTypeToString(DeviceType type);

    /**
     * @brief Convert capability flags to string array
     * 
     * @param capabilities Capability bitmask
     * @return std::vector<String> List of capability names
     */
    static std::vector<String> capabilitiesToStrings(uint32_t capabilities);

private:
    WiFiManager* _wifiManager;
    HttpClientManager* _httpClient;
    std::vector<IoTDevice> _devices;
    std::vector<DeviceDriver*> _drivers;
    bool _discoveryEnabled;
    unsigned long _lastDiscoveryScan;
    unsigned long _discoveryInterval;
    
    // Callbacks
    std::function<void(const IoTDevice&)> _deviceDiscoveredCallback;
    std::function<void(const IoTDevice&, bool)> _deviceStatusCallback;
    
    // Statistics
    unsigned long _totalScans;
    unsigned long _devicesDiscovered;
    unsigned long _lastScanDuration;
    
    /**
     * @brief Check if client has HTTP server on port 80
     * 
     * @param ipAddress Client IP address
     * @return true if HTTP server detected
     */
    bool checkHttpServer(const String& ipAddress);
    
    /**
     * @brief Probe device API to determine type and capabilities
     * 
     * @param device Device to probe
     * @return true if probing successful
     */
    bool probeDevice(IoTDevice& device);
    
    /**
     * @brief Create device ID from MAC address
     * 
     * @param macAddress MAC address
     * @return String Device ID
     */
    String createDeviceId(const String& macAddress);
    
    /**
     * @brief Find best driver for device
     * 
     * @param device Device to find driver for
     * @return DeviceDriver* Best matching driver or nullptr
     */
    DeviceDriver* findDriverForDevice(const IoTDevice& device);
    
    /**
     * @brief Update device online status
     * 
     * @param device Device to update
     * @param isOnline New online status
     */
    void updateDeviceStatus(IoTDevice& device, bool isOnline);
    
    /**
     * @brief Task function for device discovery (FreeRTOS task)
     */
    static void discoveryTask(void* parameter);
    
    // Task handle for discovery task
    TaskHandle_t _discoveryTaskHandle;
};

// Built-in device drivers implementations

/**
 * @brief ESP32 Camera device driver
 */
class ESP32CameraDriver : public DeviceDriver {
public:
    String getDriverName() const override {
        return "ESP32CameraDriver";
    }
    
    DeviceType getDeviceType() const override {
        return DeviceType::ESP32_CAMERA;
    }
    
    bool canHandle(const IoTDevice& device) const override {
        return device.type == DeviceType::ESP32_CAMERA;
    }
    
    bool probe(IoTDevice& device, HttpClientManager& httpClient) override {
        // Check for camera endpoints
        std::vector<String> cameraEndpoints = {
            "/api/v1/camera/status",
            "/api/v1/camera/capture", 
            "/api/v1/camera/settings"
        };
        
        int foundEndpoints = 0;
        for (const String& endpoint : cameraEndpoints) {
            String url = device.baseUrl + endpoint;
            HttpResponse response = httpClient.get(url);
            
            if (response.statusCode == 200 || response.statusCode == 405) {
                foundEndpoints++;
            }
        }
        
        if (foundEndpoints >= 2) {
            device.type = DeviceType::ESP32_CAMERA;
            device.name = "ESP32 Camera Device";
            device.capabilities |= static_cast<uint32_t>(DeviceCapability::CAMERA);
            device.capabilities |= static_cast<uint32_t>(DeviceCapability::NETWORKING);
            device.capabilities |= static_cast<uint32_t>(DeviceCapability::WEBSOCKET);
            
            return true;
        }
        
        return false;
    }
    
    JsonDocument getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) override {
        JsonDocument info;
        info["type"] = "ESP32_CAMERA";
        info["name"] = device.name;
        JsonArray capArray = info["capabilities"].to<JsonArray>();
        std::vector<String> caps = IoTDeviceManager::capabilitiesToStrings(device.capabilities);
        for (const String& cap : caps) {
            capArray.add(cap);
        }
        return info;
    }
    
    bool executeCommand(const IoTDevice& device, const String& command, 
                       const JsonDocument& params, HttpClientManager& httpClient) override {
        if (command == "capture") {
            String captureUrl = device.baseUrl + "/api/v1/camera/capture";
            HttpResponse response = httpClient.post(captureUrl, "", "application/json");
            return response.statusCode == 200;
        }
        return false;
    }
};

/**
 * @brief ESP32 MVC Framework device driver
 */
class ESP32MVCDriver : public DeviceDriver {
public:
    String getDriverName() const override {
        return "ESP32MVCDriver";
    }
    
    DeviceType getDeviceType() const override {
        return DeviceType::ESP32_CONTROLLER;
    }
    
    bool canHandle(const IoTDevice& device) const override {
        return device.type == DeviceType::ESP32_CONTROLLER;
    }
    
    bool probe(IoTDevice& device, HttpClientManager& httpClient) override {
        // Check for MVC framework endpoints
        std::vector<String> mvcEndpoints = {
            "/api/v1/auth/user",
            "/api/v1/system/stats",
            "/api/wifi/status"
        };
        
        int foundEndpoints = 0;
        for (const String& endpoint : mvcEndpoints) {
            String url = device.baseUrl + endpoint;
            HttpResponse response = httpClient.get(url);
            
            if (response.statusCode == 200 || response.statusCode == 401) {
                foundEndpoints++;
            }
        }
        
        if (foundEndpoints >= 2) {
            device.type = DeviceType::ESP32_CONTROLLER;
            device.name = "ESP32 MVC Device";
            device.capabilities |= static_cast<uint32_t>(DeviceCapability::NETWORKING);
            device.capabilities |= static_cast<uint32_t>(DeviceCapability::AUTHENTICATION);
            device.capabilities |= static_cast<uint32_t>(DeviceCapability::SYSTEM_CONTROL);
            
            return true;
        }
        
        return false;
    }
    
    JsonDocument getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) override {
        JsonDocument info;
        info["type"] = "ESP32_CONTROLLER";
        info["name"] = device.name;
        JsonArray capArray = info["capabilities"].to<JsonArray>();
        std::vector<String> caps = IoTDeviceManager::capabilitiesToStrings(device.capabilities);
        for (const String& cap : caps) {
            capArray.add(cap);
        }
        return info;
    }
    
    bool executeCommand(const IoTDevice& device, const String& command, 
                       const JsonDocument& params, HttpClientManager& httpClient) override {
        if (command == "system_restart") {
            String restartUrl = device.baseUrl + "/api/v1/system/restart";
            HttpResponse response = httpClient.post(restartUrl, "", "application/json");
            return response.statusCode == 200;
        }
        return false;
    }
};

/**
 * @brief Generic REST API device driver for unknown IoT devices
 */
class GenericRESTDriver : public DeviceDriver {
public:
    String getDriverName() const override {
        return "GenericRESTDriver";
    }
    
    DeviceType getDeviceType() const override {
        return DeviceType::CUSTOM_DEVICE;
    }
    
    bool canHandle(const IoTDevice& device) const override {
        return device.type == DeviceType::CUSTOM_DEVICE || device.type == DeviceType::UNKNOWN;
    }
    
    bool probe(IoTDevice& device, HttpClientManager& httpClient) override {
        // Check for common REST endpoints
        std::vector<String> commonEndpoints = {
            "/", "/api", "/status", "/info", "/health"
        };
        
        bool hasRestApi = false;
        for (const String& endpoint : commonEndpoints) {
            String url = device.baseUrl + endpoint;
            HttpResponse response = httpClient.get(url);
            
            if (response.statusCode == 200) {
                hasRestApi = true;
                
                // Check content type
                if (response.body.indexOf("{") >= 0 || response.body.indexOf("json") >= 0) {
                    // JSON response indicates REST API
                    device.capabilities |= static_cast<uint32_t>(DeviceCapability::NETWORKING);
                } else if (response.body.indexOf("<html") >= 0 || 
                          response.body.indexOf("<!DOCTYPE") >= 0) {
                    // HTML response indicates web interface
                    device.capabilities |= static_cast<uint32_t>(DeviceCapability::DISPLAY_SCREEN);
                }
            }
        }
        
        if (hasRestApi) {
            device.type = DeviceType::CUSTOM_DEVICE;
            if (device.name.isEmpty()) {
                device.name = "Generic IoT Device";
            }
            device.capabilities |= static_cast<uint32_t>(DeviceCapability::NETWORKING);
            
            return true;
        }
        
        return false;
    }
    
    JsonDocument getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) override {
        JsonDocument info;
        info["type"] = "CUSTOM_DEVICE";
        info["name"] = device.name;
        JsonArray capArray = info["capabilities"].to<JsonArray>();
        std::vector<String> caps = IoTDeviceManager::capabilitiesToStrings(device.capabilities);
        for (const String& cap : caps) {
            capArray.add(cap);
        }
        return info;
    }
    
    bool executeCommand(const IoTDevice& device, const String& command, 
                       const JsonDocument& params, HttpClientManager& httpClient) override {
        if (command == "ping") {
            String url = device.baseUrl + "/";
            HttpResponse response = httpClient.get(url);
            return response.statusCode == 200;
        }
        else if (command == "custom_get") {
            if (params["endpoint"].is<String>()) {
                String endpoint = params["endpoint"].as<String>();
                String url = device.baseUrl + endpoint;
                HttpResponse response = httpClient.get(url);
                return response.statusCode == 200;
            }
        }
        else if (command == "custom_post") {
            if (params["endpoint"].is<String>()) {
                String endpoint = params["endpoint"].as<String>();
                String url = device.baseUrl + endpoint;
                String body = params["body"].is<String>() ? params["body"].as<String>() : "";
                HttpResponse response = httpClient.post(url, body, "application/json");
                return response.statusCode == 200;
            }
        }
        
        return false;
    }
};

// Include header files (now empty but maintain file structure)
#include "esp32_camera_driver.h"
#include "esp32_mvc_driver.h"
#include "generic_rest_driver.h"

#endif // IOT_DEVICE_MANAGER_H
