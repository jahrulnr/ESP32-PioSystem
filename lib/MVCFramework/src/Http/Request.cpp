#include "Request.h"

// Static variable to store body data between handlers
static String requestBodyData;

// Body handler callback function
void handleRequestBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (total > 0 && data != nullptr) {
        if (index == 0) {
            requestBodyData = "";
        }
        
        for (size_t i = 0; i < len; i++) {
            requestBodyData += (char)data[i];
        }
        
        if (index + len == total) {
            // Store in request
            request->_tempObject = new String(requestBodyData);
        }
    }
}

// Static method to set up body handling for the server
void Request::setupBodyHandling(AsyncWebServer* server) {
    server->onRequestBody(handleRequestBody);
    
    // Set up file upload handling
    server->onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
        // This callback ensures file uploads are properly handled
        // The ESP Async Web Server already processes file uploads and makes them available via getParam
    });
}

Request::Request(AsyncWebServerRequest* request) : serverRequest(request) {
    // Extract headers
    int headerCount = request->headers();
    for (int i = 0; i < headerCount; i++) {
        const AsyncWebHeader* header = request->getHeader(i);
        headers[header->name()] = header->value();
    }
    
    // Extract parameters (but skip file uploads, as they're handled separately)
    int paramCount = request->params();
    for (int i = 0; i < paramCount; i++) {
        const AsyncWebParameter* param = request->getParam(i);
        if (!param->isFile()) {
            parameters[param->name()] = param->value();
            
            // Check if this parameter is the request body (POST data)
            if (param->isPost()) {
                if (param->name() == "plain" || param->name() == "body" || param->name() == "") {
                    body = param->value();
                }
            }
        }
    }
    
    // Also try to get body from specific body parameters
    const AsyncWebParameter* bodyParam = request->getParam("body", true);
    if (!bodyParam) {
        bodyParam = request->getParam("plain", true);
    }
    
    if (bodyParam && !bodyParam->isFile()) {
        body = bodyParam->value();
    }
    
    // Check for raw body data stored by the body handler
    if (body.isEmpty() && request->_tempObject != nullptr) {
        // Get the body from the temporary object
        String* tempBody = static_cast<String*>(request->_tempObject);
        body = *tempBody;
        
        // Clean up (don't delete - the server will do this)
        // delete tempBody;
        // request->_tempObject = nullptr;
    }
}

String Request::method() const {
    if (!serverRequest) return "";
    
    switch (serverRequest->method()) {
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

String Request::url() const {
    if (!serverRequest) return "";
    return serverRequest->url();
}

String Request::path() const {
    String fullUrl = url();
    int queryIndex = fullUrl.indexOf('?');
    if (queryIndex >= 0) {
        return fullUrl.substring(0, queryIndex);
    }
    return fullUrl;
}

String Request::query() const {
    String fullUrl = url();
    int queryIndex = fullUrl.indexOf('?');
    if (queryIndex >= 0) {
        return fullUrl.substring(queryIndex + 1);
    }
    return "";
}

String Request::input(const String& key, const String& defaultValue) const {
    return get(key, defaultValue);
}

String Request::get(const String& key, const String& defaultValue) const {
    auto it = parameters.find(key);
    if (it != parameters.end()) {
        return it->second;
    }
    return defaultValue;
}

String Request::post(const String& key, const String& defaultValue) const {
    if (isPost()) {
        return get(key, defaultValue);
    }
    return defaultValue;
}

bool Request::has(const String& key) const {
    return parameters.find(key) != parameters.end();
}

String Request::header(const String& name, const String& defaultValue) const {
    auto it = headers.find(name);
    if (it != headers.end()) {
        return it->second;
    }
    return defaultValue;
}

bool Request::hasHeader(const String& name) const {
    return headers.find(name) != headers.end();
}

bool Request::hasFile(const String& name) const {
    if (!serverRequest) return false;
    
    // Check if a file with this name exists in the request
    return serverRequest->hasParam(name, true, true);
}

String Request::getFile(const String& name) const {
    if (!serverRequest || !hasFile(name)) return "";
    
    const AsyncWebParameter* param = serverRequest->getParam(name, true, true);
    if (param && param->isFile()) {
        return param->value();
    }
    return "";
}

String Request::getFileName(const String& name) const {
    if (!serverRequest || !hasFile(name)) return "";
    
    const AsyncWebParameter* param = serverRequest->getParam(name, true, true);
    if (param && param->isFile()) {
        return param->name();
    }
    return "";
}

size_t Request::getFileSize(const String& name) const {
    if (!serverRequest || !hasFile(name)) return 0;
    
    const AsyncWebParameter* param = serverRequest->getParam(name, true, true);
    if (param && param->isFile()) {
        return param->size();
    }
    return 0;
}

bool Request::filled(const String& key) const {
    String value = get(key);
    return value.length() > 0;
}

JsonDocument Request::json() const {
    JsonDocument doc;
    if (body.length() > 0) {
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            Serial.print("Failed to parse JSON body: ");
            Serial.println(error.c_str());
            Serial.print("Body content: ");
            Serial.println(body);
        }
    }
    return doc;
}

bool Request::wantsJson() const {
    String accept = header("Accept");
    String contentType = header("Content-Type");
    
    return accept.indexOf("application/json") >= 0 || 
           contentType.indexOf("application/json") >= 0;
}

String Request::ip() const {
    if (!serverRequest) return "";
    return serverRequest->client()->remoteIP().toString();
}

String Request::userAgent() const {
    return header("User-Agent");
}

void Request::setRouteParameter(const String& key, const String& value) {
    parameters[key] = value;
}

String Request::route(const String& key, const String& defaultValue) const {
    return get(key, defaultValue);
}
