#ifndef ESP32_MVC_DRIVER_H
#define ESP32_MVC_DRIVER_H

#include "device_driver.h"

/**
 * @brief ESP32 MVC Framework device driver
 */
class ESP32MVCDriver : public DeviceDriver {
public:
    DeviceType getDeviceType() const override {
        return DeviceType::ESP32_CONTROLLER;
    }
    
    String getDriverName() const override {
        return "ESP32MVCDriver";
    }
    
    bool canHandle(const IoTDevice& device) const override {
        return device.type == DeviceType::ESP32_CONTROLLER;
    }
    
    bool probe(IoTDevice& device, HttpClientManager& httpClient) override;
    
    JsonDocument getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) override;
    
    bool executeCommand(const IoTDevice& device, const String& command, 
                       const JsonDocument& params, HttpClientManager& httpClient) override;

private:
    std::vector<String> getMVCEndpoints() const;
};

#endif // ESP32_MVC_DRIVER_H
