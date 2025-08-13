#ifndef DEVICE_DRIVER_H
#define DEVICE_DRIVER_H

#include "../types/device_types.h"
#include "httpclient.h"
#include <ArduinoJson.h>

/**
 * @brief Device driver interface for handling device-specific operations
 */
class DeviceDriver {
public:
    virtual ~DeviceDriver() = default;
    
    /**
     * @brief Get the device type this driver handles
     */
    virtual DeviceType getDeviceType() const = 0;
    
    /**
     * @brief Get the driver name for identification
     */
    virtual String getDriverName() const = 0;
    
    /**
     * @brief Check if this driver can handle the given device
     */
    virtual bool canHandle(const IoTDevice& device) const = 0;
    
    /**
     * @brief Probe device to determine if it matches this driver
     * Sets device type, capabilities, and other properties if successful
     */
    virtual bool probe(IoTDevice& device, HttpClientManager& httpClient) = 0;
    
    /**
     * @brief Get detailed device information
     */
    virtual JsonDocument getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) = 0;
    
    /**
     * @brief Execute a command on the device
     */
    virtual bool executeCommand(const IoTDevice& device, const String& command, 
                               const JsonDocument& params, HttpClientManager& httpClient) = 0;

protected:
    /**
     * @brief Helper function to check if an endpoint exists
     */
    bool checkEndpoint(const String& baseUrl, const String& endpoint, HttpClientManager& httpClient);
    
    /**
     * @brief Helper function to set common device properties
     */
    void setDeviceProperties(IoTDevice& device, DeviceType type, uint32_t capabilities, 
                           const String& name = "");
};

#endif // DEVICE_DRIVER_H
