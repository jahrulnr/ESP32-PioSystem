#ifndef IOT_DEVICE_MANAGER_CORE_H
#define IOT_DEVICE_MANAGER_CORE_H

#include "../types/device_types.h"
#include "../drivers/device_driver.h"
#include "../discovery/device_discovery.h"
#include "wifi_manager.h"
#include "httpclient.h"
#include <functional>

/**
 * @brief Main IoT Device Manager - orchestrates discovery, drivers, and device management
 */
class IoTDeviceManager {
public:
    /**
     * @brief Constructor
     */
    IoTDeviceManager(WiFiManager* wifiManager, HttpClientManager* httpClient);
    
    /**
     * @brief Destructor
     */
    ~IoTDeviceManager();

    /**
     * @brief Initialize the device manager
     */
    bool begin();

    /**
     * @brief Register a custom device driver
     */
    void registerDriver(DeviceDriver* driver);

    // Discovery methods
    void startDiscovery(unsigned long scanIntervalMs = 30000);
    void stopDiscovery();
    int scanForDevices();
    void startManualScan();
    bool isDiscoveryActive() const;
    unsigned long getScanCount() const;

    // Device access methods
    std::vector<IoTDevice> getDevices() const;
    std::vector<IoTDevice> getDiscoveredDevices() const; // Alias for backward compatibility
    std::vector<IoTDevice> getOnlineDevices() const;
    IoTDevice* getDevice(const String& deviceId);
    IoTDevice* getDeviceByIP(const String& ipAddress);
    std::vector<IoTDevice*> getDevicesByType(DeviceType type);
    std::vector<IoTDevice*> getDevicesWithCapability(DeviceCapability capability);

    // Device operations
    bool refreshDevice(const String& deviceId);
    JsonDocument executeDeviceCommand(const String& deviceId, const String& command, 
                                     const JsonDocument& parameters = JsonDocument());

    // Callbacks
    void setDeviceDiscoveredCallback(std::function<void(const IoTDevice&)> callback);
    void setDeviceStatusCallback(std::function<void(const IoTDevice&, bool)> callback);

    // Statistics and utilities
    JsonDocument getStatistics() const;
    static String deviceTypeToString(DeviceType type);
    static std::vector<String> capabilitiesToStrings(uint32_t capabilities);

private:
    DeviceDiscovery* _discovery;
    
    /**
     * @brief Find best driver for device
     */
    DeviceDriver* findDriverForDevice(const IoTDevice& device);
};

#endif // IOT_DEVICE_MANAGER_CORE_H
