# HttpClient Library for ESP32-S3

A comprehensive HTTP/HTTPS client library with SSL support for ESP32-S3 IoT applications. This library provides easy-to-use methods for making HTTP requests to third-party APIs with advanced features like SSL certificate verification, authentication, and JSON handling.

## Features

- **HTTP and HTTPS support** with SSL certificate verification
- **Custom CA certificate support** for secure connections
- **Mutual TLS authentication** with client certificates
- **Multiple authentication methods** (Basic Auth, Bearer Token)
- **JSON request/response handling** with ArduinoJson integration
- **File upload/download capabilities**
- **Configurable timeouts and redirects**
- **Connection statistics and debugging**
- **Thread-safe operations** compatible with FreeRTOS tasks

## Installation

This library is already integrated into your PioSystem project. It's located in `lib/HttpClient/`.

## Quick Start

```cpp
#include "httpclient.h"

// Create HTTP client instance
HttpClientManager httpClient;

void setup() {
    Serial.begin(115200);
    
    // Initialize WiFi (using your WiFiManager)
    // wifiManager.connectToNetwork("your_ssid", "your_password");
    
    // Initialize HTTP client
    HttpConfig config;
    config.timeout = 30000;  // 30 seconds
    config.verifySsl = true; // Verify SSL certificates
    httpClient.begin(config);
    
    // Enable debug output
    httpClient.setDebugEnabled(true);
}

void loop() {
    // Make a simple GET request
    HttpResponse response = httpClient.get("https://api.example.com/data");
    
    if (response.success) {
        Serial.println("Response: " + response.body);
    } else {
        Serial.println("Error: " + response.error);
    }
    
    delay(60000); // Wait 1 minute
}
```

## API Reference

### Basic HTTP Methods

```cpp
// GET request
HttpResponse response = httpClient.get("https://api.example.com/users");

// POST request with JSON body
HttpResponse response = httpClient.post(
    "https://api.example.com/users", 
    "{\"name\":\"John\",\"email\":\"john@example.com\"}", 
    "application/json"
);

// PUT request
HttpResponse response = httpClient.put(
    "https://api.example.com/users/123", 
    "{\"name\":\"Jane\"}", 
    "application/json"
);

// DELETE request
HttpResponse response = httpClient.del("https://api.example.com/users/123");
```

### JSON Convenience Methods

```cpp
// GET and parse JSON response
JsonDocument jsonResponse = httpClient.getJson("https://api.example.com/data");
if (!jsonResponse["error"]) {
    String name = jsonResponse["name"];
    int age = jsonResponse["age"];
}

// POST JSON data
JsonDocument requestBody;
requestBody["name"] = "John Doe";
requestBody["age"] = 30;

JsonDocument response = httpClient.postJson("https://api.example.com/users", requestBody);
```

### Authentication

```cpp
// Basic Authentication
httpClient.setBasicAuth("username", "password");

// Bearer Token
httpClient.setBearerToken("your_jwt_token_here");

// Custom headers
std::map<String, String> headers;
headers["X-API-Key"] = "your_api_key";
headers["Custom-Header"] = "value";
httpClient.setDefaultHeaders(headers);
```

### SSL Configuration

```cpp
// Use custom CA certificate
const char* caCert = R"(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
... certificate content ...
-----END CERTIFICATE-----
)";

httpClient.setCACertificate(caCert);

// Disable SSL verification (not recommended for production)
HttpConfig config;
config.verifySsl = false;
httpClient.begin(config);
```

### File Operations

```cpp
// Download file
bool success = httpClient.downloadFile(
    "https://example.com/file.zip", 
    "/downloaded_file.zip",
    [](size_t current, size_t total) {
        Serial.printf("Download progress: %d/%d bytes\n", current, total);
    }
);

// Upload file
HttpResponse response = httpClient.uploadFile(
    "https://api.example.com/upload", 
    "/local_file.txt", 
    "file"
);
```

## Configuration Options

```cpp
HttpConfig config;
config.timeout = 30000;           // Request timeout (ms)
config.connectTimeout = 10000;    // Connection timeout (ms)
config.followRedirects = true;    // Follow HTTP redirects
config.maxRedirects = 5;          // Maximum redirects to follow
config.verifySsl = true;          // Verify SSL certificates
config.userAgent = "MyApp/1.0";   // Custom user agent

httpClient.begin(config);
```

## Error Handling

```cpp
HttpResponse response = httpClient.get("https://api.example.com/data");

if (response.success) {
    // Request successful (status 200-299)
    Serial.println("Success: " + response.body);
    Serial.println("Status Code: " + String(response.statusCode));
    Serial.println("Response Time: " + String(response.responseTime) + "ms");
} else {
    // Request failed
    Serial.println("Error: " + response.error);
    Serial.println("Status Code: " + String(response.statusCode));
    
    // Get last error details
    String lastError = httpClient.getLastError();
    Serial.println("Last Error: " + lastError);
}
```

## Statistics and Monitoring

```cpp
// Get connection statistics
std::map<String, int> stats = httpClient.getStats();
Serial.println("Total Requests: " + String(stats["requests_total"]));
Serial.println("Successful Requests: " + String(stats["requests_success"]));
Serial.println("Failed Requests: " + String(stats["requests_failed"]));
Serial.println("Bytes Sent: " + String(stats["bytes_sent"]));
Serial.println("Bytes Received: " + String(stats["bytes_received"]));

// Reset statistics
httpClient.resetStats();
```

## Integration with PioSystem

This library is designed to work seamlessly with your PioSystem architecture:

### In FreeRTOS Tasks

```cpp
void apiRequestTask(void* parameter) {
    HttpClientManager* httpClient = (HttpClientManager*)parameter;
    
    while (true) {
        // Make API request
        JsonDocument response = httpClient->getJson("https://api.weather.com/current");
        
        if (!response["error"]) {
            float temperature = response["temperature"];
            
            // Update display with mutex protection
            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                displayManager.drawCenteredText("Temp: " + String(temperature) + "°C", 100, TFT_WHITE);
                xSemaphoreGive(displayMutex);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(300000)); // Wait 5 minutes
    }
}
```

### In Controllers

```cpp
// In your API controller
Response WeatherController::getCurrentWeather(Request& request) {
    JsonDocument weatherData = httpClient.getJson("https://api.openweathermap.org/data/2.5/weather?q=London&appid=YOUR_API_KEY");
    
    JsonDocument response;
    response["status"] = "success";
    response["data"] = weatherData;
    
    return json(request.getServerRequest(), response);
}
```

## Memory Considerations

- Use PSRAM for large JSON responses by enabling `-DBOARD_HAS_PSRAM`
- JsonDocument sizing: Network responses typically need ~2KB, adjust per endpoint
- The library automatically manages HTTP client lifecycle to prevent memory leaks
- Consider implementing request queuing for high-frequency API calls

## Common Use Cases

### Weather API Integration
```cpp
JsonDocument getWeatherData(String city) {
    String url = "https://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + API_KEY;
    return httpClient.getJson(url);
}
```

### IoT Device Registration
```cpp
bool registerDevice(String deviceId, String location) {
    JsonDocument payload;
    payload["device_id"] = deviceId;
    payload["location"] = location;
    payload["capabilities"] = "wifi,display,sensors";
    
    JsonDocument response = httpClient.postJson("https://iot.example.com/api/devices", payload);
    return !response["error"];
}
```

### Firmware Update Check
```cpp
JsonDocument checkForUpdates(String currentVersion) {
    std::map<String, String> headers;
    headers["X-Device-Version"] = currentVersion;
    headers["X-Device-Type"] = "ESP32-S3";
    
    return httpClient.getJson("https://updates.example.com/api/check", headers);
}
```

## Troubleshooting

1. **SSL Certificate Errors**: Use `setCACertificate()` with the appropriate root CA certificate
2. **Timeout Issues**: Increase `timeout` and `connectTimeout` values in configuration
3. **Memory Issues**: Enable PSRAM and monitor heap usage with `memoryMonitorTask`
4. **Connection Failures**: Ensure WiFi is connected before making requests

## Dependencies

- ArduinoJson (^7.4.1)
- WiFiClientSecure (ESP32 core)
- HTTPClient (ESP32 core)
- SPIFFS (for file operations)

This library follows the PioSystem coding standards and integrates with the SerialDebug system for consistent logging across your application.
