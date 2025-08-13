#include "iot_device_manager_core.h"
#include "../drivers/esp32_camera_driver.h"
#include "../drivers/esp32_mvc_driver.h"
#include "../drivers/generic_rest_driver.h"
#include "SerialDebug.h"

IoTDeviceManager::IoTDeviceManager(WiFiManager* wifiManager, HttpClientManager* httpClient) {
    _discovery = new DeviceDiscovery(wifiManager, httpClient);
}

IoTDeviceManager::~IoTDeviceManager() {
    if (_discovery) {
        delete _discovery;
    }
}

bool IoTDeviceManager::begin() {
    DEBUG_PRINTLN("IoTDeviceManager: Initializing...");
    
    // Register built-in drivers
    registerDriver(new ESP32CameraDriver());
    registerDriver(new ESP32MVCDriver());
    registerDriver(new GenericRESTDriver());
    
    DEBUG_PRINTLN("IoTDeviceManager: Initialized successfully");
    return true;
}

void IoTDeviceManager::registerDriver(DeviceDriver* driver) {
    _discovery->registerDriver(driver);
}

void IoTDeviceManager::startDiscovery(unsigned long scanIntervalMs) {
    _discovery->startDiscovery(scanIntervalMs);
}

void IoTDeviceManager::stopDiscovery() {
    _discovery->stopDiscovery();
}

int IoTDeviceManager::scanForDevices() {
    return _discovery->scanForDevices();
}

void IoTDeviceManager::startManualScan() {
    _discovery->startManualScan();
}

bool IoTDeviceManager::isDiscoveryActive() const {
    return _discovery->isDiscoveryActive();
}

unsigned long IoTDeviceManager::getScanCount() const {
    JsonDocument stats = _discovery->getStatistics();
    return stats["total_scans"].as<unsigned long>();
}

std::vector<IoTDevice> IoTDeviceManager::getDevices() const {
    return _discovery->_devices;
}

std::vector<IoTDevice> IoTDeviceManager::getDiscoveredDevices() const {
    return getDevices(); // Alias for backward compatibility
}

std::vector<IoTDevice> IoTDeviceManager::getOnlineDevices() const {
    std::vector<IoTDevice> onlineDevices;
    for (const auto& device : _discovery->_devices) {
        if (device.isOnline) {
            onlineDevices.push_back(device);
        }
    }
    return onlineDevices;
}

IoTDevice* IoTDeviceManager::getDevice(const String& deviceId) {
    for (auto& device : _discovery->_devices) {
        if (device.id == deviceId) {
            return &device;
        }
    }
    return nullptr;
}

IoTDevice* IoTDeviceManager::getDeviceByIP(const String& ipAddress) {
    for (auto& device : _discovery->_devices) {
        if (device.ipAddress == ipAddress) {
            return &device;
        }
    }
    return nullptr;
}

std::vector<IoTDevice*> IoTDeviceManager::getDevicesByType(DeviceType type) {
    std::vector<IoTDevice*> devices;
    for (auto& device : _discovery->_devices) {
        if (device.type == type) {
            devices.push_back(&device);
        }
    }
    return devices;
}

std::vector<IoTDevice*> IoTDeviceManager::getDevicesWithCapability(DeviceCapability capability) {
    std::vector<IoTDevice*> devices;
    for (auto& device : _discovery->_devices) {
        if (device.capabilities & static_cast<uint32_t>(capability)) {
            devices.push_back(&device);
        }
    }
    return devices;
}

bool IoTDeviceManager::refreshDevice(const String& deviceId) {
    IoTDevice* device = getDevice(deviceId);
    if (device == nullptr || !device->hasHttpServer) {
        return false;
    }
    
    DeviceDriver* driver = findDriverForDevice(*device);
    if (driver == nullptr) {
        return false;
    }
    
    // Re-probe the device
    return driver->probe(*device, *(_discovery->_httpClient));
}

JsonDocument IoTDeviceManager::executeDeviceCommand(const String& deviceId, const String& command, 
                                                   const JsonDocument& parameters) {
    JsonDocument response;
    
    IoTDevice* device = getDevice(deviceId);
    if (device == nullptr) {
        response["error"] = "Device not found";
        return response;
    }
    
    if (!device->isOnline) {
        response["error"] = "Device is offline";
        return response;
    }
    
    DeviceDriver* driver = findDriverForDevice(*device);
    if (driver == nullptr) {
        response["error"] = "No driver available for device";
        return response;
    }
    
    try {
        if (driver->executeCommand(*device, command, parameters, *(_discovery->_httpClient))) {
            response["success"] = true;
        } else {
            response["error"] = "Command execution failed";
        }
    } catch (const std::exception& e) {
        response["error"] = String("Command execution error: ") + e.what();
    }
    
    return response;
}

void IoTDeviceManager::setDeviceDiscoveredCallback(std::function<void(const IoTDevice&)> callback) {
    _discovery->setDeviceDiscoveredCallback(callback);
}

void IoTDeviceManager::setDeviceStatusCallback(std::function<void(const IoTDevice&, bool)> callback) {
    _discovery->setDeviceStatusCallback(callback);
}

JsonDocument IoTDeviceManager::getStatistics() const {
    JsonDocument stats = _discovery->getStatistics();
    
    stats["online_devices"] = getOnlineDevices().size();
    
    // Device type breakdown
    JsonObject typeBreakdown = stats["device_types"].to<JsonObject>();
    for (const auto& device : _discovery->_devices) {
        String typeStr = deviceTypeToString(device.type);
        if (typeBreakdown[typeStr].is<int>()) {
            typeBreakdown[typeStr] = typeBreakdown[typeStr].as<int>() + 1;
        } else {
            typeBreakdown[typeStr] = 1;
        }
    }
    
    return stats;
}

String IoTDeviceManager::deviceTypeToString(DeviceType type) {
    return DeviceTypeUtils::deviceTypeToString(type);
}

std::vector<String> IoTDeviceManager::capabilitiesToStrings(uint32_t capabilities) {
    return DeviceTypeUtils::capabilitiesToStrings(capabilities);
}

DeviceDriver* IoTDeviceManager::findDriverForDevice(const IoTDevice& device) {
    return _discovery->findDriverForDevice(device);
}
