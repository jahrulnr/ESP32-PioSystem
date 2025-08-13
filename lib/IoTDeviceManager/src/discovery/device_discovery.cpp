#include "device_discovery.h"
#include "SerialDebug.h"

DeviceDiscovery::DeviceDiscovery(WiFiManager* wifiManager, HttpClientManager* httpClient) :
    _wifiManager(wifiManager),
    _httpClient(httpClient),
    _discoveryEnabled(false),
    _lastDiscoveryScan(0),
    _discoveryInterval(30000),
    _discoveryTaskHandle(nullptr),
    _totalScans(0),
    _devicesDiscovered(0),
    _lastScanDuration(0)
{
}

DeviceDiscovery::~DeviceDiscovery() {
    stopDiscovery();
    
    // Clean up drivers
    for (auto* driver : _drivers) {
        delete driver;
    }
    _drivers.clear();
}

void DeviceDiscovery::startDiscovery(unsigned long scanIntervalMs) {
    if (_discoveryEnabled) {
        DEBUG_PRINTLN("DeviceDiscovery: Discovery already running");
        return;
    }
    
    _discoveryInterval = scanIntervalMs;
    _discoveryEnabled = true;
    
    // Create discovery task on Core 0 (same as other background tasks)
    xTaskCreatePinnedToCore(
        discoveryTask,
        "device_discovery",
        4096,
        this,
        1,
        &_discoveryTaskHandle,
        0
    );
    
    DEBUG_PRINTF("DeviceDiscovery: Started discovery with %d ms interval\n", scanIntervalMs);
}

void DeviceDiscovery::stopDiscovery() {
    if (!_discoveryEnabled) return;
    
    _discoveryEnabled = false;
    
    if (_discoveryTaskHandle != nullptr) {
        vTaskDelete(_discoveryTaskHandle);
        _discoveryTaskHandle = nullptr;
    }
    
    DEBUG_PRINTLN("DeviceDiscovery: Stopped discovery");
}

int DeviceDiscovery::scanForDevices() {
    DEBUG_PRINTLN("DeviceDiscovery: Starting device scan...");
    
    unsigned long scanStart = millis();
    int newDevices = 0;
    
    // Get current connected clients from WiFiManager
    std::vector<ClientInfo> clients = _wifiManager->getConnectedClients();
    
    DEBUG_PRINTF("DeviceDiscovery: Found %d connected clients\n", clients.size());
    
    for (const auto& client : clients) {
        if (client.ipAddress == "0.0.0.0") {
            DEBUG_PRINTF("DeviceDiscovery: Skipping client with invalid IP: %s\n", client.macAddress.c_str());
            continue;
        }
        
        String deviceId = DeviceTypeUtils::createDeviceId(client.macAddress);
        IoTDevice* existingDevice = nullptr;
        
        // Find existing device
        for (auto& device : _devices) {
            if (device.id == deviceId) {
                existingDevice = &device;
                break;
            }
        }
        
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
            
            DEBUG_PRINTF("DeviceDiscovery: New device discovered - %s (%s)\n", 
                        newDevice.ipAddress.c_str(), newDevice.macAddress.c_str());
            
            // Check if device has HTTP server
            newDevice.hasHttpServer = checkHttpServer(newDevice.ipAddress);
            
            if (newDevice.hasHttpServer) {
                DEBUG_PRINTF("DeviceDiscovery: Device %s has HTTP server, probing...\n", 
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
                    
                    DEBUG_PRINTF("DeviceDiscovery: Device %s identified as %s\n",
                                newDevice.ipAddress.c_str(), 
                                DeviceTypeUtils::deviceTypeToString(newDevice.type).c_str());
                } else {
                    DEBUG_PRINTF("DeviceDiscovery: Failed to probe device %s\n", 
                                newDevice.ipAddress.c_str());
                }
            } else {
                DEBUG_PRINTF("DeviceDiscovery: Device %s has no HTTP server\n", 
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
    
    DEBUG_PRINTF("DeviceDiscovery: Scan completed in %d ms. Found %d new devices\n",
                _lastScanDuration, newDevices);
    
    return newDevices;
}

void DeviceDiscovery::startManualScan() {
    DEBUG_PRINTLN("DeviceDiscovery: Manual scan requested");
    _lastDiscoveryScan = 0; // Force immediate scan on next update
}

bool DeviceDiscovery::isDiscoveryActive() const {
    return _discoveryEnabled;
}

void DeviceDiscovery::registerDriver(DeviceDriver* driver) {
    if (driver != nullptr) {
        _drivers.push_back(driver);
        DEBUG_PRINTF("DeviceDiscovery: Registered driver: %s\n", driver->getDriverName().c_str());
    }
}

JsonDocument DeviceDiscovery::getStatistics() const {
    JsonDocument stats;
    
    stats["total_devices"] = _devices.size();
    stats["total_scans"] = _totalScans;
    stats["devices_discovered"] = _devicesDiscovered;
    stats["last_scan_duration_ms"] = _lastScanDuration;
    stats["discovery_enabled"] = _discoveryEnabled;
    stats["discovery_interval_ms"] = _discoveryInterval;
    
    return stats;
}

void DeviceDiscovery::setDeviceDiscoveredCallback(std::function<void(const IoTDevice&)> callback) {
    _deviceDiscoveredCallback = callback;
}

void DeviceDiscovery::setDeviceStatusCallback(std::function<void(const IoTDevice&, bool)> callback) {
    _deviceStatusCallback = callback;
}

bool DeviceDiscovery::checkHttpServer(const String& ipAddress) {
    DEBUG_PRINTF("DeviceDiscovery: Checking HTTP server on %s:80\n", ipAddress.c_str());
    
    // Use HttpClientManager to check if port 80 is open
    String testUrl = "http://" + ipAddress + "/";
    
    HttpResponse response = _httpClient->get(testUrl);
    
    // Any response (even 404) indicates an HTTP server is running
    if (response.statusCode > 0) {
        DEBUG_PRINTF("DeviceDiscovery: HTTP server detected on %s (status: %d)\n", 
                    ipAddress.c_str(), response.statusCode);
        return true;
    }
    
    DEBUG_PRINTF("DeviceDiscovery: No HTTP server on %s\n", ipAddress.c_str());
    return false;
}

bool DeviceDiscovery::probeDevice(IoTDevice& device) {
    DEBUG_PRINTF("DeviceDiscovery: Probing device %s\n", device.ipAddress.c_str());
    
    // Try each registered driver to see which one can handle this device
    for (auto* driver : _drivers) {
        DEBUG_PRINTF("DeviceDiscovery: Trying driver %s\n", driver->getDriverName().c_str());
        
        if (driver->probe(device, *_httpClient)) {
            DEBUG_PRINTF("DeviceDiscovery: Device %s handled by driver %s\n",
                        device.ipAddress.c_str(), driver->getDriverName().c_str());
            return true;
        }
    }
    
    DEBUG_PRINTF("DeviceDiscovery: No driver could handle device %s\n", device.ipAddress.c_str());
    return false;
}

DeviceDriver* DeviceDiscovery::findDriverForDevice(const IoTDevice& device) {
    for (auto* driver : _drivers) {
        if (driver->canHandle(device)) {
            return driver;
        }
    }
    return nullptr;
}

void DeviceDiscovery::updateDeviceStatus(IoTDevice& device, bool isOnline) {
    if (device.isOnline != isOnline) {
        device.isOnline = isOnline;
        device.lastSeen = millis();
        
        DEBUG_PRINTF("DeviceDiscovery: Device %s (%s) is now %s\n",
                    device.name.c_str(), device.ipAddress.c_str(),
                    isOnline ? "online" : "offline");
        
        if (_deviceStatusCallback) {
            _deviceStatusCallback(device, isOnline);
        }
    }
}

void DeviceDiscovery::discoveryTask(void* parameter) {
    DeviceDiscovery* discovery = static_cast<DeviceDiscovery*>(parameter);
    
    DEBUG_PRINTLN("DeviceDiscovery: Discovery task started");
    
    while (discovery->_discoveryEnabled) {
        unsigned long now = millis();
        
        if (now - discovery->_lastDiscoveryScan >= discovery->_discoveryInterval) {
            discovery->scanForDevices();
            discovery->_lastDiscoveryScan = now;
        }
        
        // Sleep for 1 second before checking again
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    DEBUG_PRINTLN("DeviceDiscovery: Discovery task ended");
    vTaskDelete(NULL);
}
