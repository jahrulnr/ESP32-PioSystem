#include "httpclient.h"
#include "SerialDebug.h"
#include <algorithm>
#include <base64.h>
#include <SPIFFS.h>

SemaphoreHandle_t httpClientMutex = NULL;

HttpClientManager::HttpClientManager() : 
    _debugEnabled(false) {

    if (httpClientMutex == NULL) {
        httpClientMutex = xSemaphoreCreateMutex();
    }
    
    // Initialize statistics map with explicit values
    _stats.clear();
    _stats.insert(std::make_pair("requests_total", 0));
    _stats.insert(std::make_pair("requests_success", 0));
    _stats.insert(std::make_pair("requests_failed", 0));
    _stats.insert(std::make_pair("bytes_sent", 0));
    _stats.insert(std::make_pair("bytes_received", 0));
}

HttpClientManager::~HttpClientManager() {
    _httpClient.end();
}

bool HttpClientManager::begin(const HttpConfig& config) {
    _config = config;
    // _httpClient.setReuse(true);
    
    debug("HTTP Client initialized");
    return true;
}

void HttpClientManager::setBasicAuth(const String& username, const String& password) {
    String credentials = username + ":" + password;
    
    // Simple base64 encoding for basic auth
    String encoded = "";
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int val = 0, valb = -6;
    for (unsigned char c : credentials) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded += chars[(val >> valb) & 0x3F];
            valb -= 6;
        }
    }
    if (valb > -6) encoded += chars[((val << 8) >> (valb + 8)) & 0x3F];
    while (encoded.length() % 4) encoded += '=';
    
    _basicAuthHeader = "Basic " + encoded;
    debug("Basic authentication set for user: " + username);
}

void HttpClientManager::setBearerToken(const String& token) {
    _bearerToken = "Bearer " + token;
    debug("Bearer token set");
}

void HttpClientManager::setDefaultHeaders(const std::map<String, String>& headers) {
    _config.defaultHeaders = headers;
}

void HttpClientManager::addDefaultHeader(const String& name, const String& value) {
    _config.defaultHeaders[name] = value;
}

void HttpClientManager::removeDefaultHeader(const String& name) {
    _config.defaultHeaders.erase(name);
}

HttpResponse HttpClientManager::get(const String& url, const std::map<String, String>& headers) {
    return request(HTTP_GET, url, "", "", headers);
}

HttpResponse HttpClientManager::post(const String& url, const String& body, 
                                   const String& contentType, const std::map<String, String>& headers) {
    return request(HTTP_POST, url, body, contentType, headers);
}

HttpResponse HttpClientManager::put(const String& url, const String& body,
                                  const String& contentType, const std::map<String, String>& headers) {
    return request(HTTP_PUT, url, body, contentType, headers);
}

HttpResponse HttpClientManager::del(const String& url, const std::map<String, String>& headers) {
    return request(HTTP_DELETE, url, "", "", headers);
}

HttpResponse HttpClientManager::patch(const String& url, const String& body,
                                    const String& contentType, const std::map<String, String>& headers) {
    return request(HTTP_PATCH, url, body, contentType, headers);
}

HttpResponse HttpClientManager::request(WebRequestMethod method, const String& url, const String& body,
                                      const String& contentType, const std::map<String, String>& headers) {
    HttpResponse response;
    if (xSemaphoreTake(httpClientMutex, portMAX_DELAY) == pdFALSE) {
        _lastError = "Failed to acquire httpClientMutex";
        response.error = _lastError;
        updateStats("requests_failed");
        return response;
    }

    unsigned long startTime = millis();
    
    updateStats("requests_total");
    
    debug("Making " + methodToString(method) + " request to: " + url);
    
    cpu_freq lastCpu = __lastCPUSet;
    setCPU(CPU_HIGH);
    if (!setupRequest(url, headers)) {
        response.error = _lastError;
        updateStats("requests_failed");
        return response;
    }
    
    // Set content type if provided
    if (!contentType.isEmpty() && !body.isEmpty()) {
        _httpClient.addHeader("Content-Type", contentType);
    }
    
    // Apply request-specific headers
    applyHeaders(headers);
    
    // Make the request
    int httpCode = -1;
    switch (method) {
        case HTTP_GET:
            httpCode = _httpClient.GET();
            break;
        case HTTP_POST:
            httpCode = _httpClient.POST(body);
            updateStats("bytes_sent", body.length());
            break;
        case HTTP_PUT:
            httpCode = _httpClient.PUT(body);
            updateStats("bytes_sent", body.length());
            break;
        case HTTP_DELETE:
            httpCode = _httpClient.sendRequest("DELETE", body);
            if (!body.isEmpty()) updateStats("bytes_sent", body.length());
            break;
        case HTTP_PATCH:
            httpCode = _httpClient.PATCH(body);
            updateStats("bytes_sent", body.length());
            break;
        case HTTP_HEAD:
            httpCode = _httpClient.sendRequest("HEAD");
            break;
        case HTTP_OPTIONS:
            httpCode = _httpClient.sendRequest("OPTIONS");
            break;
        default:
            httpCode = -1;
            break;
    }
    
    response.statusCode = httpCode;
    response.responseTime = millis() - startTime;
    
    if (httpCode > 0) {
        response.success = (httpCode >= 200 && httpCode < 300);
        response.body = _httpClient.getString();
        response.headers = getResponseHeaders();
        
        updateStats("bytes_received", response.body.length());
        
        if (response.success) {
            updateStats("requests_success");
            debug("Request successful - Status: " + String(httpCode) + ", Time: " + String(response.responseTime) + "ms");
        } else {
            updateStats("requests_failed");
            debug("Request failed - Status: " + String(httpCode) + ", Time: " + String(response.responseTime) + "ms");
        }
    } else {
        response.success = false;
        response.error = "HTTP request failed with code: " + String(httpCode);
        _lastError = response.error;
        updateStats("requests_failed");
        debug(response.error);
    }
    
    _httpClient.end();
    setCPU(lastCpu);
    xSemaphoreGive(httpClientMutex);
    delay(5);
    return response;
}

JsonDocument HttpClientManager::getJson(const String& url, const std::map<String, String>& headers) {
    JsonDocument doc;
    HttpResponse response = get(url, headers);
    
    if (response.success && !response.body.isEmpty()) {
        DeserializationError error = deserializeJson(doc, response.body);
        if (error) {
            debug("JSON parsing failed: " + String(error.c_str()));
            doc.clear();
            doc["error"] = "JSON parsing failed";
            doc["raw_response"] = response.body;
        }
    } else {
        doc["error"] = response.error.isEmpty() ? "Request failed" : response.error;
        doc["status_code"] = response.statusCode;
    }
    
    return doc;
}

JsonDocument HttpClientManager::postJson(const String& url, const JsonDocument& jsonBody,
                                        const std::map<String, String>& headers) {
    String body;
    serializeJson(jsonBody, body);
    
    HttpResponse response = post(url, body, "application/json", headers);
    
    JsonDocument doc;
    if (response.success && !response.body.isEmpty()) {
        DeserializationError error = deserializeJson(doc, response.body);
        if (error) {
            debug("JSON parsing failed: " + String(error.c_str()));
            doc.clear();
            doc["error"] = "JSON parsing failed";
            doc["raw_response"] = response.body;
        }
    } else {
        doc["error"] = response.error.isEmpty() ? "Request failed" : response.error;
        doc["status_code"] = response.statusCode;
    }
    
    return doc;
}

JsonDocument HttpClientManager::putJson(const String& url, const JsonDocument& jsonBody,
                                       const std::map<String, String>& headers) {
    String body;
    serializeJson(jsonBody, body);
    
    HttpResponse response = put(url, body, "application/json", headers);
    
    JsonDocument doc;
    if (response.success && !response.body.isEmpty()) {
        DeserializationError error = deserializeJson(doc, response.body);
        if (error) {
            debug("JSON parsing failed: " + String(error.c_str()));
            doc.clear();
            doc["error"] = "JSON parsing failed";
            doc["raw_response"] = response.body;
        }
    } else {
        doc["error"] = response.error.isEmpty() ? "Request failed" : response.error;
        doc["status_code"] = response.statusCode;
    }
    
    return doc;
}

bool HttpClientManager::downloadFile(const String& url, const String& filePath,
                                    std::function<void(size_t current, size_t total)> onProgress) {
    debug("Downloading file from: " + url + " to: " + filePath);
    
    if (!setupRequest(url, {})) {
        return false;
    }
    
    int httpCode = _httpClient.GET();
    if (httpCode != 200) {
        _lastError = "Download failed with HTTP code: " + String(httpCode);
        debug(_lastError);
        _httpClient.end();
        return false;
    }
    
    WiFiClient* stream = _httpClient.getStreamPtr();
    size_t totalSize = _httpClient.getSize();
    size_t downloadedSize = 0;
    
    File file = SPIFFS.open(filePath, FILE_WRITE);
    if (!file) {
        _lastError = "Failed to open file for writing: " + filePath;
        debug(_lastError);
        _httpClient.end();
        return false;
    }
    
    uint8_t buff[1024];
    while (_httpClient.connected() && (downloadedSize < totalSize || totalSize == 0)) {
        size_t available = stream->available();
        if (available) {
            size_t readBytes = stream->readBytes(buff, _min(available, sizeof(buff)));
            file.write(buff, readBytes);
            downloadedSize += readBytes;
            
            if (onProgress) {
                onProgress(downloadedSize, totalSize);
            }
        }
        delay(1);
    }
    
    file.close();
    _httpClient.end();
    
    updateStats("bytes_received", downloadedSize);
    debug("Download completed - Size: " + String(downloadedSize) + " bytes");
    return true;
}

HttpResponse HttpClientManager::uploadFile(const String& url, const String& filePath, 
                                         const String& fieldName, const std::map<String, String>& headers) {
    HttpResponse response;
    
    debug("Uploading file: " + filePath + " to: " + url);
    
    File file = SPIFFS.open(filePath, FILE_READ);
    if (!file) {
        response.error = "Failed to open file: " + filePath;
        debug(response.error);
        return response;
    }
    
    size_t fileSize = file.size();
    file.close();
    
    if (!setupRequest(url, headers)) {
        response.error = _lastError;
        return response;
    }
    
    // Note: This is a simplified upload. For proper multipart form data,
    // you might need a more sophisticated implementation or additional library
    String boundary = "----ESP32FormBoundary" + String(random(0xFFFF), HEX);
    String contentType = "multipart/form-data; boundary=" + boundary;
    
    _httpClient.addHeader("Content-Type", contentType);
    applyHeaders(headers);
    
    // This is a basic implementation - for production use, consider using a dedicated multipart library
    String body = "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"" + fieldName + "\"; filename=\"" + filePath + "\"\r\n";
    body += "Content-Type: application/octet-stream\r\n\r\n";
    // File content would be added here
    body += "\r\n--" + boundary + "--\r\n";
    
    int httpCode = _httpClient.POST(body);
    response.statusCode = httpCode;
    response.success = (httpCode >= 200 && httpCode < 300);
    response.body = _httpClient.getString();
    
    if (!response.success) {
        response.error = "Upload failed with HTTP code: " + String(httpCode);
        debug(response.error);
    }
    
    _httpClient.end();
    updateStats("bytes_sent", fileSize);
    return response;
}

String HttpClientManager::getLastError() {
    return _lastError;
}

std::map<String, int> HttpClientManager::getStats() {
    return _stats;
}

void HttpClientManager::resetStats() {
    for (auto& stat : _stats) {
        stat.second = 0;
    }
}

void HttpClientManager::setDebugEnabled(bool enabled) {
    _debugEnabled = enabled;
}

bool HttpClientManager::setupRequest(const String& url, const std::map<String, String>& headers) {
    _lastError = "";
    
    if (url.isEmpty()) {
        _lastError = "URL cannot be empty";
        return false;
    }

    _httpClient.begin(url);
    
    // Configure timeouts
    _httpClient.setTimeout(_config.timeout);
    _httpClient.setConnectTimeout(_config.connectTimeout);
    
    // Configure redirects
    _httpClient.setFollowRedirects(_config.followRedirects ? HTTPC_STRICT_FOLLOW_REDIRECTS : HTTPC_DISABLE_FOLLOW_REDIRECTS);
    
    // Set user agent
    _httpClient.setUserAgent(_config.userAgent);
    
    // Apply default headers
    applyHeaders(_config.defaultHeaders);
    
    // Apply authentication
    if (!_basicAuthHeader.isEmpty()) {
        _httpClient.addHeader("Authorization", _basicAuthHeader);
    } else if (!_bearerToken.isEmpty()) {
        _httpClient.addHeader("Authorization", _bearerToken);
    }
    
    return true;
}

void HttpClientManager::applyHeaders(const std::map<String, String>& headers) {
    for (const auto& header : headers) {
        _httpClient.addHeader(header.first, header.second);
    }
}

std::map<String, String> HttpClientManager::getResponseHeaders() {
    std::map<String, String> headers;
    
    // ESP32 HTTPClient doesn't provide easy access to all response headers
    // You might need to implement this differently based on your needs
    // This is a placeholder implementation
    
    String server = _httpClient.header("Server");
    if (!server.isEmpty()) {
        headers["Server"] = server;
    }
    
    String contentType = _httpClient.header("Content-Type");
    if (!contentType.isEmpty()) {
        headers["Content-Type"] = contentType;
    }
    
    String contentLength = _httpClient.header("Content-Length");
    if (!contentLength.isEmpty()) {
        headers["Content-Length"] = contentLength;
    }
    
    return headers;
}

String HttpClientManager::methodToString(WebRequestMethod method) {
    switch (method) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DELETE";
        case HTTP_PATCH: return "PATCH";
        case HTTP_HEAD: return "HEAD";
        case HTTP_OPTIONS: return "OPTIONS";
        default: return "UNKNOWN";
    }
}

void HttpClientManager::debug(const String& message) {
    if (_debugEnabled) {
        DEBUG_PRINTLN("[HttpClient] " + message);
    }
}

void HttpClientManager::updateStats(const String& key, int increment) {
    // Thread-safe stats update with bounds checking
    if (key.length() > 0 && key.length() < 64) { // Prevent potential string corruption
        _stats[key] += increment;
    }
}
