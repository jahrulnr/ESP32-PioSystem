#include "SimpleNTP.h"
#include "SerialDebug.h"

SimpleNTP::SimpleNTP() : 
    _ntpServer("pool.ntp.org"),
    _port(123),
    _lastUpdate(0),
    _updateInterval(3600000), // 1 hour default
    _timeZone(0),
    _isTimeSet(false) {
    memset(_packetBuffer, 0, sizeof(_packetBuffer));
}

SimpleNTP::~SimpleNTP() {
    _udp.stop();
}

bool SimpleNTP::begin(const char* server) {
    DEBUG_PRINTF("SimpleNTP: Beginning with server: %s, port: %d\n", server, _port);
    _ntpServer = server;
    
    // Check WiFi connectivity
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }
    
    // First try to stop UDP if it was previously started
    DEBUG_PRINTLN("SimpleNTP: Stopping any existing UDP connections");
    _udp.stop();
    
    // Give a small delay to ensure clean start
    delay(50);
    yield();
    
    // Get the STA IP address to bind UDP to the correct interface
    IPAddress staIP = WiFi.localIP();
    DEBUG_PRINTF("SimpleNTP: WiFi STA IP: %s\n", staIP.toString().c_str());
    
    // Try to start UDP with multiple attempts, explicitly binding to the STA IP
    bool udpStarted = false;
    for (int attempt = 1; attempt <= 3 && !udpStarted; attempt++) {
        DEBUG_PRINTF("SimpleNTP: Starting UDP attempt %d/3 on port %d binding to %s\n", 
                      attempt, _port, staIP.toString().c_str());
        
        // First stop any existing UDP
        _udp.stop();
        
        // Try to begin UDP with explicit IP binding to STA interface
        udpStarted = _udp.begin(_port);
        
        if (udpStarted) {
            DEBUG_PRINTLN("SimpleNTP: UDP socket opened successfully");
            break;
        } else {
            DEBUG_PRINTF("SimpleNTP: Failed to open UDP socket on attempt %d\n", attempt);
            
            if (attempt < 3) {
                // Try a different port if first attempt fails
                _port = (_port == 123) ? 1123 : (_port + 1);
                DEBUG_PRINTF("SimpleNTP: Trying alternative port: %d\n", _port);
                delay(50); // Shorter delay
                yield();
            }
        }
    }
    
    if (!udpStarted) {
        DEBUG_PRINTLN("SimpleNTP: All attempts to open UDP socket failed!");
        return false;
    }
    
    // UDP is started successfully, now send an NTP packet to trigger sync
    DEBUG_PRINTLN("SimpleNTP: Sending initial NTP packet...");
    sendNTPPacket();
    
    // Give the system a moment to process the packet
    delay(20);
    yield();
    
    // Configure time but don't expect it to be set immediately
    DEBUG_PRINTLN("SimpleNTP: Starting time configuration process...");
    bool configResult = configTime();
    
    DEBUG_PRINTF("SimpleNTP: Initial configuration %s. Time will synchronize in the background.\n",
                 configResult ? "started successfully" : "initiated but time not set yet");
    
    // Return true if initialization was successful (even if time is not set yet)
    // The update() method will handle periodic updates
    return true;
}

void SimpleNTP::setTimeZone(int hours) {
    _timeZone = hours;
    
    // If time is already set, reconfigure with new timezone
    if (_isTimeSet) {
        configTime();
    }
}

void SimpleNTP::setUpdateInterval(unsigned long interval) {
    _updateInterval = interval;
}

bool SimpleNTP::forceUpdate() {
    // Force time update
    _lastUpdate = 0;
    return update();
}

bool SimpleNTP::update() {
    unsigned long currentMillis = millis();
    unsigned long timeSinceLastUpdate = currentMillis - _lastUpdate;
    
    // First check if time is already set by calling ESP32's time functions
    if (!_isTimeSet) {
        time_t now = 0;
        struct tm timeinfo = {0};
        time(&now);
        localtime_r(&now, &timeinfo);
        
        // Check if time was set since our last check
        if (timeinfo.tm_year > (2016 - 1900)) {
            _isTimeSet = true;
            char timeStr[50];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
            DEBUG_PRINTF("SimpleNTP: Time was set in background to: %s\n", timeStr);
        }
    }
    
    // Check if update interval has passed or if first update or if time is not yet set
    if ((timeSinceLastUpdate >= _updateInterval) || (_lastUpdate == 0) || !_isTimeSet) {
        // Check WiFi status first
        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_PRINTLN("SimpleNTP: Cannot update time - WiFi not connected!");
            return false;
        }
        
        DEBUG_PRINTF("SimpleNTP: Time to update. Last update: %lu ms ago\n", timeSinceLastUpdate);
        DEBUG_PRINTF("SimpleNTP: Current time status: %s\n", _isTimeSet ? "Set" : "Not set");
        
        _lastUpdate = currentMillis;
        
        // Get the STA IP to ensure we're using the correct interface
        IPAddress staIP = WiFi.localIP();
        DEBUG_PRINTF("SimpleNTP: Current STA IP: %s for UDP rebinding\n", staIP.toString().c_str());
        
        // If UDP is not open, try to reopen it with explicit binding to STA IP
        _udp.stop();
        if (!_udp.begin(_port)) {
            DEBUG_PRINTF("SimpleNTP: Failed to reopen UDP socket on port %d\n", _port);
            
            // Try different ports
            _port = (_port == 123) ? 1123 : (_port == 1123) ? 2123 : 123;
            
            if (!_udp.begin(_port)) {
                DEBUG_PRINTLN("SimpleNTP: Could not initialize UDP on any port!");
                return false;
            } else {
                DEBUG_PRINTF("SimpleNTP: Reopened UDP on alternative port %d\n", _port);
            }
        } else {
            DEBUG_PRINTF("SimpleNTP: UDP socket reopened on port %d\n", _port);
        }
        
        // Send a fresh NTP packet first
        DEBUG_PRINTLN("SimpleNTP: Sending fresh NTP packet for update");
        sendNTPPacket();
        
        // Small delay to give time for UDP packet to be sent
        delay(20);
        yield();
        
        // Try to configure time
        DEBUG_PRINTLN("SimpleNTP: Running configTime...");
        bool result = configTime();
        
        // If time is still not set, schedule a quicker retry
        if (!result && !_isTimeSet) {
            unsigned long retryDelay = 30000; // 30 seconds by default
            
            // Use a static variable with instance tracking to maintain per-object state
            static int failedAttempts = 0;
            failedAttempts++;
            
            if (failedAttempts > 10) {
                retryDelay = 300000; // After 10 attempts, retry every 5 minutes
            } else if (failedAttempts > 5) {
                retryDelay = 60000;  // After 5 attempts, retry every minute
            }
            
            _lastUpdate = currentMillis - _updateInterval + retryDelay;
            DEBUG_PRINTF("SimpleNTP: Will retry time sync in %lu seconds (attempt %d)\n", 
                          retryDelay / 1000, failedAttempts);
            
            // Try a different server if we've had multiple failures
            if (failedAttempts % 3 == 0) {
                // Array of reliable NTP servers to try
                const char* backupServers[] = {
                    "time.google.com", 
                    "pool.ntp.org", 
                    "time.cloudflare.com",
                    "time.windows.com",
                    "time.apple.com"
                };
                
                int serverIndex = (failedAttempts / 3) % 5;
                _ntpServer = backupServers[serverIndex];
                DEBUG_PRINTF("SimpleNTP: Switching to alternate server: %s\n", _ntpServer);
            }
            
        } else if (result) {
            // Reset failed attempts counter on success
            DEBUG_PRINTLN("SimpleNTP: Update successful");
        }
        
        return result;
    }
    
    return false;
}

bool SimpleNTP::isTimeSet() const {
    return _isTimeSet;
}

String SimpleNTP::getFormattedTime() const {
    if (!_isTimeSet) {
        return "00:00:00";
    }
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char buffer[9];  // HH:MM:SS\0
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", 
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    return String(buffer);
}

String SimpleNTP::getFormattedDate() const {
    if (!_isTimeSet) {
        return "0000-00-00";
    }
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char buffer[11];  // YYYY-MM-DD\0
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", 
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    
    return String(buffer);
}

String SimpleNTP::getFormattedDateTime() const {
    if (!_isTimeSet) {
        return "0000-00-00 00:00:00";
    }
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char buffer[20];  // YYYY-MM-DD HH:MM:SS\0
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d", 
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    return String(buffer);
}

int SimpleNTP::getHours() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_hour;
}

int SimpleNTP::getMinutes() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_min;
}

int SimpleNTP::getSeconds() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_sec;
}

int SimpleNTP::getDay() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_mday;
}

int SimpleNTP::getMonth() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_mon + 1;  // tm_mon is 0-11, we return 1-12
}

int SimpleNTP::getYear() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_year + 1900;  // tm_year is years since 1900
}

int SimpleNTP::getDayOfWeek() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_wday;  // 0-6, 0 = Sunday
}

time_t SimpleNTP::getEpochTime() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    time(&now);
    return now;
}

JsonObject SimpleNTP::toJson(JsonDocument& doc) const {
    JsonObject timeObj = doc.to<JsonObject>();
    
    if (!_isTimeSet) {
        timeObj["status"] = "not_set";
        return timeObj;
    }
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    timeObj["status"] = "set";
    timeObj["epoch"] = now;
    timeObj["timezone"] = _timeZone;
    timeObj["time"] = getFormattedTime();
    timeObj["date"] = getFormattedDate();
    timeObj["datetime"] = getFormattedDateTime();
    
    JsonObject details = timeObj.createNestedObject("details");
    details["year"] = timeinfo.tm_year + 1900;
    details["month"] = timeinfo.tm_mon + 1;
    details["day"] = timeinfo.tm_mday;
    details["hour"] = timeinfo.tm_hour;
    details["minute"] = timeinfo.tm_min;
    details["second"] = timeinfo.tm_sec;
    details["dayofweek"] = timeinfo.tm_wday;
    
    return timeObj;
}

void SimpleNTP::sendNTPPacket() {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("SimpleNTP: Cannot send NTP packet - WiFi not connected!");
        return;
    }
    
    DEBUG_PRINTF("SimpleNTP: Preparing NTP packet to %s\n", _ntpServer);
    memset(_packetBuffer, 0, sizeof(_packetBuffer));
    
    // Initialize values needed to form NTP request
    _packetBuffer[0] = 0b11100011;   // LI, Version, Mode (NTP v4, Client)
    _packetBuffer[1] = 0;            // Stratum, or type of clock
    _packetBuffer[2] = 6;            // Polling Interval
    _packetBuffer[3] = 0xEC;         // Peer Clock Precision
    
    // 8 bytes of zero for Root Delay & Root Dispersion
    _packetBuffer[12] = 49;
    _packetBuffer[13] = 0x4E;
    _packetBuffer[14] = 49;
    _packetBuffer[15] = 52;
    
    // Try to resolve host with timeout
    IPAddress ntpServerIP;
    DEBUG_PRINTF("SimpleNTP: Performing DNS lookup for %s\n", _ntpServer);
    
    // Try multiple times with a short delay
    bool dnsSuccess = false;
    for (int attempt = 1; attempt <= 3 && !dnsSuccess; attempt++) {
        DEBUG_PRINTF("SimpleNTP: DNS lookup attempt %d/3\n", attempt);
        dnsSuccess = WiFi.hostByName(_ntpServer, ntpServerIP);
        
        if (dnsSuccess) {
            break;
        } else {
            if (attempt < 3) {
                DEBUG_PRINTLN("SimpleNTP: DNS lookup failed, retrying...");
                delay(100);  // Short delay before retry
                yield();
            }
        }
    }
    
    if (!dnsSuccess) {
        DEBUG_PRINTLN("SimpleNTP: All DNS lookup attempts failed for NTP server!");
        return;
    }
    
    DEBUG_PRINTF("SimpleNTP: NTP server IP: %s\n", ntpServerIP.toString().c_str());
    
    // Get the STA IP to ensure we're using the correct interface
    IPAddress staIP = WiFi.localIP();
    DEBUG_PRINTF("SimpleNTP: Current STA IP: %s\n", staIP.toString().c_str());
    
    // Reinitialize UDP with explicit binding to STA IP
    _udp.stop();
    if (!_udp.begin(_port)) {
        DEBUG_PRINTLN("SimpleNTP: Failed to initialize UDP client");
        return;
    }
    
    // Send the packet with retries
    bool packetSent = false;
    for (int attempt = 1; attempt <= 2 && !packetSent; attempt++) {
        DEBUG_PRINTF("SimpleNTP: Sending packet attempt %d/2\n", attempt);
        
        // Use explicit beginPacket with NTP server IP and port 123 (NTP port)
        if (_udp.beginPacket(ntpServerIP, 123)) {
            DEBUG_PRINTLN("SimpleNTP: UDP packet started");
            
            // Write the packet to avoid buffer issues
            _udp.write(_packetBuffer, sizeof(_packetBuffer));
            yield(); // Allow other processing
            
            if (_udp.endPacket()) {
                DEBUG_PRINTLN("SimpleNTP: NTP packet sent successfully");
                packetSent = true;
                break;
            } else {
                DEBUG_PRINTLN("SimpleNTP: Failed to send NTP packet");
                if (attempt < 2) {
                    delay(200);  // Wait before retry
                    yield();
                }
            }
        } else {
            DEBUG_PRINTLN("SimpleNTP: Failed to start UDP packet");
            if (attempt < 2) {
                delay(200);
                yield();
            }
        }
    }
    
    if (!packetSent) {
        DEBUG_PRINTLN("SimpleNTP: All attempts to send NTP packet failed!");
    } else {
        // Short delay to allow packet to be processed
        delay(10);
        yield();
    }
}

bool SimpleNTP::configTime() {
    String tzString = getTzString(_timeZone);
    DEBUG_PRINTF("SimpleNTP: Configuring time with timezone: %s, server: %s\n", tzString.c_str(), _ntpServer);
    
    // Check WiFi connectivity first
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("SimpleNTP: WiFi not connected! Cannot sync time.");
        return false;
    }
    
    // Print WiFi details for debugging
    DEBUG_PRINTF("SimpleNTP: WiFi connected. IP: %s, RSSI: %d dBm\n", 
                 WiFi.localIP().toString().c_str(), WiFi.RSSI());
                 
    // Use ESP32's built-in time configuration - non-blocking
    // This sets up the system but doesn't wait for the sync to complete
    DEBUG_PRINTLN("SimpleNTP: Calling configTzTime() with server: " + String(_ntpServer));
    configTzTime(tzString.c_str(), _ntpServer);
    
    // Give a brief moment for the configuration to take effect
    delay(10);
    yield();
    
    // Check if time was already set (might happen instantly if synced recently)
    time_t now = 0;
    struct tm timeinfo = {0};
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Check if we got a valid year (greater than 2016)
    _isTimeSet = (timeinfo.tm_year > (2016 - 1900));
    
    // Log the status and time details
    if (_isTimeSet) {
        char timeStr[50];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        DEBUG_PRINTF("SimpleNTP: Time successfully set to: %s\n", timeStr);
        DEBUG_PRINTF("SimpleNTP: Year: %d, Month: %d, Day: %d\n", 
                    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
        return true;
    } else {
        DEBUG_PRINTF("SimpleNTP: Time not set yet. Current year from system: %d\n", timeinfo.tm_year + 1900);
        DEBUG_PRINTLN("SimpleNTP: Will check again on next update");
        return false; // Time wasn't set yet, but configuration was started
    }
}

String SimpleNTP::getTzString(int timezone) const {
    // Create timezone string in format required by ESP32 (e.g., "EST5EDT,M3.2.0,M11.1.0")
    // For simplicity, we just use hour offset without DST
    char tz[20];
    if (timezone >= 0) {
        snprintf(tz, sizeof(tz), "GMT%d", timezone);
    } else {
        snprintf(tz, sizeof(tz), "GMT%d", timezone);
    }
    return String(tz);
}
