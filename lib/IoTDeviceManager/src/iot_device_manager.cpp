#include "iot_device_manager.h"
#include "SerialDebug.h"
#include <ESPmDNS.h>

IoTDeviceManager::IoTDeviceManager(WiFiManager* wifiManager, HttpClientManager* httpClient) :
    _wifiManager(wifiManager),
    _httpClient(httpClient),
    _discoveryEnabled(false),
    _lastDiscoveryScan(0),
    _discoveryInterval(30000),
    _totalScans(0),
    _devicesDiscovered(0),
    _lastScanDuration(0),
    _discoveryTaskHandle(nullptr)
{
}

IoTDeviceManager::~IoTDeviceManager() {
    stopDiscovery();
    
    // Clean up drivers
    for (auto* driver : _drivers) {
        delete driver;
    }
    _drivers.clear();
}

bool IoTDeviceManager::begin() {
    DEBUG_PRINTLN("IoTDeviceManager: Initializing...");
    
    // Register built-in drivers
    registerDriver(new ESP32CameraDriver());
    registerDriver(new ESP32MVCDriver());
    registerDriver(new GenericRESTDriver());
    
    DEBUG_PRINTF("IoTDeviceManager: Registered %d device drivers\n", _drivers.size());
    return true;
}

void IoTDeviceManager::registerDriver(DeviceDriver* driver) {
    if (driver != nullptr) {
        _drivers.push_back(driver);
        DEBUG_PRINTF("IoTDeviceManager: Registered driver: %s\n", driver->getDriverName().c_str());
    }
}

void IoTDeviceManager::startDiscovery(unsigned long scanIntervalMs) {
    if (_discoveryEnabled) {
        DEBUG_PRINTLN("IoTDeviceManager: Discovery already running");
        return;
    }
    
    _discoveryInterval = scanIntervalMs;
    _discoveryEnabled = true;
    
    // Create discovery task on Core 0 (same as other background tasks)
    xTaskCreatePinnedToCore(
        discoveryTask,
        "iot_discovery",
        4096,
        this,
        1,
        &_discoveryTaskHandle,
        0
    );
    
    DEBUG_PRINTF("IoTDeviceManager: Started discovery with %d ms interval\n", scanIntervalMs);
}

void IoTDeviceManager::stopDiscovery() {
    if (!_discoveryEnabled) return;
    
    _discoveryEnabled = false;
    
    if (_discoveryTaskHandle != nullptr) {
        vTaskDelete(_discoveryTaskHandle);
        _discoveryTaskHandle = nullptr;
    }
    
    DEBUG_PRINTLN("IoTDeviceManager: Stopped discovery");
}

int IoTDeviceManager::scanForDevices() {
    DEBUG_PRINTLN("IoTDeviceManager: Starting device scan...");
    
    unsigned long scanStart = millis();
    int newDevices = 0;
    
    // Get current connected clients from WiFiManager
    std::vector<ClientInfo> clients = _wifiManager->getConnectedClients();
    
    DEBUG_PRINTF("IoTDeviceManager: Found %d connected clients\n", clients.size());
    
    for (const auto& client : clients) {
        if (client.ipAddress == "0.0.0.0") {
            DEBUG_PRINTF("IoTDeviceManager: Skipping client with invalid IP: %s\n", client.macAddress.c_str());
            continue;
        }
        
        String deviceId = createDeviceId(client.macAddress);
        IoTDevice* existingDevice = getDevice(deviceId);
        
        if (existingDevice == nullptr) {
            // New device discovered
            IoTDevice newDevice;
            newDevice.id = deviceId;
            newDevice.macAddress = client.macAddress;
            newDevice.ipAddress = client.ipAddress;
            newDevice.hostname = client.hostname;
            newDevice.discoveredAt = millis();
            newDevice.lastSeen = millis();
            newDevice.baseUrl = "http://" + client.ipAddress;
            
            DEBUG_PRINTF("IoTDeviceManager: New device discovered - %s (%s)\n", 
                        newDevice.ipAddress.c_str(), newDevice.macAddress.c_str());
            
            // Check if device has HTTP server
            newDevice.hasHttpServer = checkHttpServer(newDevice.ipAddress);
            
            if (newDevice.hasHttpServer) {
                DEBUG_PRINTF("IoTDeviceManager: Device %s has HTTP server, probing...\n", 
                            newDevice.ipAddress.c_str());
                
                // Probe device to determine type and capabilities
                if (probeDevice(newDevice)) {
                    newDevice.isOnline = true;
                    _devices.push_back(newDevice);
                    newDevices++;
                    _devicesDiscovered++;
                    
                    // Call discovery callback
                    if (_deviceDiscoveredCallback) {
                        _deviceDiscoveredCallback(newDevice);
                    }
                    
                    DEBUG_PRINTF("IoTDeviceManager: Device %s identified as %s\n",
                                newDevice.ipAddress.c_str(), 
                                deviceTypeToString(newDevice.type).c_str());
                } else {
                    DEBUG_PRINTF("IoTDeviceManager: Failed to probe device %s\n", 
                                newDevice.ipAddress.c_str());
                }
            } else {
                DEBUG_PRINTF("IoTDeviceManager: Device %s has no HTTP server\n", 
                            newDevice.ipAddress.c_str());
            }
        } else {
            // Update existing device
            bool wasOnline = existingDevice->isOnline;
            existingDevice->ipAddress = client.ipAddress;
            existingDevice->hostname = client.hostname;
            existingDevice->lastSeen = millis();
            
            // Check if device is still online
            if (existingDevice->hasHttpServer) {
                bool isOnline = checkHttpServer(existingDevice->ipAddress);
                if (isOnline != wasOnline) {
                    updateDeviceStatus(*existingDevice, isOnline);
                }
            }
        }
    }
    
    // Mark devices as offline if they're no longer connected
    for (auto& device : _devices) {
        bool found = false;
        for (const auto& client : clients) {
            if (device.macAddress.equalsIgnoreCase(client.macAddress)) {
                found = true;
                break;
            }
        }
        
        if (!found && device.isOnline) {
            updateDeviceStatus(device, false);
        }
    }
    
    _totalScans++;
    _lastScanDuration = millis() - scanStart;
    
    DEBUG_PRINTF("IoTDeviceManager: Scan completed in %d ms. Found %d new devices\n",
                _lastScanDuration, newDevices);
    
    return newDevices;
}

void IoTDeviceManager::startManualScan() {
    DEBUG_PRINTLN("IoTDeviceManager: Manual scan requested");
    _lastDiscoveryScan = 0; // Force immediate scan on next update
}

bool IoTDeviceManager::isDiscoveryActive() const {
    return _discoveryEnabled;
}

unsigned long IoTDeviceManager::getScanCount() const {
    return _totalScans;
}

std::vector<IoTDevice> IoTDeviceManager::getDiscoveredDevices() const {
    return _devices;
}

std::vector<IoTDevice> IoTDeviceManager::getDevices() const {
    return _devices;
}

std::vector<IoTDevice> IoTDeviceManager::getOnlineDevices() const {
    std::vector<IoTDevice> onlineDevices;
    for (const auto& device : _devices) {
        if (device.isOnline) {
            onlineDevices.push_back(device);
        }
    }
    return onlineDevices;
}

IoTDevice* IoTDeviceManager::getDevice(const String& deviceId) {
    for (auto& device : _devices) {
        if (device.id == deviceId) {
            return &device;
        }
    }
    return nullptr;
}

IoTDevice* IoTDeviceManager::getDeviceByIP(const String& ipAddress) {
    for (auto& device : _devices) {
        if (device.ipAddress == ipAddress) {
            return &device;
        }
    }
    return nullptr;
}

std::vector<IoTDevice*> IoTDeviceManager::getDevicesByType(DeviceType type) {
    std::vector<IoTDevice*> devices;
    for (auto& device : _devices) {
        if (device.type == type) {
            devices.push_back(&device);
        }
    }
    return devices;
}

std::vector<IoTDevice*> IoTDeviceManager::getDevicesWithCapability(DeviceCapability capability) {
    std::vector<IoTDevice*> devices;
    for (auto& device : _devices) {
        if (device.capabilities & static_cast<uint32_t>(capability)) {
            devices.push_back(&device);
        }
    }
    return devices;
}

bool IoTDeviceManager::refreshDevice(const String& deviceId) {
    IoTDevice* device = getDevice(deviceId);
    if (device == nullptr) {
        return false;
    }
    
    if (!device->hasHttpServer) {
        return false;
    }
    
    return probeDevice(*device);
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
        if (driver->executeCommand(*device, command, parameters, *_httpClient)) {
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
    _deviceDiscoveredCallback = callback;
}

void IoTDeviceManager::setDeviceStatusCallback(std::function<void(const IoTDevice&, bool)> callback) {
    _deviceStatusCallback = callback;
}

JsonDocument IoTDeviceManager::getStatistics() const {
    JsonDocument stats;
    
    stats["total_devices"] = _devices.size();
    stats["online_devices"] = getOnlineDevices().size();
    stats["total_scans"] = _totalScans;
    stats["devices_discovered"] = _devicesDiscovered;
    stats["last_scan_duration_ms"] = _lastScanDuration;
    stats["discovery_enabled"] = _discoveryEnabled;
    stats["discovery_interval_ms"] = _discoveryInterval;
    
    // Device type breakdown
    JsonObject typeBreakdown = stats["device_types"].to<JsonObject>();
    for (const auto& device : _devices) {
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
    switch (type) {
        case DeviceType::ESP32_CAMERA: return "ESP32 Camera";
        case DeviceType::ESP32_SENSOR: return "ESP32 Sensor";
        case DeviceType::ESP32_CONTROLLER: return "ESP32 Controller";
        case DeviceType::ESP32_DISPLAY: return "ESP32 Display";
        case DeviceType::RASPBERRY_PI: return "Raspberry Pi";
        case DeviceType::ARDUINO_IOT: return "Arduino IoT";
        case DeviceType::CUSTOM_DEVICE: return "Custom Device";
        default: return "Unknown";
    }
}

std::vector<String> IoTDeviceManager::capabilitiesToStrings(uint32_t capabilities) {
    std::vector<String> caps;
    
    if (capabilities & static_cast<uint32_t>(DeviceCapability::CAMERA)) caps.push_back("Camera");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::SENSORS)) caps.push_back("Sensors");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::DISPLAY_SCREEN)) caps.push_back("Display");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::ACTUATORS)) caps.push_back("Actuators");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::STORAGE)) caps.push_back("Storage");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::NETWORKING)) caps.push_back("Networking");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::AUTHENTICATION)) caps.push_back("Authentication");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::WEBSOCKET)) caps.push_back("WebSocket");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::FILE_UPLOAD)) caps.push_back("File Upload");
    if (capabilities & static_cast<uint32_t>(DeviceCapability::SYSTEM_CONTROL)) caps.push_back("System Control");
    
    return caps;
}

bool IoTDeviceManager::checkHttpServer(const String& ipAddress) {
    DEBUG_PRINTF("IoTDeviceManager: Checking HTTP server on %s:80\n", ipAddress.c_str());
    
    // Use HttpClientManager to check if port 80 is open
    String testUrl = "http://" + ipAddress + "/";
    
    HttpResponse response = _httpClient->get(testUrl);
    
    // Any response (even 404) indicates an HTTP server is running
    if (response.statusCode > 0) {
        DEBUG_PRINTF("IoTDeviceManager: HTTP server detected on %s (status: %d)\n", 
                    ipAddress.c_str(), response.statusCode);
        return true;
    }
    
    DEBUG_PRINTF("IoTDeviceManager: No HTTP server on %s\n", ipAddress.c_str());
    return false;
}

bool IoTDeviceManager::probeDevice(IoTDevice& device) {
    DEBUG_PRINTF("IoTDeviceManager: Probing device %s\n", device.ipAddress.c_str());
    
    // Try each registered driver to see which one can handle this device
    for (auto* driver : _drivers) {
        DEBUG_PRINTF("IoTDeviceManager: Trying driver %s\n", driver->getDriverName().c_str());
        
        if (driver->probe(device, *_httpClient)) {
            DEBUG_PRINTF("IoTDeviceManager: Device %s handled by driver %s\n",
                        device.ipAddress.c_str(), driver->getDriverName().c_str());
            return true;
        }
    }
    
    DEBUG_PRINTF("IoTDeviceManager: No driver could handle device %s\n", device.ipAddress.c_str());
    return false;
}

String IoTDeviceManager::createDeviceId(const String& macAddress) {
    // Remove colons and convert to lowercase for consistent ID
    String id = macAddress;
    id.replace(":", "");
    id.toLowerCase();
    return "iot_" + id;
}

DeviceDriver* IoTDeviceManager::findDriverForDevice(const IoTDevice& device) {
    for (auto* driver : _drivers) {
        if (driver->canHandle(device)) {
            return driver;
        }
    }
    return nullptr;
}

void IoTDeviceManager::updateDeviceStatus(IoTDevice& device, bool isOnline) {
    if (device.isOnline != isOnline) {
        device.isOnline = isOnline;
        device.lastSeen = millis();
        
        DEBUG_PRINTF("IoTDeviceManager: Device %s (%s) is now %s\n",
                    device.name.c_str(), device.ipAddress.c_str(),
                    isOnline ? "online" : "offline");
        
        if (_deviceStatusCallback) {
            _deviceStatusCallback(device, isOnline);
        }
    }
}

void IoTDeviceManager::discoveryTask(void* parameter) {
    IoTDeviceManager* manager = static_cast<IoTDeviceManager*>(parameter);
    
    DEBUG_PRINTLN("IoTDeviceManager: Discovery task started");
    
    while (manager->_discoveryEnabled) {
        unsigned long now = millis();
        
        if (now - manager->_lastDiscoveryScan >= manager->_discoveryInterval) {
            manager->scanForDevices();
            manager->_lastDiscoveryScan = now;
        }
        
        // Sleep for 1 second before checking again
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    DEBUG_PRINTLN("IoTDeviceManager: Discovery task ended");
    vTaskDelete(NULL);
}
