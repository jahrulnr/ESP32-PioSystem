/*
 * Weather Station Demo for PioSystem
 * 
 * This example demonstrates a real-world use case of the HttpClient library
 * to create a weather station that fetches data from OpenWeatherMap API
 * and displays it on the TFT screen with automatic updates.
 * 
 * Features:
 * - Real-time weather data fetching
 * - Multiple city support
 * - Error handling and retry logic
 * - Display with weather icons (text-based)
 * - Background updates via FreeRTOS task
 * - Button navigation between cities
 * 
 * Hardware: ESP32-S3 with TFT display and buttons
 * 
 * Setup:
 * 1. Get free API key from https://openweathermap.org/api
 * 2. Update WEATHER_API_KEY below
 * 3. Update WiFi credentials
 * 4. Upload and enjoy!
 * 
 * Author: PioSystem
 * Date: 2025
 */

#include <Arduino.h>
#include "../../WiFiManager/src/wifi_manager.h"
#include "../../DisplayManager/src/display_manager.h"
#include "../../Button/src/input_manager.h"
#include "../../SerialDebug/src/SerialDebug.h"
#include "../src/httpclient.h"

// Configuration - UPDATE THESE VALUES
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";
const String WEATHER_API_KEY = "your_openweathermap_api_key"; // Get from openweathermap.org

// Cities to cycle through
const String CITIES[] = {"London", "New York", "Tokyo", "Sydney", "Paris"};
const int CITY_COUNT = sizeof(CITIES) / sizeof(CITIES[0]);

// Global instances
WiFiManager wifiManager("WeatherStation", "weather123");
DisplayManager displayManager;
InputManager inputManager;
HttpClientManager httpClient;

// Task handles and synchronization
TaskHandle_t weatherTaskHandle = NULL;
TaskHandle_t inputTaskHandle = NULL;
SemaphoreHandle_t displayMutex;

// Weather data structure
struct WeatherData {
    String city;
    float temperature;
    float feelsLike;
    int humidity;
    int pressure;
    String description;
    String icon;
    float windSpeed;
    int visibility;
    String country;
    unsigned long timestamp;
    bool valid;
    
    WeatherData() : temperature(0), feelsLike(0), humidity(0), pressure(0), 
                   windSpeed(0), visibility(0), timestamp(0), valid(false) {}
};

// Global state
WeatherData currentWeather;
int currentCityIndex = 0;
bool manualRefresh = false;
unsigned long lastSuccessfulUpdate = 0;
const unsigned long UPDATE_INTERVAL = 600000; // 10 minutes
const unsigned long RETRY_INTERVAL = 60000;   // 1 minute on error

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_PRINTLN("=== PioSystem Weather Station ===");
    
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
    
    // Initialize input manager
    if (!inputManager.begin()) {
        LOG_ERROR("Failed to initialize input manager");
        return;
    }
    
    // Show startup screen
    updateDisplay("Weather Station", "Starting up...", "", TFT_CYAN);
    
    // Initialize WiFi
    DEBUG_PRINTLN("Initializing WiFi...");
    if (!wifiManager.begin(WIFI_STA)) {
        LOG_ERROR("Failed to initialize WiFi");
        updateDisplay("Error", "WiFi Init Failed", "", TFT_RED);
        return;
    }
    
    // Connect to WiFi
    DEBUG_PRINTLN("Connecting to WiFi: " + String(WIFI_SSID));
    updateDisplay("Connecting", "WiFi: " + String(WIFI_SSID), "", TFT_YELLOW);
    
    if (!wifiManager.connectToNetwork(WIFI_SSID, WIFI_PASSWORD, 30000)) {
        LOG_ERROR("Failed to connect to WiFi");
        updateDisplay("Error", "WiFi Connection", "Failed", TFT_RED);
        return;
    }
    
    DEBUG_PRINTLN("WiFi connected! IP: " + WiFi.localIP().toString());
    updateDisplay("Connected", "IP: " + WiFi.localIP().toString(), "", TFT_GREEN);
    delay(2000);
    
    // Initialize HTTP client
    DEBUG_PRINTLN("Initializing HTTP client...");
    HttpConfig config;
    config.timeout = 20000;
    config.verifySsl = true;
    config.userAgent = "PioSystem-WeatherStation/1.0";
    
    if (!httpClient.begin(config)) {
        LOG_ERROR("Failed to initialize HTTP client");
        updateDisplay("Error", "HTTP Init Failed", "", TFT_RED);
        return;
    }
    
    httpClient.setDebugEnabled(true);
    DEBUG_PRINTLN("HTTP client initialized");
    
    // Validate API key
    if (WEATHER_API_KEY == "your_openweathermap_api_key") {
        updateDisplay("Error", "Please set", "API Key", TFT_RED);
        LOG_ERROR("Please update WEATHER_API_KEY in the code");
        while(1) delay(1000);
    }
    
    // Create tasks
    xTaskCreatePinnedToCore(
        weatherTask,           // Task function
        "WeatherTask",         // Task name
        6144,                  // Stack size (larger for JSON processing)
        NULL,                  // Parameters
        2,                     // Priority
        &weatherTaskHandle,    // Task handle
        0                      // Core 0 (background)
    );
    
    xTaskCreatePinnedToCore(
        inputTask,             // Task function
        "InputTask",           // Task name
        2048,                  // Stack size
        NULL,                  // Parameters
        3,                     // Priority (higher for responsiveness)
        &inputTaskHandle,      // Task handle
        1                      // Core 1 (UI critical)
    );
    
    updateDisplay("Ready", "Weather Station", "Online", TFT_GREEN);
    DEBUG_PRINTLN("Weather Station ready!");
    
    // Trigger initial weather fetch
    manualRefresh = true;
}

void loop() {
    // Main loop mostly idle - tasks handle everything
    delay(1000);
    
    // Optional: Monitor system health
    static unsigned long lastHealthCheck = 0;
    if (millis() - lastHealthCheck > 60000) { // Every minute
        lastHealthCheck = millis();
        
        // Check task health
        if (eTaskGetState(weatherTaskHandle) == eDeleted) {
            LOG_ERROR("Weather task died! Restarting...");
            // Could restart task here
        }
        
        if (eTaskGetState(inputTaskHandle) == eDeleted) {
            LOG_ERROR("Input task died! Restarting...");
            // Could restart task here
        }
        
        // Memory check
        size_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 50000) { // Less than 50KB free
            LOG_ERROR("Low memory warning: %d bytes free", freeHeap);
        }
    }
}

void weatherTask(void* parameter) {
    DEBUG_PRINTLN("Weather task started on Core " + String(xPortGetCoreID()));
    
    while (true) {
        // Check if update is needed
        unsigned long now = millis();
        bool needUpdate = false;
        
        if (manualRefresh) {
            needUpdate = true;
            manualRefresh = false;
            DEBUG_PRINTLN("Manual refresh requested");
        } else if (!currentWeather.valid) {
            needUpdate = true;
            DEBUG_PRINTLN("No valid weather data, fetching...");
        } else if (now - lastSuccessfulUpdate > UPDATE_INTERVAL) {
            needUpdate = true;
            DEBUG_PRINTLN("Update interval reached, refreshing...");
        }
        
        if (needUpdate) {
            // Check WiFi connection
            if (wifiManager.getStatus() != WL_CONNECTED) {
                LOG_ERROR("WiFi not connected, skipping weather update");
                updateDisplayError("No WiFi Connection");
                vTaskDelay(pdMS_TO_TICKS(RETRY_INTERVAL));
                continue;
            }
            
            // Fetch weather data
            if (fetchWeatherData(CITIES[currentCityIndex])) {
                lastSuccessfulUpdate = millis();
                displayWeatherData();
            } else {
                updateDisplayError("Weather Update Failed");
                // Retry sooner on failure
                vTaskDelay(pdMS_TO_TICKS(RETRY_INTERVAL));
                continue;
            }
        }
        
        // Sleep for a short time before checking again
        vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 5 seconds
    }
}

void inputTask(void* parameter) {
    DEBUG_PRINTLN("Input task started on Core " + String(xPortGetCoreID()));
    
    while (true) {
        inputManager.update();
        
        // Handle button presses
        if (inputManager.isPressed(BUTTON_SELECT)) {
            DEBUG_PRINTLN("Select button pressed - refreshing weather");
            manualRefresh = true;
        }
        
        if (inputManager.isPressed(BUTTON_UP)) {
            DEBUG_PRINTLN("Up button pressed - next city");
            currentCityIndex = (currentCityIndex + 1) % CITY_COUNT;
            manualRefresh = true;
        }
        
        if (inputManager.isPressed(BUTTON_DOWN)) {
            DEBUG_PRINTLN("Down button pressed - previous city");
            currentCityIndex = (currentCityIndex - 1 + CITY_COUNT) % CITY_COUNT;
            manualRefresh = true;
        }
        
        if (inputManager.isLongPressed(BUTTON_BACK)) {
            DEBUG_PRINTLN("Back button long pressed - entering sleep mode");
            displayManager.forceSleep();
        }
        
        vTaskDelay(pdMS_TO_TICKS(50)); // 50ms input polling
    }
}

bool fetchWeatherData(const String& city) {
    DEBUG_PRINTLN("Fetching weather data for: " + city);
    
    updateDisplay("Updating", "Fetching weather", "for " + city, TFT_YELLOW);
    
    String url = "https://api.openweathermap.org/data/2.5/weather";
    url += "?q=" + city;
    url += "&appid=" + WEATHER_API_KEY;
    url += "&units=metric";
    
    JsonDocument response = httpClient.getJson(url);
    
    if (response["error"]) {
        String error = response["error"];
        LOG_ERROR("Weather API error: %s", error.c_str());
        return false;
    }
    
    // Check API response status
    if (response["cod"] != 200) {
        String message = response["message"];
        LOG_ERROR("Weather API returned error: %s", message.c_str());
        return false;
    }
    
    // Parse weather data
    currentWeather.city = response["name"];
    currentWeather.country = response["sys"]["country"];
    currentWeather.temperature = response["main"]["temp"];
    currentWeather.feelsLike = response["main"]["feels_like"];
    currentWeather.humidity = response["main"]["humidity"];
    currentWeather.pressure = response["main"]["pressure"];
    currentWeather.description = response["weather"][0]["description"];
    currentWeather.icon = response["weather"][0]["icon"];
    currentWeather.windSpeed = response["wind"]["speed"];
    currentWeather.visibility = response["visibility"];
    currentWeather.timestamp = millis();
    currentWeather.valid = true;
    
    DEBUG_PRINTLN("Weather data updated successfully for " + currentWeather.city);
    DEBUG_PRINTLN("Temperature: " + String(currentWeather.temperature) + "°C");
    DEBUG_PRINTLN("Description: " + currentWeather.description);
    
    return true;
}

void displayWeatherData() {
    if (!currentWeather.valid) {
        updateDisplayError("No Weather Data");
        return;
    }
    
    String title = currentWeather.city + ", " + currentWeather.country;
    String temp = String(currentWeather.temperature, 1) + "°C";
    String details = currentWeather.description;
    details += " | " + String(currentWeather.humidity) + "%";
    
    // Get weather icon (simplified text representation)
    String icon = getWeatherIcon(currentWeather.icon);
    
    // Color based on temperature
    uint16_t tempColor = TFT_WHITE;
    if (currentWeather.temperature > 30) tempColor = TFT_RED;
    else if (currentWeather.temperature > 20) tempColor = TFT_ORANGE;
    else if (currentWeather.temperature > 10) tempColor = TFT_YELLOW;
    else if (currentWeather.temperature > 0) tempColor = TFT_CYAN;
    else tempColor = TFT_BLUE;
    
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        displayManager.clearScreen();
        
        // Title
        displayManager.drawCenteredText(title, 20, TFT_WHITE);
        
        // Weather icon
        displayManager.drawCenteredText(icon, 45, TFT_YELLOW);
        
        // Temperature
        displayManager.drawCenteredText(temp, 70, tempColor);
        
        // Description
        displayManager.drawCenteredText(details, 95, TFT_LIGHTGREY);
        
        // Additional info
        String windInfo = "Wind: " + String(currentWeather.windSpeed, 1) + " m/s";
        displayManager.drawCenteredText(windInfo, 115, TFT_LIGHTGREY);
        
        // Navigation hint
        displayManager.drawCenteredText("↑↓ Cities | ⚪ Refresh", 140, TFT_DARKGREY);
        
        xSemaphoreGive(displayMutex);
    }
}

void updateDisplay(const String& title, const String& line1, const String& line2, uint16_t color) {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        displayManager.clearScreen();
        displayManager.drawTitle(title);
        displayManager.drawCenteredText(line1, 60, color);
        if (!line2.isEmpty()) {
            displayManager.drawCenteredText(line2, 80, color);
        }
        xSemaphoreGive(displayMutex);
    }
}

void updateDisplayError(const String& error) {
    updateDisplay("Error", error, "Press ⚪ to retry", TFT_RED);
}

String getWeatherIcon(const String& iconCode) {
    // Simplified text-based weather icons
    if (iconCode.startsWith("01")) return "☀️ Clear";
    if (iconCode.startsWith("02")) return "⛅ Partly Cloudy";
    if (iconCode.startsWith("03")) return "☁️ Cloudy";
    if (iconCode.startsWith("04")) return "☁️ Overcast";
    if (iconCode.startsWith("09")) return "🌧️ Shower";
    if (iconCode.startsWith("10")) return "🌦️ Rain";
    if (iconCode.startsWith("11")) return "⛈️ Thunder";
    if (iconCode.startsWith("13")) return "❄️ Snow";
    if (iconCode.startsWith("50")) return "🌫️ Mist";
    return "🌤️ Weather";
}
