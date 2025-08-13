#ifndef GENERIC_REST_DRIVER_H
#define GENERIC_REST_DRIVER_H

#include "device_driver.h"

/**
 * @brief Generic REST API device driver for unknown IoT devices
 */
class GenericRESTDriver : public DeviceDriver {
public:
    DeviceType getDeviceType() const override {
        return DeviceType::CUSTOM_DEVICE;
    }
    
    String getDriverName() const override {
        return "GenericRESTDriver";
    }
    
    bool canHandle(const IoTDevice& device) const override {
        return device.type == DeviceType::CUSTOM_DEVICE || device.type == DeviceType::UNKNOWN;
    }
    
    bool probe(IoTDevice& device, HttpClientManager& httpClient) override;
    
    JsonDocument getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) override;
    
    bool executeCommand(const IoTDevice& device, const String& command, 
                       const JsonDocument& params, HttpClientManager& httpClient) override;

private:
    std::vector<String> getCommonEndpoints() const;
    void detectCapabilities(IoTDevice& device, HttpClientManager& httpClient);
};

#endif // GENERIC_REST_DRIVER_H
