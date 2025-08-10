#ifndef IOT_DEVICE_CONTROLLER_H
#define IOT_DEVICE_CONTROLLER_H

#include <Arduino.h>
#include <MVCFramework.h>
#include "iot_device_manager.h"

class IoTDeviceController : public Controller {
private:
    IoTDeviceManager* iotManager;

public:
    IoTDeviceController(IoTDeviceManager* manager) : iotManager(manager) {}

    // GET /api/v1/iot/devices - Get all discovered devices
    Response getAllDevices(Request& request);
    
    // GET /api/v1/iot/devices/online - Get only online devices
    Response getOnlineDevices(Request& request);
    
    // GET /api/v1/iot/devices/:id - Get specific device by ID
    Response getDevice(Request& request);
    
    // GET /api/v1/iot/devices/:id/info - Get detailed device information
    Response getDeviceInfo(Request& request);
    
    // POST /api/v1/iot/devices/:id/command - Execute command on device
    Response executeCommand(Request& request);
    
    // POST /api/v1/iot/devices/:id/refresh - Refresh device information
    Response refreshDevice(Request& request);
    
    // GET /api/v1/iot/devices/types/:type - Get devices by type
    Response getDevicesByType(Request& request);
    
    // GET /api/v1/iot/devices/capabilities/:capability - Get devices by capability
    Response getDevicesByCapability(Request& request);
    
    // POST /api/v1/iot/scan - Trigger device discovery scan
    Response scanDevices(Request& request);
    
    // GET /api/v1/iot/statistics - Get IoT discovery statistics
    Response getStatistics(Request& request);
    
    // GET /api/v1/iot/discovery/status - Get discovery service status
    Response getDiscoveryStatus(Request& request);
    
    // POST /api/v1/iot/discovery/start - Start discovery service
    Response startDiscovery(Request& request);
    
    // POST /api/v1/iot/discovery/stop - Stop discovery service
    Response stopDiscovery(Request& request);

private:
    // Helper method to convert IoTDevice to JSON
    void deviceToJson(const IoTDevice& device, JsonObject& json);
    
    // Helper method to convert device type string to enum
    DeviceType stringToDeviceType(const String& typeStr);
    
    // Helper method to convert capability string to enum
    DeviceCapability stringToCapability(const String& capStr);
};

#endif // IOT_DEVICE_CONTROLLER_H
