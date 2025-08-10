# HttpClient Library Quick Start Guide

This guide will help you get started with making HTTP/HTTPS requests using the HttpClient library in your PioSystem project.

## Prerequisites

1. ESP32-S3 device with WiFi connection
2. PioSystem framework initialized
3. WiFiManager configured and connected

## Basic Usage Examples

### 1. Simple GET Request

```cpp
#include "httpclient.h"

HttpClientManager httpClient;

void setup() {
    // Initialize your WiFi connection first
    wifiManager.connectToNetwork("your_ssid", "your_password");
    
    // Initialize HTTP client
    HttpConfig config;
    config.timeout = 30000;
    httpClient.begin(config);
    httpClient.setDebugEnabled(true);
}

void makeSimpleRequest() {
    HttpResponse response = httpClient.get("https://jsonplaceholder.typicode.com/posts/1");
    
    if (response.success) {
        DEBUG_PRINTLN("Response received:");
        DEBUG_PRINTLN(response.body);
    } else {
        LOG_ERROR("Request failed: %s", response.error.c_str());
    }
}
```

### 2. JSON API Request

```cpp
void fetchWeatherData() {
    // Replace with your actual API key
    String apiKey = "your_weather_api_key";
    String url = "https://api.openweathermap.org/data/2.5/weather?q=London&appid=" + apiKey;
    
    JsonDocument weatherData = httpClient.getJson(url);
    
    if (!weatherData["error"]) {
        String cityName = weatherData["name"];
        float temperature = weatherData["main"]["temp"];
        String description = weatherData["weather"][0]["description"];
        
        DEBUG_PRINTLN("Weather in " + cityName + ":");
        DEBUG_PRINTLN("Temperature: " + String(temperature) + "K");
        DEBUG_PRINTLN("Description: " + description);
        
        // Display on TFT with mutex protection
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            displayManager.clearScreen();
            displayManager.drawTitle("Weather");
            displayManager.drawCenteredText(cityName, 40, TFT_WHITE);
            displayManager.drawCenteredText(String(temperature - 273.15) + "°C", 60, TFT_YELLOW);
            displayManager.drawCenteredText(description, 80, TFT_LIGHTGREY);
            xSemaphoreGive(displayMutex);
        }
    } else {
        LOG_ERROR("Weather API error: %s", weatherData["error"].as<String>().c_str());
    }
}
```

### 3. POST Request with Authentication

```cpp
void sendSensorData() {
    // Set API key authentication
    httpClient.addDefaultHeader("X-API-Key", "your_api_key_here");
    
    // Create JSON payload
    JsonDocument sensorData;
    sensorData["device_id"] = "esp32_device_001";
    sensorData["timestamp"] = millis();
    sensorData["temperature"] = 25.6;
    sensorData["humidity"] = 60.2;
    sensorData["location"] = "Living Room";
    
    JsonDocument response = httpClient.postJson("https://api.iot-platform.com/sensors/data", sensorData);
    
    if (!response["error"]) {
        String dataId = response["id"];
        DEBUG_PRINTLN("Data uploaded successfully. ID: " + dataId);
    } else {
        LOG_ERROR("Upload failed: %s", response["error"].as<String>().c_str());
    }
}
```

## Integration with FreeRTOS Tasks

### Background API Task

```cpp
void apiTask(void* parameter) {
    HttpClientManager* client = new HttpClientManager();
    
    HttpConfig config;
    config.timeout = 20000;
    config.verifySsl = true;
    client->begin(config);
    
    while (true) {
        // Check if WiFi is connected
        if (wifiManager.getStatus() == WL_CONNECTED) {
            // Fetch data from API
            JsonDocument apiData = client->getJson("https://api.example.com/status");
            
            if (!apiData["error"]) {
                // Process the data
                bool systemActive = apiData["active"];
                String message = apiData["message"];
                
                // Update display or process data
                DEBUG_PRINTLN("API Status: " + String(systemActive ? "Active" : "Inactive"));
                DEBUG_PRINTLN("Message: " + message);
            }
        }
        
        // Wait 5 minutes before next request
        vTaskDelay(pdMS_TO_TICKS(300000));
    }
    
    delete client;
    vTaskDelete(NULL);
}

// Create the task in your setup
void createApiTask() {
    xTaskCreatePinnedToCore(
        apiTask,           // Task function
        "ApiTask",         // Task name
        4096,              // Stack size
        NULL,              // Parameters
        1,                 // Priority
        NULL,              // Task handle
        0                  // Core (Core 0 for background tasks)
    );
}
```

## Error Handling Best Practices

```cpp
void robustApiRequest() {
    const int maxRetries = 3;
    int retryCount = 0;
    
    while (retryCount < maxRetries) {
        HttpResponse response = httpClient.get("https://api.example.com/data");
        
        if (response.success) {
            DEBUG_PRINTLN("Request successful on attempt " + String(retryCount + 1));
            // Process response
            break;
        } else {
            retryCount++;
            LOG_ERROR("Request failed (attempt %d/%d): %s", retryCount, maxRetries, response.error.c_str());
            
            if (retryCount < maxRetries) {
                DEBUG_PRINTLN("Retrying in 5 seconds...");
                delay(5000);
            } else {
                LOG_ERROR("Max retries reached. Request failed permanently.");
                // Handle permanent failure
            }
        }
    }
}
```

## Configuration for Different Environments

```cpp
void configureForEnvironment() {
    HttpConfig config;
    
    #ifdef PRODUCTION
        config.verifySsl = true;
        config.timeout = 30000;
        config.userAgent = "PioSystem-Device/1.0";
    #else
        config.verifySsl = false;  // For testing with self-signed certs
        config.timeout = 60000;    // Longer timeout for debugging
        config.userAgent = "PioSystem-Debug/1.0";
    #endif
    
    httpClient.begin(config);
    httpClient.setDebugEnabled(true);
}
```

## Memory Management Tips

```cpp
void memoryEfficientRequests() {
    // For large responses, consider streaming or chunked processing
    HttpResponse response = httpClient.get("https://api.example.com/large-data");
    
    if (response.success) {
        // Process response in chunks if it's large
        size_t chunkSize = 1024;
        for (size_t i = 0; i < response.body.length(); i += chunkSize) {
            String chunk = response.body.substring(i, i + chunkSize);
            // Process chunk
            yield(); // Allow other tasks to run
        }
    }
    
    // Check memory usage
    size_t freeHeap = ESP.getFreeHeap();
    size_t freePsram = ESP.getFreePsram();
    DEBUG_PRINTLN("Free Heap: " + String(freeHeap) + " bytes");
    DEBUG_PRINTLN("Free PSRAM: " + String(freePsram) + " bytes");
}
```

## Testing and Debugging

```cpp
void testApiConnectivity() {
    // Test with a simple, reliable endpoint
    HttpResponse response = httpClient.get("https://httpbin.org/get");
    
    if (response.success) {
        DEBUG_PRINTLN("Internet connectivity: OK");
        DEBUG_PRINTLN("Response time: " + String(response.responseTime) + "ms");
        
        // Parse response to check your IP
        JsonDocument data;
        deserializeJson(data, response.body);
        String origin = data["origin"];
        DEBUG_PRINTLN("Your IP: " + origin);
    } else {
        LOG_ERROR("Connectivity test failed: %s", response.error.c_str());
    }
    
    // Print statistics
    auto stats = httpClient.getStats();
    DEBUG_PRINTLN("Total requests: " + String(stats["requests_total"]));
    DEBUG_PRINTLN("Success rate: " + String(stats["requests_success"]) + "/" + String(stats["requests_total"]));
}
```

## Next Steps

1. Check the `BasicApiRequest` example for a complete working implementation
2. Review the `WeatherStationDemo` for a real-world use case
3. Look at `SecureApiExample` for SSL certificate handling
4. Explore advanced features in the main README.md

## Common Issues and Solutions

| Issue | Solution |
|-------|----------|
| Request timeout | Increase timeout values in `HttpConfig` |
| Memory allocation failed | Enable PSRAM with `-DBOARD_HAS_PSRAM` |
| WiFi not connected | Check WiFi status before making requests |
| JSON parsing error | Validate API response format and increase JsonDocument size |

Remember to always check WiFi connectivity and handle errors gracefully in your production code!
