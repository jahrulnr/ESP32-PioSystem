#include "AsyncSimpleNTP.h"
#include "SerialDebug.h"

AsyncSimpleNTP::AsyncSimpleNTP() : 
    _ntpServer("pool.ntp.org"),
    _port(123),
    _lastUpdate(0),
    _updateInterval(3600000), // 1 hour default
    _timeZone(0),
    _isTimeSet(false),
    _packetReceived(false),
    _waitingForPacket(false),
    _packetSendTime(0),
    _sharedUdp(nullptr),
    _usingSharedUdp(false) {
    memset(_packetBuffer, 0, sizeof(_packetBuffer));
}

AsyncSimpleNTP::~AsyncSimpleNTP() {
    if (!_usingSharedUdp) {
        _udp.close();
    }
}

bool AsyncSimpleNTP::begin(const char* server) {
    DEBUG_PRINTF("AsyncSimpleNTP: Beginning with server: %s, port: %d\n", server, _port);
    _ntpServer = server;
    
    // Check WiFi connectivity
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("AsyncSimpleNTP: WiFi not connected! Cannot initialize NTP client.");
        return false;
    }
    
    // Get the STA IP address to bind UDP to the correct interface
    IPAddress staIP = WiFi.localIP();
    DEBUG_PRINTF("AsyncSimpleNTP: WiFi STA IP: %s\n", staIP.toString().c_str());
    
    if (_usingSharedUdp) {
        // Using existing UDP instance, just send NTP packet
        DEBUG_PRINTLN("AsyncSimpleNTP: Using shared UDP instance");
    } else {
        // First try to stop UDP if it was previously started
        _udp.close();
        
        // Set up packet handler with explicit binding to STA interface
        tcpip_adapter_if_t wifi_sta_if = TCPIP_ADAPTER_IF_STA;
        IPAddress staIP = WiFi.localIP();
        
        // First try to bind on the standard port with explicit STA interface
        if (!_udp.listen(staIP, _port)) {
            DEBUG_PRINTF("AsyncSimpleNTP: Failed to open UDP socket on STA IP %s port %d!\n", 
                         staIP.toString().c_str(), _port);
            
            // Try alternate ports
            _port = 1123;  // Try a different port
            DEBUG_PRINTF("AsyncSimpleNTP: Trying alternative port: %d\n", _port);
            
            if (!_udp.listen(staIP, _port)) {
                DEBUG_PRINTLN("AsyncSimpleNTP: Failed to open UDP socket on alternate port!");
                
                // Last resort: try binding to ANY IP but on a high port
                _port = 32123;
                DEBUG_PRINTF("AsyncSimpleNTP: Last attempt with ANY IP on high port: %d\n", _port);
                
                if (!_udp.listen(_port)) {
                    DEBUG_PRINTLN("AsyncSimpleNTP: All UDP socket binding attempts failed!");
                    return false;
                }
            }
        }
        
        DEBUG_PRINTF("AsyncSimpleNTP: UDP socket opened successfully on port %d\n", _port);
        
        // Set up callback for packet receipt
        _udp.onPacket([this](AsyncUDPPacket packet) {
            this->handlePacket(packet);
        });
    }
    
    // Send an NTP packet immediately to trigger sync
    DEBUG_PRINTLN("AsyncSimpleNTP: Sending initial NTP packet...");
    _waitingForPacket = true;
    _packetReceived = false;
    _packetSendTime = millis();
    sendNTPPacket();
    
    // Configure time but don't expect it to be set immediately
    DEBUG_PRINTLN("AsyncSimpleNTP: Starting time configuration process...");
    configTime();
    
    // The packet will be handled asynchronously via callback
    DEBUG_PRINTLN("AsyncSimpleNTP: Initialization complete, waiting for NTP response");
    return true;
}

void AsyncSimpleNTP::setTimeZone(int hours) {
    _timeZone = hours;
    
    // If time is already set, reconfigure with new timezone
    if (_isTimeSet) {
        configTime();
    }
}

void AsyncSimpleNTP::setUpdateInterval(unsigned long interval) {
    _updateInterval = interval;
}

bool AsyncSimpleNTP::forceUpdate() {
    // Force time update
    _lastUpdate = 0;
    return update();
}

bool AsyncSimpleNTP::update() {
    unsigned long currentMillis = millis();
    
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
            DEBUG_PRINTF("AsyncSimpleNTP: Time was set in background to: %s\n", timeStr);
        }
    }
    
    // Check for timeout on waiting for packet
    if (_waitingForPacket && !_packetReceived) {
        if (currentMillis - _packetSendTime > 5000) {  // 5-second timeout
            DEBUG_PRINTLN("AsyncSimpleNTP: NTP packet timeout, will retry later");
            _waitingForPacket = false;
        }
    }
    
    // Check if update interval has passed or if first update or if time is not yet set
    if ((currentMillis - _lastUpdate >= _updateInterval) || (_lastUpdate == 0) || (!_isTimeSet && !_waitingForPacket)) {
        // Check WiFi status first
        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_PRINTLN("AsyncSimpleNTP: Cannot update time - WiFi not connected!");
            return false;
        }
        
        DEBUG_PRINTF("AsyncSimpleNTP: Time to update. Last update: %lu ms ago\n", currentMillis - _lastUpdate);
        _lastUpdate = currentMillis;
        
        // Send a fresh NTP packet
        DEBUG_PRINTLN("AsyncSimpleNTP: Sending fresh NTP packet for update");
        _waitingForPacket = true;
        _packetReceived = false;
        _packetSendTime = currentMillis;
        sendNTPPacket();
        
        return true;
    }
    
    return false;
}

void AsyncSimpleNTP::handlePacket(AsyncUDPPacket packet) {
    // Validate packet size
    if (packet.length() < 48) {
        DEBUG_PRINTLN("AsyncSimpleNTP: Received packet is too small");
        return;
    }
    
    DEBUG_PRINTF("AsyncSimpleNTP: Received NTP response from %s:%d\n", 
                  packet.remoteIP().toString().c_str(), packet.remotePort());
    
    // Read packet into buffer
    memcpy(_packetBuffer, packet.data(), 48);
    
    // NTP response contains the server time in the last 8 bytes
    // Extract the seconds since 1900 from bytes 40-43
    unsigned long highWord = word(_packetBuffer[40], _packetBuffer[41]);
    unsigned long lowWord = word(_packetBuffer[42], _packetBuffer[43]);
    
    // Combine to get seconds since Jan 1, 1900
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    
    // Unix time starts on Jan 1 1970, subtract 70 years in seconds
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epochTime = secsSince1900 - seventyYears;
    
    DEBUG_PRINTF("AsyncSimpleNTP: NTP response time: %lu\n", epochTime);
    
    // Now call configTime to set the system time
    configTime();
    
    // Mark that we've received the packet
    _packetReceived = true;
    _waitingForPacket = false;
    
    // Check if time is now set
    time_t now = 0;
    struct tm timeinfo = {0};
    time(&now);
    localtime_r(&now, &timeinfo);
    
    _isTimeSet = (timeinfo.tm_year > (2016 - 1900));
    
    if (_isTimeSet) {
        char timeStr[50];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        DEBUG_PRINTF("AsyncSimpleNTP: Time successfully set to: %s\n", timeStr);
    } else {
        DEBUG_PRINTLN("AsyncSimpleNTP: Time not set yet despite receiving NTP response");
    }
}

bool AsyncSimpleNTP::isTimeSet() const {
    return _isTimeSet;
}

void AsyncSimpleNTP::sendNTPPacket() {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("AsyncSimpleNTP: Cannot send NTP packet - WiFi not connected!");
        return;
    }
    
    DEBUG_PRINTF("AsyncSimpleNTP: Preparing NTP packet to %s\n", _ntpServer);
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
    DEBUG_PRINTF("AsyncSimpleNTP: Performing DNS lookup for %s\n", _ntpServer);
    
    if (!WiFi.hostByName(_ntpServer, ntpServerIP)) {
        DEBUG_PRINTLN("AsyncSimpleNTP: DNS lookup failed for NTP server!");
        return;
    }
    
    DEBUG_PRINTF("AsyncSimpleNTP: NTP server IP: %s\n", ntpServerIP.toString().c_str());
    
    // Create UDP packet
    if (_usingSharedUdp && _sharedUdp != nullptr) {
        // Using shared UDP instance - convert IP to ip_addr_t
        ip_addr_t dest_addr;
        dest_addr.type = IPADDR_TYPE_V4;
#if CONFIG_LWIP_IPV6
        dest_addr.u_addr.ip4.addr = ntpServerIP;
#else
        dest_addr.addr = ntpServerIP;
#endif
        // Send directly to the NTP server IP using the shared UDP instance
        _sharedUdp->writeTo(_packetBuffer, sizeof(_packetBuffer), &dest_addr, 123);
        DEBUG_PRINTLN("AsyncSimpleNTP: NTP packet sent using shared UDP");
    } else {
        // Using our own UDP instance - explicit STA interface
        tcpip_adapter_if_t wifi_sta_if = TCPIP_ADAPTER_IF_STA;
        _udp.writeTo(_packetBuffer, sizeof(_packetBuffer), ntpServerIP, 123, wifi_sta_if);
        DEBUG_PRINTLN("AsyncSimpleNTP: NTP packet sent using dedicated UDP on STA interface");
    }
}

bool AsyncSimpleNTP::configTime() {
    String tzString = getTzString(_timeZone);
    DEBUG_PRINTF("AsyncSimpleNTP: Configuring time with timezone: %s, server: %s\n", tzString.c_str(), _ntpServer);
    
    // Check WiFi connectivity first
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("AsyncSimpleNTP: WiFi not connected! Cannot sync time.");
        return false;
    }
    
    // Print WiFi details for debugging
    DEBUG_PRINTF("AsyncSimpleNTP: WiFi connected. IP: %s, RSSI: %d dBm\n", 
                 WiFi.localIP().toString().c_str(), WiFi.RSSI());
                 
    // Use ESP32's built-in time configuration
    configTzTime(tzString.c_str(), _ntpServer);
    
    // Check if time was already set
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
        DEBUG_PRINTF("AsyncSimpleNTP: Time successfully set to: %s\n", timeStr);
        DEBUG_PRINTF("AsyncSimpleNTP: Year: %d, Month: %d, Day: %d\n", 
                    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
        return true;
    } else {
        DEBUG_PRINTF("AsyncSimpleNTP: Time not set yet. Current year from system: %d\n", timeinfo.tm_year + 1900);
        DEBUG_PRINTLN("AsyncSimpleNTP: Will check again on next update");
        return false;
    }
}

String AsyncSimpleNTP::getTzString(int timezone) const {
    char tz[20];
    if (timezone >= 0) {
        snprintf(tz, sizeof(tz), "GMT-%d", timezone);
    } else {
        snprintf(tz, sizeof(tz), "GMT+%d", -timezone);
    }
    return String(tz);
}

String AsyncSimpleNTP::getFormattedTime() const {
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

String AsyncSimpleNTP::getFormattedDate() const {
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

String AsyncSimpleNTP::getFormattedDateTime() const {
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

int AsyncSimpleNTP::getHours() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_hour;
}

int AsyncSimpleNTP::getMinutes() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_min;
}

int AsyncSimpleNTP::getSeconds() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_sec;
}

int AsyncSimpleNTP::getDay() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_mday;
}

int AsyncSimpleNTP::getMonth() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_mon + 1;  // tm_mon is 0-11, we return 1-12
}

int AsyncSimpleNTP::getYear() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_year + 1900;  // tm_year is years since 1900
}

int AsyncSimpleNTP::getDayOfWeek() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    return timeinfo.tm_wday;  // 0-6, 0 = Sunday
}

time_t AsyncSimpleNTP::getEpochTime() const {
    if (!_isTimeSet) return 0;
    
    time_t now;
    time(&now);
    return now;
}

JsonObject AsyncSimpleNTP::toJson(JsonDocument& doc) const {
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

void AsyncSimpleNTP::setSharedUDP(AsyncUDP* udp) {
    if (udp != nullptr) {
        // Check if the UDP instance is initialized and connected
        if (udp->connected()) {
            _sharedUdp = udp;
            _usingSharedUdp = true;
            DEBUG_PRINTLN("AsyncSimpleNTP: Using shared UDP instance");
            
            // Close our own UDP if it's open
            if (_udp.connected()) {
                _udp.close();
                DEBUG_PRINTLN("AsyncSimpleNTP: Closed dedicated UDP instance");
            }
        } else {
            DEBUG_PRINTLN("AsyncSimpleNTP: Provided UDP instance is not connected, ignoring");
            _usingSharedUdp = false;
            _sharedUdp = nullptr;
        }
    } else {
        _usingSharedUdp = false;
        _sharedUdp = nullptr;
        DEBUG_PRINTLN("AsyncSimpleNTP: Disabled shared UDP instance");
    }
}