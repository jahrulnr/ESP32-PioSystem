# PioSystem Copilot Instructions

## Project Overview
ESP32-S3 IoT device with WiFi hotspot management, dual interfaces (TFT + web), and custom MVC framework.

## Critical Architecture Patterns

### FreeRTOS Task Architecture
- **Core 0**: Memory monitoring, WiFi tasks, **autosleep task** (background)  
- **Core 1**: Input handling, display updates (UI critical)
- **Display Mutex**: ALWAYS use `xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000))` before TFT operations
- **Autosleep Task**: Monitors display timeout, handles automatic sleep/wake cycles
- Task creation pattern in `src/Tasks/Handler/tasks.cpp` with specific stack sizes (4096 bytes)

### Debugging System
Use `SerialDebug.h` macros throughout:
```cpp
DEBUG_PRINTLN("Message");           // Basic debug
LOG_ERROR("Error: %s", variable);   // Leveled logging
```
Enable via `-DSERIAL_DEBUG` in platformio.ini build flags.

### Manager-Based Hardware Abstraction
- `WiFiManager`: Handles AP/STA modes, client tracking, uses `std::vector<ClientInfo>` structures
- `DisplayManager`: Singleton pattern, manages sleep/wake, status bar rendering, autosleep timeout configuration
- `InputManager`: Button states with debouncing, hold detection (1000ms threshold), wake-up event handling
- **Autosleep System**: Automatic sleep after inactivity, wake on button press, long-press BACK for manual sleep
- All managers provide examples in `lib/*/examples/` directories

### Custom MVC Framework (`lib/MVCFramework/`)
- **Application**: Singleton with `getInstance()`, boot sequence, service container
- **Controllers**: Inherit from `Controller`, return `Response` objects with JSON
- **Routes**: Grouped registration pattern using lambdas in `registerApiRoutes()`
- **Models**: CSV-based persistence in `data/database/`
- **API Versioning**: Current routes use `/api/v1/` prefix with middleware chains
- **Response Helpers**: `json()` and `error()` helper methods in Controller base class

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
4. Monitor serial output for task stack usage and memory leaks

## Autosleep System Implementation
- **Task Location**: `src/Tasks/Handler/autosleep_task.cpp` on Core 0
- **Check Frequency**: 1-second intervals, mutex timeout 100ms to avoid blocking
- **Wake Events**: Any button press wakes display, activity updates on button input
- **Manual Sleep**: Long-press BACK button (1000ms threshold) triggers immediate sleep
- **Configuration**: Runtime adjustment via `setSleepTimeout(ms)`, default 60 seconds
- **Debug Output**: Task startup, timeout changes, wake events (enable with `-DSERIAL_DEBUG`)
- **Thread Safety**: All display operations use `displayMutex` with proper timeouts
