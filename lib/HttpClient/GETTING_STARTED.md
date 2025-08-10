# Getting Started with HttpClient Library

## Quick Integration Guide

Follow these steps to add HTTP/HTTPS API capabilities to your PioSystem project:

### 1. Basic Setup

The HttpClient library is already included in your project. Here's the minimal setup:

```cpp
#include "../lib/HttpClient/src/httpclient.h"

// Global instance
HttpClientManager httpClient;

void setup() {
    // Initialize WiFi first (using your existing WiFiManager)
    wifiManager.connectToNetwork("your_ssid", "your_password");
    
    // Initialize HTTP client
    HttpConfig config;
    config.timeout = 30000;
    httpClient.begin(config);
    httpClient.setDebugEnabled(true);
}

void makeApiCall() {
    // Simple GET request
    HttpResponse response = httpClient.get("https://api.example.com/data");
    
    if (response.success) {
        DEBUG_PRINTLN("Response: " + response.body);
    } else {
        LOG_ERROR("Request failed: %s", response.error.c_str());
    }
}
```

### 2. JSON API Integration

```cpp
void fetchWeatherData() {
    JsonDocument response = httpClient.getJson("https://api.openweathermap.org/data/2.5/weather?q=London&appid=YOUR_KEY");
    
    if (!response["error"]) {
        String city = response["name"];
        float temp = response["main"]["temp"];
        DEBUG_PRINTLN("Weather in " + city + ": " + String(temp) + "°C");
    }
}
```

### 3. POST Data to API

```cpp
void sendSensorData() {
    JsonDocument data;
    data["temperature"] = 25.5;
    data["humidity"] = 60;
    data["device_id"] = "esp32_001";
    
    JsonDocument response = httpClient.postJson("https://api.iot-platform.com/sensors", data);
    
    if (!response["error"]) {
        DEBUG_PRINTLN("Data sent successfully!");
    }
}
```

### 4. Add to FreeRTOS Task

```cpp
void apiTask(void* parameter) {
    while (true) {
        if (wifiManager.getStatus() == WL_CONNECTED) {
            fetchWeatherData();
        }
        vTaskDelay(pdMS_TO_TICKS(300000)); // 5 minutes
    }
}

// Create task in setup()
xTaskCreatePinnedToCore(apiTask, "ApiTask", 4096, NULL, 1, NULL, 0);
```

### 5. Authentication

```cpp
// API Key in header
httpClient.addDefaultHeader("X-API-Key", "your_api_key");

// Bearer Token
httpClient.setBearerToken("your_jwt_token");

// Basic Authentication
httpClient.setBasicAuth("username", "password");
```

### 6. Error Handling

```cpp
void robustApiCall() {
    const int maxRetries = 3;
    
    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        HttpResponse response = httpClient.get("https://api.example.com/data");
        
        if (response.success) {
            // Process successful response
            break;
        } else {
            LOG_ERROR("Attempt %d failed: %s", attempt, response.error.c_str());
            if (attempt < maxRetries) {
                delay(5000); // Wait before retry
            }
        }
    }
}
```

### 7. Web API Integration

Add the provided `ApiController` to your web routes:

```cpp
// In your routes registration
void registerRoutes(Router* router) {
    // ... existing routes ...
    
    // Add external API routes
    registerExternalApiRoutes(router);
}
```

Then access via:
- `GET /api/external/weather?city=London&api_key=YOUR_KEY`
- `GET /api/external/time?timezone=UTC`
- `GET /api/external/test` (connectivity test)
- `GET /api/external/stats` (HTTP client statistics)

## Common Use Cases

### Weather Station
See `examples/WeatherStationDemo/` for a complete weather station implementation.

### IoT Data Upload
```cpp
void uploadSensorReading() {
    JsonDocument reading;
    reading["timestamp"] = millis();
    reading["temperature"] = readTemperature();
    reading["humidity"] = readHumidity();
    reading["device_id"] = WiFi.macAddress();
    
    httpClient.postJson("https://your-iot-platform.com/api/readings", reading);
}
```

### Remote Configuration
```cpp
JsonDocument fetchDeviceConfig() {
    String url = "https://config.example.com/device/" + WiFi.macAddress();
    return httpClient.getJson(url);
}
```

### Firmware Update Check
```cpp
bool checkForUpdates() {
    JsonDocument response = httpClient.getJson("https://updates.example.com/esp32/latest");
    String latestVersion = response["version"];
    return latestVersion != CURRENT_VERSION;
}
```

## Tips and Best Practices

1. **Always check WiFi status** before making API calls
2. **Use FreeRTOS tasks** for background API operations
3. **Implement retry logic** for critical API calls
4. **Monitor memory usage** when processing large responses
5. **Use HTTPS** for sensitive data
6. **Cache responses** when appropriate to reduce API calls
7. **Handle rate limits** by implementing delays between requests

## Memory Management

- Enable PSRAM with `-DBOARD_HAS_PSRAM` for large JSON responses
- Use streaming for large file downloads
- Consider chunked processing for very large API responses

## Security

- Always validate API responses before processing
- Use proper SSL certificate verification in production
- Don't log sensitive data like API keys
- Implement request timeouts to prevent hanging

## Troubleshooting

| Issue | Solution |
|-------|----------|
| SSL handshake fails | Add proper CA certificate or disable verification for testing |
| Request timeouts | Increase timeout values in HttpConfig |
| Memory allocation errors | Enable PSRAM, reduce JsonDocument size |
| WiFi disconnects | Add WiFi reconnection logic |
| API rate limits | Implement exponential backoff delays |

For more detailed examples, check the `examples/` directory in the HttpClient library folder.
