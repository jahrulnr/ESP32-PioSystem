#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>
#include "cpu_freq.h"

/**
 * @brief HTTP response structure
 */
struct HttpResponse {
    int statusCode;
    String body;
    std::map<String, String> headers;
    unsigned long responseTime;
    bool success;
    String error;
    
    HttpResponse() : statusCode(0), responseTime(0), success(false) {}
};

/**
 * @brief HTTP request configuration structure
 */
struct HttpConfig {
    unsigned long timeout;          // Request timeout in milliseconds
    unsigned long connectTimeout;   // Connection timeout in milliseconds
    bool followRedirects;          // Whether to follow HTTP redirects
    int maxRedirects;              // Maximum number of redirects to follow
    bool verifySsl;                // Whether to verify SSL certificates
    String userAgent;              // User agent string
    std::map<String, String> defaultHeaders; // Default headers for all requests
    
    HttpConfig() : 
        timeout(30000), 
        connectTimeout(10000), 
        followRedirects(true), 
        maxRedirects(5), 
        verifySsl(true),
        userAgent("ESP32-S3-HttpClient/1.0") {}
};

/**
 * @brief Advanced HTTP Client with SSL support for ESP32-S3
 * 
 * Features:
 * - HTTP and HTTPS support with SSL certificate verification
 * - Custom CA certificate support
 * - Request/response header management
 * - JSON request/response handling
 * - Connection pooling and keep-alive
 * - Authentication (Basic, Bearer token)
 * - Configurable timeouts and redirects
 * - Debug logging integration
 */
class HttpClientManager {
public:
    /**
     * @brief Construct a new HTTP Client Manager
     */
    HttpClientManager();
    
    /**
     * @brief Destroy the HTTP Client Manager
     */
    ~HttpClientManager();

    /**
     * @brief Initialize the HTTP client
     * 
     * @param config Configuration settings
     * @return true if initialization successful
     */
    bool begin(const HttpConfig& config = HttpConfig());

    /**
     * @brief Set basic authentication credentials
     * 
     * @param username Username for basic auth
     * @param password Password for basic auth
     */
    void setBasicAuth(const String& username, const String& password);

    /**
     * @brief Set bearer token for authorization
     * 
     * @param token Bearer token string
     */
    void setBearerToken(const String& token);

    /**
     * @brief Set custom headers for all requests
     * 
     * @param headers Map of header name-value pairs
     */
    void setDefaultHeaders(const std::map<String, String>& headers);

    /**
     * @brief Add a default header
     * 
     * @param name Header name
     * @param value Header value
     */
    void addDefaultHeader(const String& name, const String& value);

    /**
     * @brief Remove a default header
     * 
     * @param name Header name to remove
     */
    void removeDefaultHeader(const String& name);

    /**
     * @brief Make a GET request
     * 
     * @param url Request URL
     * @param headers Optional additional headers
     * @return HttpResponse Response structure
     */
    HttpResponse get(const String& url, const std::map<String, String>& headers = {});

    /**
     * @brief Make a POST request
     * 
     * @param url Request URL
     * @param body Request body
     * @param contentType Content-Type header value
     * @param headers Optional additional headers
     * @return HttpResponse Response structure
     */
    HttpResponse post(const String& url, const String& body, 
                     const String& contentType = "application/json",
                     const std::map<String, String>& headers = {});

    /**
     * @brief Make a PUT request
     * 
     * @param url Request URL
     * @param body Request body
     * @param contentType Content-Type header value
     * @param headers Optional additional headers
     * @return HttpResponse Response structure
     */
    HttpResponse put(const String& url, const String& body,
                    const String& contentType = "application/json",
                    const std::map<String, String>& headers = {});

    /**
     * @brief Make a DELETE request
     * 
     * @param url Request URL
     * @param headers Optional additional headers
     * @return HttpResponse Response structure
     */
    HttpResponse del(const String& url, const std::map<String, String>& headers = {});

    /**
     * @brief Make a PATCH request
     * 
     * @param url Request URL
     * @param body Request body
     * @param contentType Content-Type header value
     * @param headers Optional additional headers
     * @return HttpResponse Response structure
     */
    HttpResponse patch(const String& url, const String& body,
                      const String& contentType = "application/json", 
                      const std::map<String, String>& headers = {});

    /**
     * @brief Make a generic HTTP request
     * 
     * @param method HTTP method
     * @param url Request URL
     * @param body Request body (empty for GET, DELETE, etc.)
     * @param contentType Content-Type header value
     * @param headers Optional additional headers
     * @return HttpResponse Response structure
     */
    HttpResponse request(WebRequestMethod method, const String& url, const String& body = "",
                        const String& contentType = "application/json",
                        const std::map<String, String>& headers = {});

    /**
     * @brief Make a GET request and parse JSON response
     * 
     * @param url Request URL
     * @param headers Optional additional headers
     * @return JsonDocument Parsed JSON response
     */
    JsonDocument getJson(const String& url, const std::map<String, String>& headers = {});

    /**
     * @brief Make a POST request with JSON body
     * 
     * @param url Request URL
     * @param jsonBody JSON document to send
     * @param headers Optional additional headers
     * @return JsonDocument Parsed JSON response
     */
    JsonDocument postJson(const String& url, const JsonDocument& jsonBody,
                         const std::map<String, String>& headers = {});

    /**
     * @brief Make a PUT request with JSON body
     * 
     * @param url Request URL
     * @param jsonBody JSON document to send
     * @param headers Optional additional headers
     * @return JsonDocument Parsed JSON response
     */
    JsonDocument putJson(const String& url, const JsonDocument& jsonBody,
                        const std::map<String, String>& headers = {});

    /**
     * @brief Download a file from URL
     * 
     * @param url File URL
     * @param filePath Local file path to save
     * @param onProgress Optional progress callback
     * @return true if download successful
     */
    bool downloadFile(const String& url, const String& filePath,
                     std::function<void(size_t current, size_t total)> onProgress = nullptr);

    /**
     * @brief Upload a file via multipart form data
     * 
     * @param url Upload URL
     * @param filePath Local file path
     * @param fieldName Form field name
     * @param headers Optional additional headers
     * @return HttpResponse Response structure
     */
    HttpResponse uploadFile(const String& url, const String& filePath, 
                           const String& fieldName = "file",
                           const std::map<String, String>& headers = {});

    /**
     * @brief Get last error message
     * 
     * @return String Error message
     */
    String getLastError();

    /**
     * @brief Get connection statistics
     * 
     * @return std::map<String, int> Statistics map
     */
    std::map<String, int> getStats();

    /**
     * @brief Reset connection statistics
     */
    void resetStats();

    /**
     * @brief Enable/disable debug logging
     * 
     * @param enabled Whether to enable debug output
     */
    void setDebugEnabled(bool enabled);

private:
    HTTPClient _httpClient;
    HttpConfig _config;
    String _lastError;
    bool _debugEnabled;
    
    // Authentication
    String _basicAuthHeader;
    String _bearerToken;
    
    // Statistics
    std::map<String, int> _stats;
    
    /**
     * @brief Setup HTTP client for request
     * 
     * @param url Request URL
     * @param headers Request headers
     * @return true if setup successful
     */
    bool setupRequest(const String& url, const std::map<String, String>& headers);
    
    /**
     * @brief Apply headers to HTTP client
     * 
     * @param headers Headers to apply
     */
    void applyHeaders(const std::map<String, String>& headers);
    
    /**
     * @brief Get response headers from HTTP client
     * 
     * @return std::map<String, String> Response headers
     */
    std::map<String, String> getResponseHeaders();
    
    /**
     * @brief Convert HTTP method enum to string
     * 
     * @param method HTTP method enum
     * @return String HTTP method string
     */
    String methodToString(WebRequestMethod method);
    
    /**
     * @brief Log debug message
     * 
     * @param message Debug message
     */
    void debug(const String& message);
    
    /**
     * @brief Update statistics
     * 
     * @param key Statistics key
     * @param increment Value to add
     */
    void updateStats(const String& key, int increment = 1);
};

extern SemaphoreHandle_t httpClientMutex;

#endif // HTTP_CLIENT_H
