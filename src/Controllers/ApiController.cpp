#include "ApiController.h"
#include "../../lib/SerialDebug/src/SerialDebug.h"

ApiController::ApiController(HttpClientManager* httpClient) : _httpClient(httpClient) {
    if (_httpClient == nullptr) {
        DEBUG_PRINTLN("ERROR: HttpClientManager is null in ApiController!");
    } else {
        DEBUG_PRINTLN("ApiController initialized");
    }
}

ApiController::~ApiController() {
    // Cleanup if needed
}

Response ApiController::getWeather(Request& request) {
    DEBUG_PRINTLN("Weather API request received");
    
    // Check if HTTP client is valid
    if (_httpClient == nullptr) {
        JsonDocument errorDoc = createErrorResponse("HTTP client not initialized");
        return json(request.getServerRequest(), errorDoc);
    }
    
    // Validate required parameters
    String validationError = validateParams(request, {"city"});
    if (!validationError.isEmpty()) {
        JsonDocument errorDoc = createErrorResponse(validationError);
        return json(request.getServerRequest(), errorDoc);
    }
    
    String city = request.route("city");
    String apiKey = request.route("api_key"); // Optional, could be configured
    
    if (apiKey.isEmpty()) {
        // Use a default API key or return error
        JsonDocument errorDoc = createErrorResponse("API key required");
        return json(request.getServerRequest(), errorDoc);
    }
    
    // Build weather API URL
    String url = "https://api.openweathermap.org/data/2.5/weather";
    url += "?q=" + city;
    url += "&appid=" + apiKey;
    url += "&units=metric";
    
    DEBUG_PRINTLN("Fetching weather for: " + city);
    
    // Make API request
    JsonDocument apiResponse = _httpClient->getJson(url);
    
    JsonDocument response;
    
    if (apiResponse["error"]) {
        // External API error
        String error = apiResponse["error"];
        DEBUG_PRINTLN("Weather API error: " + error);
        
        response["status"] = "error";
        response["message"] = "Weather service unavailable";
        response["details"] = error;
    } else if (apiResponse["cod"] != 200) {
        // API returned error status
        String message = apiResponse["message"];
        DEBUG_PRINTLN("Weather API returned error: " + message);
        
        response["status"] = "error";
        response["message"] = "Weather data not found";
        response["details"] = message;
    } else {
        // Success - extract and format weather data
        JsonDocument weatherData;
        weatherData["city"] = apiResponse["name"];
        weatherData["country"] = apiResponse["sys"]["country"];
        weatherData["temperature"] = apiResponse["main"]["temp"];
        weatherData["feels_like"] = apiResponse["main"]["feels_like"];
        weatherData["humidity"] = apiResponse["main"]["humidity"];
        weatherData["pressure"] = apiResponse["main"]["pressure"];
        weatherData["description"] = apiResponse["weather"][0]["description"];
        weatherData["wind_speed"] = apiResponse["wind"]["speed"];
        weatherData["visibility"] = apiResponse["visibility"];
        weatherData["timestamp"] = millis();
        
        response["status"] = "success";
        response["data"] = weatherData;
        
        DEBUG_PRINTLN("Weather data retrieved successfully for " + String(apiResponse["name"].as<String>()));
    }
    
    return json(request.getServerRequest(), response);
}

Response ApiController::getTime(Request& request) {
    DEBUG_PRINTLN("Time API request received");
    
    // Check if HTTP client is valid
    if (_httpClient == nullptr) {
        JsonDocument errorDoc = createErrorResponse("HTTP client not initialized");
        return json(request.getServerRequest(), errorDoc);
    }
    
    String timezone = request.route("timezone");
    if (timezone.isEmpty()) {
        timezone = "UTC";
    }
    
    // Build time API URL
    String url = "http://worldtimeapi.org/api/timezone/" + timezone;
    
    DEBUG_PRINTLN("Fetching time for timezone: " + timezone);
    
    // Make API request
    JsonDocument apiResponse = _httpClient->getJson(url);
    
    JsonDocument response;
    
    if (apiResponse["error"]) {
        String error = apiResponse["error"];
        DEBUG_PRINTLN("Time API error: " + error);
        
        response["status"] = "error";
        response["message"] = "Time service unavailable";
        response["details"] = error;
    } else {
        // Success - extract time data
        JsonDocument timeData;
        timeData["timezone"] = apiResponse["timezone"];
        timeData["datetime"] = apiResponse["datetime"];
        timeData["utc_offset"] = apiResponse["utc_offset"];
        timeData["day_of_year"] = apiResponse["day_of_year"];
        timeData["week_number"] = apiResponse["week_number"];
        
        response["status"] = "success";
        response["data"] = timeData;
        
        DEBUG_PRINTLN("Time data retrieved successfully for " + timezone);
    }
    
    return json(request.getServerRequest(), response);
}

Response ApiController::testConnectivity(Request& request) {
    DEBUG_PRINTLN("Connectivity test request received");
    
    // Check if HTTP client is valid
    if (_httpClient == nullptr) {
        JsonDocument errorDoc = createErrorResponse("HTTP client not initialized");
        return json(request.getServerRequest(), errorDoc);
    }
    
    JsonDocument response;
    JsonDocument testResults;
    
    // Test 1: Basic HTTP connectivity
    DEBUG_PRINTLN("Testing HTTP connectivity...");
    HttpResponse httpTest = _httpClient->get("http://httpbin.org/get");
    testResults["http"]["success"] = httpTest.success;
    testResults["http"]["status_code"] = httpTest.statusCode;
    testResults["http"]["response_time"] = httpTest.responseTime;
    if (!httpTest.success) {
        testResults["http"]["error"] = httpTest.error;
    }
    
    // Test 2: HTTPS connectivity
    DEBUG_PRINTLN("Testing HTTPS connectivity...");
    HttpResponse httpsTest = _httpClient->get("https://httpbin.org/get");
    testResults["https"]["success"] = httpsTest.success;
    testResults["https"]["status_code"] = httpsTest.statusCode;
    testResults["https"]["response_time"] = httpsTest.responseTime;
    if (!httpsTest.success) {
        testResults["https"]["error"] = httpsTest.error;
    }
    
    // Test 3: JSON API test
    DEBUG_PRINTLN("Testing JSON API...");
    JsonDocument jsonTest = _httpClient->getJson("https://jsonplaceholder.typicode.com/posts/1");
    testResults["json_api"]["success"] = !jsonTest["error"];
    if (jsonTest["error"]) {
        testResults["json_api"]["error"] = jsonTest["error"];
    } else {
        testResults["json_api"]["title"] = jsonTest["title"];
    }
    
    // Overall connectivity status
    bool overallSuccess = httpTest.success && httpsTest.success && !jsonTest["error"];
    
    response["status"] = overallSuccess ? "success" : "partial";
    response["message"] = overallSuccess ? "All connectivity tests passed" : "Some tests failed";
    response["data"] = testResults;
    response["timestamp"] = millis();
    
    DEBUG_PRINTLN("Connectivity test completed - Overall: " + String(overallSuccess ? "PASS" : "FAIL"));
    
    return json(request.getServerRequest(), response);
}

Response ApiController::getStats(Request& request) {
    DEBUG_PRINTLN("HTTP client stats request received");
    
    // Check if HTTP client is valid
    if (_httpClient == nullptr) {
        JsonDocument errorDoc = createErrorResponse("HTTP client not initialized");
        return json(request.getServerRequest(), errorDoc);
    }
    
    auto stats = _httpClient->getStats();
    
    JsonDocument response;
    JsonDocument statsData;
    
    statsData["requests_total"] = stats["requests_total"];
    statsData["requests_success"] = stats["requests_success"];
    statsData["requests_failed"] = stats["requests_failed"];
    statsData["bytes_sent"] = stats["bytes_sent"];
    statsData["bytes_received"] = stats["bytes_received"];
    
    // Calculate success rate
    float successRate = 0;
    if (stats["requests_total"] > 0) {
        successRate = ((float)stats["requests_success"] / stats["requests_total"]) * 100;
    }
    statsData["success_rate_percent"] = successRate;
    
    // Add last error if available
    String lastError = _httpClient->getLastError();
    if (!lastError.isEmpty()) {
        statsData["last_error"] = lastError;
    }
    
    response["status"] = "success";
    response["data"] = statsData;
    response["timestamp"] = millis();
    
    return json(request.getServerRequest(), response);
}

Response ApiController::proxy(Request& request) {
    DEBUG_PRINTLN("Proxy request received");
    
    // Check if HTTP client is valid
    if (_httpClient == nullptr) {
        JsonDocument errorDoc = createErrorResponse("HTTP client not initialized");
        return json(request.getServerRequest(), errorDoc);
    }
    
    // Validate required parameters
    String validationError = validateParams(request, {"url"});
    if (!validationError.isEmpty()) {
        JsonDocument errorDoc = createErrorResponse(validationError);
        return json(request.getServerRequest(), errorDoc);
    }
    
    String targetUrl = request.route("url");
    String method = request.route("method");
    if (method.isEmpty()) {
        method = "GET";
    }
    method.toUpperCase();
    
    DEBUG_PRINTLN("Proxying " + method + " request to: " + targetUrl);
    
    // Security check - only allow certain domains
    if (!targetUrl.startsWith("https://api.") && 
        !targetUrl.startsWith("https://httpbin.org") &&
        !targetUrl.startsWith("http://worldtimeapi.org")) {
        JsonDocument errorDoc = createErrorResponse("URL not allowed for security reasons");
        return json(request.getServerRequest(), errorDoc);
    }
    
    HttpResponse proxyResponse;
    
    if (method == "GET") {
        proxyResponse = _httpClient->get(targetUrl);
    } else if (method == "POST") {
        String body = request.route("body");
        String contentType = request.route("content_type");
        if (contentType.isEmpty()) {
            contentType = "application/json";
        }
        proxyResponse = _httpClient->post(targetUrl, body, contentType);
    } else {
        JsonDocument errorDoc = createErrorResponse("HTTP method not supported: " + method);
        return json(request.getServerRequest(), errorDoc);
    }
    
    JsonDocument response;
    
    if (proxyResponse.success) {
        // Try to parse as JSON, fall back to raw text
        JsonDocument parsedBody;
        DeserializationError error = deserializeJson(parsedBody, proxyResponse.body);
        
        response["status"] = "success";
        response["status_code"] = proxyResponse.statusCode;
        response["response_time"] = proxyResponse.responseTime;
        
        if (error) {
            // Not JSON, return as raw text
            response["data"] = proxyResponse.body;
            response["content_type"] = "text/plain";
        } else {
            // Valid JSON, return parsed
            response["data"] = parsedBody;
            response["content_type"] = "application/json";
        }
        
        DEBUG_PRINTLN("Proxy request successful - Status: " + String(proxyResponse.statusCode));
    } else {
        response["status"] = "error";
        response["message"] = "Proxy request failed";
        response["details"] = proxyResponse.error;
        response["status_code"] = proxyResponse.statusCode;
        
        DEBUG_PRINTLN("Proxy request failed: " + proxyResponse.error);
    }
    
    return json(request.getServerRequest(), response);
}

JsonDocument ApiController::createErrorResponse(const String& message) {
    JsonDocument doc;
    doc["status"] = "error";
    doc["message"] = message;
    doc["timestamp"] = millis();
    return doc;
}

String ApiController::validateParams(Request& request, const std::vector<String>& params) {
    for (const String& param : params) {
        if (request.route(param).isEmpty()) {
            return "Required parameter missing: " + param;
        }
    }
    return "";
}
