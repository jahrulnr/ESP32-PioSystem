/*
 * Fixed main.cpp example showing proper HttpClient initialization
 */

#include <Arduino.h>

// PioSystem Core Libraries
#include "../lib/WiFiManager/src/wifi_manager.h"
#include "../lib/DisplayManager/src/display_manager.h"
#include "../lib/SerialDebug/src/SerialDebug.h"

// HTTP Client Library
#include "../lib/HttpClient/src/httpclient.h"

// MVC Framework
#include "../lib/MVCFramework/src/MVCFramework.h"
#include "Routes/routes.h"

// Global instances
WiFiManager wifiManager("PioSystem-Fixed", "password123");
DisplayManager displayManager;
Application* app;

// CRITICAL: Properly declare and initialize the HttpClient
HttpClientManager httpClientInstance;  // Create the actual instance
HttpClientManager* httpClientManager = &httpClientInstance;  // Export pointer for routes

// Configuration
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_PRINTLN("=== PioSystem HTTP Client - Fixed Version ===");
    
    // Initialize display
    if (!displayManager.begin()) {
        LOG_ERROR("Failed to initialize display");
        while(1) delay(1000);
    }
    
    // Initialize WiFi
    DEBUG_PRINTLN("Initializing WiFi...");
    if (!wifiManager.begin(WIFI_AP_STA)) {
        LOG_ERROR("Failed to initialize WiFi");
        while(1) delay(1000);
    }
    
    // Start in AP mode
    if (!wifiManager.startAccessPoint()) {
        LOG_ERROR("Failed to start access point");
        while(1) delay(1000);
    }
    
    // Try to connect to configured WiFi
    if (strlen(WIFI_SSID) > 0) {
        DEBUG_PRINTLN("Attempting to connect to: " + String(WIFI_SSID));
        wifiManager.connectToNetwork(WIFI_SSID, WIFI_PASSWORD, 30000);
    }
    
    // CRITICAL: Initialize HTTP client BEFORE using it
    DEBUG_PRINTLN("Initializing HTTP client...");
    HttpConfig config;
    config.timeout = 30000;
    config.connectTimeout = 10000;
    config.verifySsl = true;
    config.userAgent = "PioSystem-Fixed/1.0";
    
    if (!httpClientManager->begin(config)) {
        LOG_ERROR("Failed to initialize HTTP client");
        while(1) delay(1000);
    }
    
    httpClientManager->setDebugEnabled(true);
    DEBUG_PRINTLN("HTTP client initialized successfully");
    
    // Initialize MVC application
    app = Application::getInstance();
    if (!app->boot()) {
        LOG_ERROR("Failed to boot MVC application");
        while(1) delay(1000);
    }
    
    // Register routes (now httpClientManager is properly initialized)
    registerRoutes(app->getRouter());
    registerExternalApiRoutes(app->getRouter());
    
    // Start web server
    if (!app->start()) {
        LOG_ERROR("Failed to start web server");
        while(1) delay(1000);
    }
    
    DEBUG_PRINTLN("System initialized successfully!");
    
    // Display connection info
    String apIP = WiFi.softAPIP().toString();
    DEBUG_PRINTLN("Access Point: http://" + apIP);
    
    if (WiFi.status() == WL_CONNECTED) {
        String staIP = WiFi.localIP().toString();
        DEBUG_PRINTLN("Station Mode: http://" + staIP);
    }
    
    // Test the HTTP client
    testHttpClient();
}

void loop() {
    // Main loop - all work is done in tasks
    delay(1000);
}

void testHttpClient() {
    DEBUG_PRINTLN("Testing HTTP client initialization...");
    
    // Test basic connectivity
    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("WiFi connected, testing HTTP request...");
        
        HttpResponse response = httpClientManager->get("https://httpbin.org/get");
        
        if (response.success) {
            DEBUG_PRINTLN("✓ HTTP client test successful!");
            DEBUG_PRINTLN("Response time: " + String(response.responseTime) + "ms");
            
            // Show stats
            auto stats = httpClientManager->getStats();
            DEBUG_PRINTLN("Stats - Total: " + String(stats["requests_total"]) + 
                         ", Success: " + String(stats["requests_success"]));
        } else {
            LOG_ERROR("✗ HTTP client test failed: %s", response.error.c_str());
        }
    } else {
        DEBUG_PRINTLN("WiFi not connected, skipping HTTP test");
    }
}
