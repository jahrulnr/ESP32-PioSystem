#ifndef ESP32_CAMERA_DRIVER_H
#define ESP32_CAMERA_DRIVER_H

#include "device_driver.h"

/**
 * @brief ESP32 Camera device driver
 */
class ESP32CameraDriver : public DeviceDriver {
public:
    DeviceType getDeviceType() const override {
        return DeviceType::ESP32_CAMERA;
    }
    
    String getDriverName() const override {
        return "ESP32CameraDriver";
    }
    
    bool canHandle(const IoTDevice& device) const override {
        return device.type == DeviceType::ESP32_CAMERA;
    }
    
    bool probe(IoTDevice& device, HttpClientManager& httpClient) override;
    
    JsonDocument getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) override;
    
    bool executeCommand(const IoTDevice& device, const String& command, 
                       const JsonDocument& params, HttpClientManager& httpClient) override;

private:
    std::vector<String> getCameraEndpoints() const;
};

#endif // ESP32_CAMERA_DRIVER_H
