/*
 * Basic API Request Example for PioSystem
 * 
 * This example demonstrates how to make HTTP/HTTPS requests to third-party APIs
 * using the HttpClient library. It includes:
 * - GET and POST requests
 * - JSON handling
 * - Error handling
 * - Integration with PioSystem architecture
 * 
 * Hardware: ESP32-S3 with WiFi capability
 * 
 * Author: PioSystem
 * Date: 2025
 */

#include <Arduino.h>
#include "../../WiFiManager/src/wifi_manager.h"
#include "../../DisplayManager/src/display_manager.h"
#include "../../SerialDebug/src/SerialDebug.h"
#include "../src/httpclient.h"

// Global instances (as per PioSystem architecture)
WiFiManager wifiManager("PioSystem-Demo", "password123");
DisplayManager displayManager;
HttpClientManager httpClient;

// Configuration
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";

// API endpoints for testing
const String TEST_API_URL = "https://jsonplaceholder.typicode.com/posts/1";
const String WEATHER_API_URL = "https://api.openweathermap.org/data/2.5/weather";
const String WEATHER_API_KEY = "your_openweathermap_api_key"; // Get from openweathermap.org

// FreeRTOS task handles
TaskHandle_t apiTaskHandle = NULL;
SemaphoreHandle_t displayMutex;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_PRINTLN("=== PioSystem HTTP Client Demo ===");
    
    // Initialize display
    if (!displayManager.begin()) {
        LOG_ERROR("Failed to initialize display");
        return;
    }
    
    // Create display mutex
    displayMutex = xSemaphoreCreateMutex();
    if (displayMutex == NULL) {
        LOG_ERROR("Failed to create display mutex");
        return;
    }
    
    // Initialize WiFi
    DEBUG_PRINTLN("Initializing WiFi...");
    if (!wifiManager.begin(WIFI_STA)) {
        LOG_ERROR("Failed to initialize WiFi");
        return;
    }
    
    // Connect to WiFi
    DEBUG_PRINTLN("Connecting to WiFi: " + String(WIFI_SSID));
    updateDisplay("Connecting WiFi...", TFT_YELLOW);
    
    if (!wifiManager.connectToNetwork(WIFI_SSID, WIFI_PASSWORD, 30000)) {
        LOG_ERROR("Failed to connect to WiFi");
        updateDisplay("WiFi Failed!", TFT_RED);
        return;
    }
    
    DEBUG_PRINTLN("WiFi connected! IP: " + WiFi.localIP().toString());
    updateDisplay("WiFi Connected!", TFT_GREEN);
    delay(2000);
    
    // Initialize HTTP client
    DEBUG_PRINTLN("Initializing HTTP client...");
    HttpConfig config;
    config.timeout = 30000;
    config.verifySsl = true;
    config.userAgent = "PioSystem-Demo/1.0";
    
    if (!httpClient.begin(config)) {
        LOG_ERROR("Failed to initialize HTTP client");
        updateDisplay("HTTP Init Failed!", TFT_RED);
        return;
    }
    
    httpClient.setDebugEnabled(true);
    DEBUG_PRINTLN("HTTP client initialized");
    
    // Create API task on Core 0 (background tasks)
    xTaskCreatePinnedToCore(
        apiTask,           // Task function
        "ApiTask",         // Task name  
        4096,              // Stack size
        NULL,              // Parameters
        1,                 // Priority
        &apiTaskHandle,    // Task handle
        0                  // Core 0
    );
    
    updateDisplay("Demo Ready!", TFT_GREEN);
    DEBUG_PRINTLN("Setup complete - Demo ready!");
}

void loop() {
    // Main loop can handle other tasks or go to sleep
    delay(1000);
    
    // Optional: Monitor memory usage
    static unsigned long lastMemCheck = 0;
    if (millis() - lastMemCheck > 30000) { // Every 30 seconds
        lastMemCheck = millis();
        
        size_t freeHeap = ESP.getFreeHeap();
        size_t freePsram = ESP.getFreePsram();
        
        DEBUG_PRINTLN("=== Memory Status ===");
        DEBUG_PRINTLN("Free Heap: " + String(freeHeap) + " bytes");
        DEBUG_PRINTLN("Free PSRAM: " + String(freePsram) + " bytes");
        
        // Display memory on screen briefly
        updateDisplay("Heap: " + String(freeHeap/1024) + "KB", TFT_LIGHTGREY);
    }
}

void apiTask(void* parameter) {
    DEBUG_PRINTLN("API Task started on Core " + String(xPortGetCoreID()));
    
    while (true) {
        // Check WiFi connection
        if (wifiManager.getStatus() != WL_CONNECTED) {
            LOG_ERROR("WiFi not connected, skipping API requests");
            updateDisplay("WiFi Disconnected", TFT_RED);
            vTaskDelay(pdMS_TO_TICKS(10000)); // Wait 10 seconds
            continue;
        }
        
        // Demo sequence
        updateDisplay("Running API Demo...", TFT_CYAN);
        
        // 1. Test basic connectivity
        testConnectivity();
        vTaskDelay(pdMS_TO_TICKS(3000));
        
        // 2. Make a simple GET request
        simpleGetRequest();
        vTaskDelay(pdMS_TO_TICKS(3000));
        
        // 3. Test JSON API
        jsonApiRequest();
        vTaskDelay(pdMS_TO_TICKS(3000));
        
        // 4. Test POST request
        postDataExample();
        vTaskDelay(pdMS_TO_TICKS(3000));
        
        // 5. Weather API example (if API key is provided)
        if (WEATHER_API_KEY != "your_openweathermap_api_key") {
            weatherApiExample();
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
        
        // Show statistics
        showStatistics();
        
        updateDisplay("Demo Complete", TFT_GREEN);
        DEBUG_PRINTLN("=== Demo cycle complete ===");
        
        // Wait 5 minutes before next cycle
        vTaskDelay(pdMS_TO_TICKS(300000));
    }
}

void testConnectivity() {
    DEBUG_PRINTLN("Testing connectivity...");
    updateDisplay("Testing Connection", TFT_YELLOW);
    
    HttpResponse response = httpClient.get("https://httpbin.org/get");
    
    if (response.success) {
        DEBUG_PRINTLN("✓ Connectivity test passed");
        DEBUG_PRINTLN("Response time: " + String(response.responseTime) + "ms");
        
        // Parse IP address from response
        JsonDocument data;
        DeserializationError error = deserializeJson(data, response.body);
        if (!error) {
            String ip = data["origin"];
            updateDisplay("IP: " + ip, TFT_GREEN);
            DEBUG_PRINTLN("Your public IP: " + ip);
        }
    } else {
        LOG_ERROR("✗ Connectivity test failed: %s", response.error.c_str());
        updateDisplay("Connection Failed", TFT_RED);
    }
}

void simpleGetRequest() {
    DEBUG_PRINTLN("Making simple GET request...");
    updateDisplay("GET Request", TFT_YELLOW);
    
    HttpResponse response = httpClient.get(TEST_API_URL);
    
    if (response.success) {
        DEBUG_PRINTLN("✓ GET request successful");
        DEBUG_PRINTLN("Status: " + String(response.statusCode));
        DEBUG_PRINTLN("Response time: " + String(response.responseTime) + "ms");
        DEBUG_PRINTLN("Body length: " + String(response.body.length()) + " bytes");
        
        // Parse and display title
        JsonDocument data;
        DeserializationError error = deserializeJson(data, response.body);
        if (!error) {
            String title = data["title"];
            updateDisplay("Title: " + title.substring(0, 15) + "...", TFT_GREEN);
            DEBUG_PRINTLN("Post title: " + title);
        }
    } else {
        LOG_ERROR("✗ GET request failed: %s", response.error.c_str());
        updateDisplay("GET Failed", TFT_RED);
    }
}

void jsonApiRequest() {
    DEBUG_PRINTLN("Testing JSON API request...");
    updateDisplay("JSON API", TFT_YELLOW);
    
    JsonDocument response = httpClient.getJson(TEST_API_URL);
    
    if (!response["error"]) {
        DEBUG_PRINTLN("✓ JSON API request successful");
        
        int userId = response["userId"];
        int id = response["id"];
        String title = response["title"];
        String body = response["body"];
        
        DEBUG_PRINTLN("User ID: " + String(userId));
        DEBUG_PRINTLN("Post ID: " + String(id));
        DEBUG_PRINTLN("Title: " + title);
        
        updateDisplay("Post ID: " + String(id), TFT_GREEN);
    } else {
        String error = response["error"];
        LOG_ERROR("✗ JSON API request failed: %s", error.c_str());
        updateDisplay("JSON Failed", TFT_RED);
    }
}

void postDataExample() {
    DEBUG_PRINTLN("Testing POST request...");
    updateDisplay("POST Request", TFT_YELLOW);
    
    // Create sample data to post
    JsonDocument postData;
    postData["title"] = "PioSystem Test Post";
    postData["body"] = "This is a test post from ESP32-S3";
    postData["userId"] = 1;
    
    JsonDocument response = httpClient.postJson("https://jsonplaceholder.typicode.com/posts", postData);
    
    if (!response["error"]) {
        DEBUG_PRINTLN("✓ POST request successful");
        
        int id = response["id"];
        String title = response["title"];
        
        DEBUG_PRINTLN("Created post ID: " + String(id));
        DEBUG_PRINTLN("Title: " + title);
        
        updateDisplay("Posted ID: " + String(id), TFT_GREEN);
    } else {
        String error = response["error"];
        LOG_ERROR("✗ POST request failed: %s", error.c_str());
        updateDisplay("POST Failed", TFT_RED);
    }
}

void weatherApiExample() {
    DEBUG_PRINTLN("Testing Weather API...");
    updateDisplay("Weather API", TFT_YELLOW);
    
    String url = WEATHER_API_URL + "?q=London&appid=" + WEATHER_API_KEY + "&units=metric";
    
    JsonDocument weather = httpClient.getJson(url);
    
    if (!weather["error"]) {
        DEBUG_PRINTLN("✓ Weather API request successful");
        
        String cityName = weather["name"];
        float temperature = weather["main"]["temp"];
        String description = weather["weather"][0]["description"];
        int humidity = weather["main"]["humidity"];
        
        DEBUG_PRINTLN("City: " + cityName);
        DEBUG_PRINTLN("Temperature: " + String(temperature) + "°C");
        DEBUG_PRINTLN("Description: " + description);
        DEBUG_PRINTLN("Humidity: " + String(humidity) + "%");
        
        updateDisplay(cityName + ": " + String(temperature) + "°C", TFT_GREEN);
    } else {
        String error = weather["error"];
        LOG_ERROR("✗ Weather API request failed: %s", error.c_str());
        updateDisplay("Weather Failed", TFT_RED);
    }
}

void showStatistics() {
    DEBUG_PRINTLN("=== HTTP Client Statistics ===");
    
    auto stats = httpClient.getStats();
    
    DEBUG_PRINTLN("Total requests: " + String(stats["requests_total"]));
    DEBUG_PRINTLN("Successful requests: " + String(stats["requests_success"]));
    DEBUG_PRINTLN("Failed requests: " + String(stats["requests_failed"]));
    DEBUG_PRINTLN("Bytes sent: " + String(stats["bytes_sent"]));
    DEBUG_PRINTLN("Bytes received: " + String(stats["bytes_received"]));
    
    // Calculate success rate
    float successRate = 0;
    if (stats["requests_total"] > 0) {
        successRate = ((float)stats["requests_success"] / stats["requests_total"]) * 100;
    }
    
    updateDisplay("Success: " + String(successRate, 1) + "%", TFT_CYAN);
    DEBUG_PRINTLN("Success rate: " + String(successRate, 1) + "%");
}

void updateDisplay(const String& message, uint16_t color) {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        displayManager.clearScreen();
        displayManager.drawTitle("HTTP Demo");
        displayManager.drawCenteredText(message, 60, color);
        
        // Show WiFi status
        String wifiStatus = (wifiManager.getStatus() == WL_CONNECTED) ? "WiFi: OK" : "WiFi: ERROR";
        displayManager.drawCenteredText(wifiStatus, 90, TFT_LIGHTGREY);
        
        xSemaphoreGive(displayMutex);
    }
}
