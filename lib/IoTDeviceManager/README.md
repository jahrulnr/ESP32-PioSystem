# IoT Device Manager - Modular Architecture

## Overview

The IoT Device Manager has been refactored into a modular architecture for better maintainability, testability, and extensibility. The system is now organized into focused modules with clear responsibilities.

## Architecture

```
lib/IoTDeviceManager/src/
├── types/                    # Core data structures and utilities
│   ├── device_types.h       # Device types, capabilities, IoTDevice struct
│   └── device_types.cpp     # Utility functions for types
├── drivers/                 # Device-specific protocol handlers
│   ├── device_driver.h      # Base driver interface
│   ├── device_driver.cpp    # Common driver functionality
│   ├── esp32_camera_driver.h/.cpp    # ESP32 camera device support
│   ├── esp32_mvc_driver.h/.cpp       # ESP32 MVC framework support
│   └── generic_rest_driver.h/.cpp    # Generic REST device support
├── discovery/               # Network scanning and device identification
│   ├── device_discovery.h   # Discovery engine interface
│   └── device_discovery.cpp # Discovery implementation with FreeRTOS task
├── core/                    # Main management interface
│   ├── iot_device_manager_core.h     # Core manager interface
│   └── iot_device_manager_core.cpp   # Core manager implementation
├── iot_device_manager.h     # Main include file (backward compatible)
├── iot_device_manager.cpp   # Simple adapter file
└── iot_device_manager_legacy.cpp     # Original monolithic implementation
```

## Key Benefits

### 1. **Separation of Concerns**
- **Types**: Pure data structures and utilities
- **Drivers**: Device-specific protocol handling
- **Discovery**: Network scanning and device identification
- **Core**: High-level device management

### 2. **Better Testability**
- Each module can be unit tested independently
- Mock drivers can be easily created for testing
- Discovery logic is isolated from device management

### 3. **Extensibility**
- New device drivers can be added without modifying core code
- Discovery strategies can be enhanced independently
- New device types are added in one place

### 4. **Maintainability**
- Clear module boundaries reduce complexity
- Changes to one module don't affect others
- Easier to understand and debug

## Usage (Backward Compatible)

The public API remains unchanged for backward compatibility:

```cpp
#include "iot_device_manager.h"

// Same usage as before
IoTDeviceManager* manager = new IoTDeviceManager(wifiManager, httpClient);
manager->begin();
manager->startDiscovery();

// All existing methods work the same way
std::vector<IoTDevice> devices = manager->getDevices();
IoTDevice* device = manager->getDevice("device_id");
manager->executeDeviceCommand("device_id", "command", params);
```

## Adding Custom Device Drivers

Creating a new device driver is now more structured:

```cpp
#include "drivers/device_driver.h"

class MyCustomDriver : public DeviceDriver {
public:
    DeviceType getDeviceType() const override {
        return DeviceType::CUSTOM_DEVICE;
    }
    
    String getDriverName() const override {
        return "MyCustomDriver";
    }
    
    bool canHandle(const IoTDevice& device) const override {
        return device.type == DeviceType::CUSTOM_DEVICE;
    }
    
    bool probe(IoTDevice& device, HttpClientManager& httpClient) override {
        // Check device-specific endpoints
        if (checkEndpoint(device.baseUrl, "/my/api", httpClient)) {
            setDeviceProperties(device, DeviceType::CUSTOM_DEVICE, 
                              static_cast<uint32_t>(DeviceCapability::NETWORKING));
            return true;
        }
        return false;
    }
    
    // ... implement other methods
};

// Register the driver
manager->registerDriver(new MyCustomDriver());
```

## Module Dependencies

```
types/ ← drivers/ ← discovery/ ← core/
  ↑       ↑         ↑         ↑
  └───────┴─────────┴─────────┘
           iot_device_manager.h
```

- **types/** has no dependencies (pure data structures)
- **drivers/** depends only on types and external HTTP client
- **discovery/** depends on types and drivers
- **core/** orchestrates all modules
- **iot_device_manager.h** includes all modules for easy use

## FreeRTOS Task Integration

The discovery module runs its own FreeRTOS task on Core 0, following the project's task architecture patterns:

```cpp
// Task creation in DeviceDiscovery::startDiscovery()
xTaskCreatePinnedToCore(
    discoveryTask,
    "device_discovery",
    4096,                // Stack size
    this,
    1,                   // Priority
    &_discoveryTaskHandle,
    0                    // Core 0 (background tasks)
);
```

## Memory Management

- Each module manages its own resources
- Drivers are automatically cleaned up by discovery engine
- Clear ownership boundaries prevent memory leaks
- RAII principles used throughout

## Debugging

Each module uses the same SerialDebug system with module-specific prefixes:

```cpp
DEBUG_PRINTF("DeviceDiscovery: Found %d devices\n", count);
DEBUG_PRINTF("ESP32CameraDriver: Probing device %s\n", ip.c_str());
DEBUG_PRINTF("IoTDeviceManager: Executing command %s\n", cmd.c_str());
```

## Migration Notes

If you're extending the original code:

1. **Adding device types**: Modify `types/device_types.h`
2. **Creating drivers**: Extend `drivers/device_driver.h`
3. **Modifying discovery**: Edit `discovery/device_discovery.cpp`
4. **Changing core logic**: Update `core/iot_device_manager_core.cpp`

The legacy implementation remains in `iot_device_manager_legacy.cpp` for reference.
