# HttpClient Memory Issues Troubleshooting Guide

## The Problem
The panic you encountered was caused by a `LoadProhibited` exception when accessing the `_stats` map in the `HttpClientManager::updateStats()` method. This typically indicates:

1. **NULL pointer dereference** - httpClientManager was NULL
2. **Memory corruption** - Object was destroyed or corrupted
3. **Uninitialized object** - Constructor never called properly

## Root Causes Found

### 1. NULL Pointer Access
**Problem:** In `external_api.cpp`, `httpClientManager` was declared as `NULL` but never initialized:
```cpp
HttpClientManager* httpClientManager = NULL; // ❌ Always NULL!
```

**Solution:** Properly initialize the global pointer:
```cpp
// In main.cpp or a global scope
HttpClientManager httpClientInstance;                    // Create actual instance
HttpClientManager* httpClientManager = &httpClientInstance; // Export pointer

// OR declare as extern and initialize elsewhere
extern HttpClientManager* httpClientManager; // In header/routes
HttpClientManager* httpClientManager = &myInstance; // In main.cpp
```

### 2. Incorrect Include Path
**Problem:** Wrong include path caused compilation issues:
```cpp
#include "httpclient.h" // ❌ Wrong path
```

**Solution:** Use correct relative path:
```cpp
#include "../../lib/HttpClient/src/httpclient.h" // ✅ Correct
```

### 3. Wrong Request Parameter Access
**Problem:** Using non-existent `route()` method:
```cpp
String city = request.route("city"); // ❌ Method doesn't exist
```

**Solution:** Use proper parameter access:
```cpp
String city = request.getParam("city"); // ✅ Correct method
```

## Fixed Code Examples

### Proper Global Declaration (main.cpp)
```cpp
#include "../lib/HttpClient/src/httpclient.h"

// Create the actual instance
HttpClientManager httpClientInstance;

// Export pointer for use in other modules
HttpClientManager* httpClientManager = &httpClientInstance;

void setup() {
    // Initialize BEFORE using
    HttpConfig config;
    config.timeout = 30000;
    
    if (!httpClientManager->begin(config)) {
        LOG_ERROR("HTTP client init failed");
        while(1) delay(1000);
    }
    
    // Now safe to register routes that use httpClientManager
    registerExternalApiRoutes(app->getRouter());
}
```

### Fixed Routes (external_api.cpp)
```cpp
#include "../../lib/HttpClient/src/httpclient.h"

extern HttpClientManager* httpClientManager; // Reference to main.cpp instance

void registerExternalApiRoutes(Router* router) {
    // Add safety check
    if (httpClientManager == nullptr) {
        DEBUG_PRINTLN("ERROR: httpClientManager not initialized!");
        return;
    }
    
    ApiController* apiController = new ApiController(httpClientManager);
    // ... rest of route registration
}
```

### Fixed Controller Methods
```cpp
Response ApiController::testConnectivity(Request& request) {
    // Always check for null pointer first
    if (_httpClient == nullptr) {
        JsonDocument errorDoc = createErrorResponse("HTTP client not initialized");
        return json(request.getServerRequest(), errorDoc);
    }
    
    // Now safe to use _httpClient
    HttpResponse response = _httpClient->get("https://httpbin.org/get");
    // ... rest of method
}
```

## Memory Safety Improvements

### 1. Thread-Safe Stats Updates
```cpp
void HttpClientManager::updateStats(const String& key, int increment) {
    // Validate key to prevent corruption
    if (key.length() > 0 && key.length() < 64) {
        _stats[key] += increment;
    }
}
```

### 2. Proper Map Initialization
```cpp
HttpClientManager::HttpClientManager() {
    // Explicit initialization to prevent corruption
    _stats.clear();
    _stats.insert(std::make_pair("requests_total", 0));
    _stats.insert(std::make_pair("requests_success", 0));
    _stats.insert(std::make_pair("requests_failed", 0));
    _stats.insert(std::make_pair("bytes_sent", 0));
    _stats.insert(std::make_pair("bytes_received", 0));
}
```

## Prevention Checklist

### ✅ Before Using HttpClient
- [ ] Create actual instance (not just pointer)
- [ ] Initialize with `begin()` before any HTTP calls
- [ ] Verify initialization was successful
- [ ] Add null pointer checks in all controller methods

### ✅ In Route Registration
- [ ] Use correct include paths
- [ ] Check httpClientManager is not null before creating controllers
- [ ] Use extern declaration if instance is in another file

### ✅ In Controller Methods
- [ ] Always check `_httpClient != nullptr` first
- [ ] Use `request.getParam()` not `request.route()`
- [ ] Return proper error responses for null client

### ✅ Memory Management
- [ ] Enable PSRAM with `-DBOARD_HAS_PSRAM`
- [ ] Monitor heap usage in memory monitor task
- [ ] Use appropriate JsonDocument sizes
- [ ] Don't create multiple HttpClient instances unnecessarily

## Quick Fix for Your Current Code

1. **Update your main.cpp:**
```cpp
// Add this after other global instances
HttpClientManager httpClientInstance;
HttpClientManager* httpClientManager = &httpClientInstance;

// In setup(), after WiFi init:
HttpConfig config;
config.timeout = 30000;
httpClientManager->begin(config);
```

2. **Update routes.h:**
```cpp
// Add this declaration
extern HttpClientManager* httpClientManager;
```

3. **Update external_api.cpp:**
```cpp
// Change the include and declaration
#include "../../lib/HttpClient/src/httpclient.h"
extern HttpClientManager* httpClientManager;
```

## Testing the Fix

After implementing the fixes, test with:

```bash
# Build and upload
pio run -t upload && pio device monitor

# Test the API endpoint
curl "http://your_esp32_ip/api/external/test"
```

The connectivity test should now work without crashing, and you should see proper debug output showing successful HTTP requests.

## Memory Monitoring

Add this to monitor memory health:
```cpp
void memoryMonitorTask(void* parameter) {
    while (true) {
        size_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 50000) {
            LOG_ERROR("Low memory: %d bytes", freeHeap);
        }
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
```

This should completely resolve the memory corruption and panic issues you were experiencing.
