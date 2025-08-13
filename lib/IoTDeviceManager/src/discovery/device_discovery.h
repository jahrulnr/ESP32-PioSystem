#ifndef DEVICE_DISCOVERY_H
#define DEVICE_DISCOVERY_H

#include "../types/device_types.h"
#include "../drivers/device_driver.h"
#include "wifi_manager.h"
#include "httpclient.h"
#include <vector>
#include <functional>

/**
 * @brief Device discovery engine for scanning and identifying IoT devices
 */
class DeviceDiscovery {
public:
    /**
     * @brief Constructor
     */
    DeviceDiscovery(WiFiManager* wifiManager, HttpClientManager* httpClient);
    
    /**
     * @brief Destructor
     */
    ~DeviceDiscovery();
    
    /**
     * @brief Start discovery process
     */
    void startDiscovery(unsigned long scanIntervalMs = 30000);
    
    /**
     * @brief Stop discovery process
     */
    void stopDiscovery();
    
    /**
     * @brief Perform manual scan for devices
     */
    int scanForDevices();
    
    /**
     * @brief Force immediate scan on next update
     */
    void startManualScan();
    
    /**
     * @brief Check if discovery is active
     */
    bool isDiscoveryActive() const;
    
    /**
     * @brief Register a device driver
     */
    void registerDriver(DeviceDriver* driver);
    
    /**
     * @brief Get scan statistics
     */
    JsonDocument getStatistics() const;
    
    /**
     * @brief Set device discovered callback
     */
    void setDeviceDiscoveredCallback(std::function<void(const IoTDevice&)> callback);
    
    /**
     * @brief Set device status change callback
     */
    void setDeviceStatusCallback(std::function<void(const IoTDevice&, bool)> callback);

private:
    WiFiManager* _wifiManager;
    HttpClientManager* _httpClient;
    std::vector<DeviceDriver*> _drivers;
    std::vector<IoTDevice> _devices;
    
    bool _discoveryEnabled;
    unsigned long _lastDiscoveryScan;
    unsigned long _discoveryInterval;
    TaskHandle_t _discoveryTaskHandle;
    
    // Statistics
    unsigned long _totalScans;
    unsigned long _devicesDiscovered;
    unsigned long _lastScanDuration;
    
    // Callbacks
    std::function<void(const IoTDevice&)> _deviceDiscoveredCallback;
    std::function<void(const IoTDevice&, bool)> _deviceStatusCallback;
    
    /**
     * @brief Check if device has HTTP server
     */
    bool checkHttpServer(const String& ipAddress);
    
    /**
     * @brief Probe device with registered drivers
     */
    bool probeDevice(IoTDevice& device);
    
    /**
     * @brief Find best driver for device
     */
    DeviceDriver* findDriverForDevice(const IoTDevice& device);
    
    /**
     * @brief Update device status
     */
    void updateDeviceStatus(IoTDevice& device, bool isOnline);
    
    /**
     * @brief Discovery task function
     */
    static void discoveryTask(void* parameter);
    
    friend class IoTDeviceManager; // Allow access to _devices
};

#endif // DEVICE_DISCOVERY_H
