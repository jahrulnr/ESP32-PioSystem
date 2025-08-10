/*
 * HttpClient Integration Example for PioSystem
 * 
 * This example shows how to properly integrate the HttpClient library
 * into your main PioSystem application with proper initialization,
 * task management, and web API integration.
 * 
 * Author: PioSystem
 * Date: 2025
 */

#include <Arduino.h>

// PioSystem Core Libraries
#include "../lib/WiFiManager/src/wifi_manager.h"
#include "../lib/DisplayManager/src/display_manager.h"
#include "../lib/Button/src/input_manager.h"
#include "../lib/SerialDebug/src/SerialDebug.h"

// HTTP Client Library
#include "../lib/HttpClient/src/httpclient.h"

// MVC Framework
#include "../lib/MVCFramework/src/MVCFramework.h"
#include "Controllers/ApiController.h"
#include "Routes/routes.h"

// Global instances (following PioSystem architecture)
WiFiManager wifiManager("PioSystem-API", "password123");
DisplayManager displayManager;
InputManager inputManager;
HttpClientManager httpClientManager; // Our new HTTP client
Application* app;

// Task handles
TaskHandle_t apiBackgroundTaskHandle = NULL;
TaskHandle_t memoryMonitorTaskHandle = NULL;
TaskHandle_t inputTaskHandle = NULL;
TaskHandle_t autosleepTaskHandle = NULL;

// Synchronization
SemaphoreHandle_t displayMutex;

// Configuration
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";
const String WEATHER_API_KEY = "your_openweathermap_api_key";

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_PRINTLN("=== PioSystem with HttpClient Integration ===");
    
    // Initialize core components
    initializeComponents();
    
    // Initialize HTTP client
    initializeHttpClient();
    
    // Initialize MVC application
    initializeWebServer();
    
    // Create FreeRTOS tasks
    createTasks();
    
    DEBUG_PRINTLN("System initialization complete!");
}

void loop() {
    // Main loop handles Arduino compatibility
    // All real work is done in FreeRTOS tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Optional: Handle any Arduino-specific libraries that need loop()
}

void initializeComponents() {
    DEBUG_PRINTLN("Initializing core components...");
    
    // Initialize display
    if (!displayManager.begin()) {
        LOG_ERROR("Failed to initialize display");
        while(1) delay(1000);
    }
    
    // Create display mutex
    displayMutex = xSemaphoreCreateMutex();
    if (displayMutex == NULL) {
        LOG_ERROR("Failed to create display mutex");
        while(1) delay(1000);
    }
    
    // Initialize input manager
    if (!inputManager.begin()) {
        LOG_ERROR("Failed to initialize input manager");
        while(1) delay(1000);
    }
    
    // Initialize WiFi
    DEBUG_PRINTLN("Initializing WiFi...");
    if (!wifiManager.begin(WIFI_AP_STA)) { // Use AP_STA mode for flexibility
        LOG_ERROR("Failed to initialize WiFi");
        while(1) delay(1000);
    }
    
    // Start in AP mode first
    if (!wifiManager.startAccessPoint()) {
        LOG_ERROR("Failed to start access point");
        while(1) delay(1000);
    }
    
    // Try to connect to configured WiFi
    if (strlen(WIFI_SSID) > 0) {
        DEBUG_PRINTLN("Attempting to connect to: " + String(WIFI_SSID));
        wifiManager.connectToNetwork(WIFI_SSID, WIFI_PASSWORD, 30000);
    }
    
    DEBUG_PRINTLN("Core components initialized");
}

void initializeHttpClient() {
    DEBUG_PRINTLN("Initializing HTTP client...");
    
    // Configure HTTP client
    HttpConfig config;
    config.timeout = 30000;           // 30 second timeout
    config.connectTimeout = 10000;    // 10 second connection timeout
    config.verifySsl = true;          // Verify SSL certificates
    config.followRedirects = true;    // Follow redirects
    config.userAgent = "PioSystem/1.0 ESP32-S3";
    
    // Set default headers
    config.defaultHeaders["Accept"] = "application/json";
    config.defaultHeaders["Cache-Control"] = "no-cache";
    
    if (!httpClientManager.begin(config)) {
        LOG_ERROR("Failed to initialize HTTP client");
        while(1) delay(1000);
    }
    
    // Enable debug output
    httpClientManager.setDebugEnabled(true);
    
    // Set up SSL certificate if needed
    // httpClientManager.setCACertificate(your_ca_cert);
    
    DEBUG_PRINTLN("HTTP client initialized successfully");
}

void initializeWebServer() {
    DEBUG_PRINTLN("Initializing web server and MVC framework...");
    
    // Get application instance
    app = Application::getInstance();
    
    // Initialize the application
    if (!app->boot()) {
        LOG_ERROR("Failed to boot MVC application");
        while(1) delay(1000);
    }
    
    // Register all routes including our new API routes
    registerRoutes(app->getRouter());
    registerApiRoutes(app->getRouter()); // Our new external API routes
    
    // Start web server
    if (!app->start()) {
        LOG_ERROR("Failed to start web server");
        while(1) delay(1000);
    }
    
    DEBUG_PRINTLN("Web server started successfully");
    
    // Display server info
    String apIP = WiFi.softAPIP().toString();
    String staIP = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "Not connected";
    
    DEBUG_PRINTLN("Access Points:");
    DEBUG_PRINTLN("  AP Mode: http://" + apIP);
    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("  Station Mode: http://" + staIP);
    }
}

void createTasks() {
    DEBUG_PRINTLN("Creating FreeRTOS tasks...");
    
    // Memory monitor task (Core 0)
    xTaskCreatePinnedToCore(
        memoryMonitorTask,
        "MemoryMonitor",
        2048,
        NULL,
        1,
        &memoryMonitorTaskHandle,
        0
    );
    
    // Input handling task (Core 1 - UI critical)
    xTaskCreatePinnedToCore(
        inputTask,
        "InputTask",
        2048,
        NULL,
        3,
        &inputTaskHandle,
        1
    );
    
    // API background task (Core 0)
    xTaskCreatePinnedToCore(
        apiBackgroundTask,
        "ApiBackgroundTask",
        4096,
        NULL,
        2,
        &apiBackgroundTaskHandle,
        0
    );
    
    // Autosleep task (Core 0)
    xTaskCreatePinnedToCore(
        autosleepTask,
        "AutosleepTask",
        2048,
        NULL,
        1,
        &autosleepTaskHandle,
        0
    );
    
    DEBUG_PRINTLN("All tasks created successfully");
}

void memoryMonitorTask(void* parameter) {
    DEBUG_PRINTLN("Memory monitor task started on Core " + String(xPortGetCoreID()));
    
    while (true) {
        size_t freeHeap = ESP.getFreeHeap();
        size_t freePsram = ESP.getFreePsram();
        
        // Log every 30 seconds
        DEBUG_PRINTLN("Memory Status - Heap: " + String(freeHeap) + " bytes, PSRAM: " + String(freePsram) + " bytes");
        
        // Alert if memory is low
        if (freeHeap < 50000) { // Less than 50KB
            LOG_ERROR("Low heap memory warning: %d bytes", freeHeap);
        }
        
        vTaskDelay(pdMS_TO_TICKS(30000)); // Check every 30 seconds
    }
}

void inputTask(void* parameter) {
    DEBUG_PRINTLN("Input task started on Core " + String(xPortGetCoreID()));
    
    while (true) {
        inputManager.update();
        
        // Handle button presses
        if (inputManager.isPressed(BUTTON_SELECT)) {
            DEBUG_PRINTLN("Select pressed - triggering API test");
            // Could trigger manual API refresh or test
        }
        
        if (inputManager.isPressed(BUTTON_UP)) {
            DEBUG_PRINTLN("Up pressed");
            // Could cycle through display modes
        }
        
        if (inputManager.isPressed(BUTTON_DOWN)) {
            DEBUG_PRINTLN("Down pressed");
            // Could cycle through display modes
        }
        
        if (inputManager.isLongPressed(BUTTON_BACK)) {
            DEBUG_PRINTLN("Long press back - entering sleep mode");
            displayManager.forceSleep();
        }
        
        vTaskDelay(pdMS_TO_TICKS(50)); // 50ms polling
    }
}

void apiBackgroundTask(void* parameter) {
    DEBUG_PRINTLN("API background task started on Core " + String(xPortGetCoreID()));
    
    unsigned long lastWeatherUpdate = 0;
    unsigned long lastConnectivityCheck = 0;
    const unsigned long weatherInterval = 600000;      // 10 minutes
    const unsigned long connectivityInterval = 300000; // 5 minutes
    
    while (true) {
        unsigned long now = millis();
        
        // Only make API calls if WiFi is connected
        if (wifiManager.getStatus() == WL_CONNECTED) {
            
            // Periodic weather update
            if (now - lastWeatherUpdate > weatherInterval) {
                updateWeatherDisplay();
                lastWeatherUpdate = now;
            }
            
            // Periodic connectivity check
            if (now - lastConnectivityCheck > connectivityInterval) {
                performConnectivityCheck();
                lastConnectivityCheck = now;
            }
            
        } else {
            // No WiFi connection
            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                displayManager.clearScreen();
                displayManager.drawTitle("System Status");
                displayManager.drawCenteredText("WiFi: Disconnected", 60, TFT_RED);
                displayManager.drawCenteredText("AP: " + WiFi.softAPIP().toString(), 80, TFT_YELLOW);
                xSemaphoreGive(displayMutex);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000)); // Check every 10 seconds
    }
}

void autosleepTask(void* parameter) {
    DEBUG_PRINTLN("Autosleep task started on Core " + String(xPortGetCoreID()));
    
    unsigned long lastActivity = millis();
    const unsigned long sleepTimeout = 60000; // 1 minute
    
    while (true) {
        // Check for user activity
        if (inputManager.hasActivity()) {
            lastActivity = millis();
            if (displayManager.isAsleep()) {
                displayManager.wake();
                DEBUG_PRINTLN("Display woken by user activity");
            }
        }
        
        // Check for sleep timeout
        if (!displayManager.isAsleep() && (millis() - lastActivity > sleepTimeout)) {
            DEBUG_PRINTLN("Sleep timeout reached - putting display to sleep");
            displayManager.forceSleep();
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
    }
}

void updateWeatherDisplay() {
    if (WEATHER_API_KEY == "your_openweathermap_api_key") {
        DEBUG_PRINTLN("Weather API key not configured, skipping weather update");
        return;
    }
    
    DEBUG_PRINTLN("Updating weather display...");
    
    String url = "https://api.openweathermap.org/data/2.5/weather";
    url += "?q=London&appid=" + WEATHER_API_KEY + "&units=metric";
    
    JsonDocument weather = httpClientManager.getJson(url);
    
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        displayManager.clearScreen();
        displayManager.drawTitle("Weather");
        
        if (!weather["error"] && weather["cod"] == 200) {
            String city = weather["name"];
            float temp = weather["main"]["temp"];
            String desc = weather["weather"][0]["description"];
            
            displayManager.drawCenteredText(city, 40, TFT_WHITE);
            displayManager.drawCenteredText(String(temp, 1) + "°C", 65, TFT_YELLOW);
            displayManager.drawCenteredText(desc, 90, TFT_LIGHTGREY);
            
            DEBUG_PRINTLN("Weather updated: " + city + ", " + String(temp) + "°C");
        } else {
            displayManager.drawCenteredText("Weather Update", 50, TFT_RED);
            displayManager.drawCenteredText("Failed", 75, TFT_RED);
            
            String error = weather["error"];
            LOG_ERROR("Weather update failed: %s", error.c_str());
        }
        
        xSemaphoreGive(displayMutex);
    }
}

void performConnectivityCheck() {
    DEBUG_PRINTLN("Performing connectivity check...");
    
    HttpResponse test = httpClientManager.get("https://httpbin.org/get");
    
    if (test.success) {
        DEBUG_PRINTLN("✓ Connectivity check passed - Response time: " + String(test.responseTime) + "ms");
    } else {
        LOG_ERROR("✗ Connectivity check failed: %s", test.error.c_str());
    }
    
    // Log statistics
    auto stats = httpClientManager.getStats();
    DEBUG_PRINTLN("HTTP Stats - Total: " + String(stats["requests_total"]) + 
                 ", Success: " + String(stats["requests_success"]) + 
                 ", Failed: " + String(stats["requests_failed"]));
}
