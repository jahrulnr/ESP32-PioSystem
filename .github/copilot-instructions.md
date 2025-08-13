# PioSystem Copilot Instructions

## Project Overview
ESP32-S3 IoT device with WiFi hotspot management, dual interfaces (TFT + web), custom MVC framework, IoT device discovery system, and real-time WebSocket communication.

## Critical Architecture Patterns

### FreeRTOS Task Architecture
- **Core 0**: Memory monitoring, WiFi tasks, **autosleep task**, IoT discovery, **WebSocket management** (background)  
- **Core 1**: Input handling, display updates (UI critical)
- **Display Mutex**: ALWAYS use `xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000))` before TFT operations
- **Autosleep Task**: Monitors display timeout, handles automatic sleep/wake cycles
- **IoT Discovery Task**: Scans network for ESP32 devices with 30-second intervals
- **WebSocket Task**: Real-time communication with discovered devices on Core 0
- Task creation pattern in `src/Tasks/Handler/tasks.cpp` with specific stack sizes (4096 bytes)

### Debugging System
Use `SerialDebug.h` macros throughout:
```cpp
DEBUG_PRINTLN("Message");           // Basic debug
LOG_ERROR("Error: %s", variable);   // Leveled logging
```
Enable via `-DSERIAL_DEBUG` in platformio.ini build flags.

### Manager-Based Hardware Abstraction
- `WiFiManager`: Handles AP/STA modes, client tracking with proper uint32_t IP conversion, uses `std::vector<ClientInfo>` structures
- `DisplayManager`: Singleton pattern, manages sleep/wake, status bar rendering, autosleep timeout configuration
- `InputManager`: Button states with debouncing, hold detection (1000ms threshold), wake-up event handling
- `IoTDeviceManager`: Auto-discovery of ESP32 devices, capability detection (Camera, Sensors, MVC), HTTP client management
- `WebSocketClientManager`: Real-time device communication with auto-reconnection, message routing, and callback system
- `HttpClientManager`: Manages HTTP connections for device discovery and API communication
- **Autosleep System**: Automatic sleep after inactivity, wake on button press, long-press BACK for manual sleep
- All managers provide examples in `lib/*/examples/` directories

### Custom MVC Framework (`lib/MVCFramework/`)
- **Application**: Singleton with `getInstance()`, boot sequence, service container
- **Controllers**: Inherit from `Controller`, return `Response` objects with JSON
- **Routes**: Grouped registration pattern using lambdas in `registerApiRoutes()`
- **Models**: CSV-based persistence in `data/database/`
- **API Versioning**: Current routes use `/api/v1/` prefix with middleware chains
- **Response Helpers**: `json()` and `error()` helper methods in Controller base class
- **IoT Integration**: Controllers like `IoTDeviceController` manage device discovery and command execution
- **WebSocket Routes**: Fluent API with `.onConnect()`, `.onDisconnect()`, `.onMessage()` chaining

## Essential Build Commands
```bash
# Build and upload with monitor
pio run -t upload && pio device monitor

# Check memory usage
pio run -v | grep -E "(RAM|Flash)"

# Format SPIFFS and upload filesystem
pio run -t uploadfs
```

**Platform Configuration:**
- Board: `esp32-s3-devkitc-1` with Arduino framework
- PSRAM support available but commented out (enable `-DBOARD_HAS_PSRAM` if needed)
- Build flags include `-DSERIAL_DEBUG` for debug output and FreeRTOS configuration

## API Response Patterns
Controllers use Response objects with fluent interface. Two main patterns exist:

**Standard MVC Pattern (preferred for new code):**
```cpp
Response WifiController::method(Request& request) {
    JsonDocument doc;
    doc["status"] = "success";  // Required status field
    doc["data"] = /* ... */;    // Main response data
    return json(request.getServerRequest(), doc);
}
```

**Alternative Response Builder Pattern (current in api.cpp):**
```cpp
Response AuthController::method(Request& request) {
    JsonDocument response;
    response["success"] = true;  // Note: uses "success" not "status"
    response["users"] = JsonArray();
    
    return Response(request.getServerRequest())
            .status(200)
            .json(response);
}
```

**Error Response Pattern:**
```cpp
return error(request.getServerRequest(), "Error message");  // Helper method
// OR
JsonDocument errorDoc;
errorDoc["status"] = "error";
errorDoc["message"] = "Description of error";
return json(request.getServerRequest(), errorDoc);
```

## Route Registration Patterns
Routes MUST use grouped lambdas for organization:
```cpp
void registerWifiRoutes(Router* router, WiFiManager* wifiManager) {
    WifiController* wifiController = new WifiController(wifiManager);
    
    router->group("/api/wifi", [&](Router& wifi) {
        wifi.middleware({"cors", "json"});
        
        wifi.get("/status", [wifiController](Request& request) -> Response {
            return wifiController->status(request);
        }).name("api.wifi.status");
    });
}
```

**Middleware Patterns:**
- `/api/v1/` routes use `{"cors", "json", "ratelimit"}` middleware
- Admin routes add `{"auth", "admin", "json"}` middleware for authentication
- WiFi routes use `{"cors", "json"}` for public access
- IoT routes use `{"cors", "json"}` for device discovery API access

## Hardware Integration Points
- **Pin Definitions**: `BUTTON_UP=4, BUTTON_DOWN=5, BUTTON_SELECT=9, BUTTON_BACK=6`
- **Battery Monitoring**: ADC pin 1, voltage divider ratio 2, defined in `include/battery_config.h`
- **Display Sleep**: Auto-sleep with configurable timeout via `DisplayManager::setSleepTimeout()`
- **Sleep Controls**: `forceSleep()` for immediate sleep, long-press BACK button, automatic wake on any button press
- **Power Management**: CPU frequency scaling, backlight control (TFT_BL), display controller power states

## Memory Management Critical Rules
1. Use PSRAM for large allocations (enabled with `-DBOARD_HAS_PSRAM`)
2. Monitor with dedicated `memoryMonitorTask` on Core 0
3. JsonDocument sizing: Network responses need ~2KB, adjust per endpoint
4. SPIFFS: Verify initialization, create `/database` directory programmatically

## File System Structure
- `data/views/`: HTML templates served via SPIFFS
- `data/assets/`: Bootstrap CSS/JS for web interface  
- `data/database/`: CSV storage for models
- Web server serves static files from root `/`

## Error Handling Conventions
- WiFi operations: Check return booleans, fallback to AP mode
- SPIFFS: Always check `SPIFFS.begin(true)` result
- Tasks: Use `vTaskDelete(NULL)` in main loop to free Arduino loop task
- Mutex timeouts: Use `pdMS_TO_TICKS(1000)` standard timeout

## Integration Testing Approach
1. Physical interface via TFT menu navigation (see `src/Tasks/Menus/`)
2. Web interface at `http://192.168.4.1` when in AP mode
3. API testing: `/api/wifi/status`, `/api/wifi/clients` endpoints
4. IoT API testing: `/api/v1/iot/devices`, `/api/v1/iot/devices/online` endpoints
5. Monitor serial output for task stack usage and memory leaks

## WiFi Event Handling Patterns
**Critical Issue**: WiFi events use different structures for different event types:
- `ARDUINO_EVENT_WIFI_AP_STACONNECTED`: Uses `info.wifi_ap_staconnected.mac`
- `ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED`: Uses `info.got_ip.ip_info.ip.addr` (uint32_t format)
- **IP Conversion**: Convert uint32_t to IPAddress with: `IPAddress(addr & 0xFF, (addr >> 8) & 0xFF, (addr >> 16) & 0xFF, (addr >> 24) & 0xFF)`
- **Client Tracking**: IP assignment events don't contain MAC - update existing clients with `ipAddress == "0.0.0.0"`

## Modular IoTDeviceManager Architecture
The IoTDeviceManager follows a clean modular architecture with separation of concerns:

### Directory Structure (`lib/IoTDeviceManager/src/`)
- `types/device_types.h` - Core data structures, enums, and device definitions
- `drivers/` - Device-specific protocol handlers with base `DeviceDriver` interface
- `discovery/device_discovery.h` - Network scanning engine with FreeRTOS task
- `core/iot_device_manager_core.h` - Main management interface and API

### Device Driver Pattern
All device drivers inherit from `DeviceDriver` base class:
```cpp
class CustomDriver : public DeviceDriver {
public:
    DeviceType getDeviceType() const override;
    String getDriverName() const override;
    bool canHandle(const IoTDevice& device) const override;
    bool probe(IoTDevice& device, HttpClientManager& httpClient) override;
    JsonDocument getDeviceInfo(const IoTDevice& device, HttpClientManager& httpClient) override;
    bool executeCommand(const IoTDevice& device, const String& command, 
                       const JsonDocument& params, HttpClientManager& httpClient) override;
};
```

### Built-in Drivers
- `ESP32CameraDriver`: Handles ESP32-CAM devices with camera streaming capabilities
- `ESP32MVCDriver`: Manages ESP32 devices running the MVC framework (like this system)
- `GenericRESTDriver`: Fallback driver for unknown devices with basic HTTP endpoints

### Driver Registration Pattern
```cpp
// Automatic registration of built-in drivers in IoTDeviceManager::begin()
registerDriver(new ESP32CameraDriver());
registerDriver(new ESP32MVCDriver());
registerDriver(new GenericRESTDriver());

// Custom driver registration
manager->registerDriver(new MyCustomDriver());
```

### Device Types and Capabilities System
**Device Types** (enum in `types/device_types.h`):
- `ESP32_CAMERA` - Camera-enabled ESP32 devices
- `ESP32_SENSOR` - Sensor-focused ESP32 devices  
- `ESP32_CONTROLLER` - Control/automation ESP32 devices
- `ESP32_MVC` - Devices running this MVC framework
- `ARDUINO_IOT` - Arduino-based IoT devices
- `RASPBERRY_PI` - Raspberry Pi devices
- `GENERIC_DEVICE` - Unknown/generic HTTP devices

**Device Capabilities** (bitfield system):
- `CAMERA` - Image capture capability
- `SENSORS` - Environmental/data sensors
- `ACTUATORS` - Control motors, relays, etc.
- `DISPLAY_SCREEN` - Has visual display
- `NETWORKING` - Network communication
- `STORAGE` - Local storage capability
- `WEBSOCKET` - WebSocket communication support

### Helper Utilities
- `DeviceTypeUtils::deviceTypeToString()` - Convert enum to string
- `DeviceTypeUtils::capabilitiesToStrings()` - Convert bitfield to string array
- `DeviceTypeUtils::parseDeviceType()` - String to enum conversion
- `setDeviceProperties()` - Helper for driver implementations

### Custom Driver Development
1. Create driver class inheriting from `DeviceDriver`
2. Implement required virtual methods (probe, executeCommand, etc.)
3. Use `setDeviceProperties()` helper to configure device type and capabilities
4. Register driver with IoTDeviceManager before starting discovery
5. Examples available in `lib/IoTDeviceManager/examples/custom_driver.cpp`

### Device Discovery Process
1. Network scan discovers devices via `WiFiManager::getConnectedClients()`
2. HTTP server check determines if device has web interface
3. Driver probing attempts to identify device type and capabilities
4. Successful devices added to managed device list
5. Callbacks notify application of discovery events

## IoT Device Discovery System
- **Auto-discovery**: Network scanning for ESP32 devices every 30 seconds on Core 0
- **Device Types**: ESP32_CAMERA, ESP32_SENSOR, ESP32_CONTROLLER, ESP32_DISPLAY, etc.
- **Capabilities**: Bitfield system using `DeviceCapability` enum (CAMERA, SENSORS, DISPLAY_SCREEN, etc.)
- **Modular Drivers**: Extensible driver system in `lib/IoTDeviceManager/src/drivers/`
- **Callbacks**: `setDeviceDiscoveredCallback()` and `setDeviceStatusCallback()` for real-time updates

## WebSocket Communication System
- **Library Location**: `lib/WebSocketClient/` - dedicated WebSocket client manager for device-to-device communication
- **Message Types**: Enum-based system (PING, PONG, DEVICE_STATUS, COMMAND, RESPONSE, CAMERA_STREAM, SENSOR_DATA)
- **Connection Management**: Auto-reconnection with configurable attempts, ping/pong keepalive (30s intervals)
- **Task Integration**: Dedicated WebSocket task on Core 0 with 100ms update cycles
- **Device Integration**: Automatic connection to discovered IoT devices with WebSocket capability
- **Callback Pattern**: `setMessageCallback()` and `setConnectionCallback()` for event handling
- **API Routes**: `/api/v1/websocket/` endpoints for connection management and command sending
- **Real-time Features**: Live sensor streaming, camera feeds, immediate command execution between devices

## Autosleep System Implementation
- **Task Location**: `src/Tasks/Handler/autosleep_task.cpp` on Core 0
- **Check Frequency**: 1-second intervals, mutex timeout 100ms to avoid blocking
- **Wake Events**: Any button press wakes display, activity updates on button input
- **Manual Sleep**: Long-press BACK button (1000ms threshold) triggers immediate sleep
- **Configuration**: Runtime adjustment via `setSleepTimeout(ms)`, default 60 seconds
- **Debug Output**: Task startup, timeout changes, wake events (enable with `-DSERIAL_DEBUG`)
- **Thread Safety**: All display operations use `displayMutex` with proper timeouts
