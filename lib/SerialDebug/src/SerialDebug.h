#ifndef SERIAL_DEBUG_H
#define SERIAL_DEBUG_H
// #include "SerialDebug.h"

#include <Arduino.h>

// define SERIAL_DEBUG on platformio.ini

#ifdef SERIAL_DEBUG
    // Enable debug macros
    #define DEBUG_BEGIN(baud) Serial.begin(baud)
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
    #define DEBUG_FLUSH() Serial.flush()
#else
    // Disable debug macros (compile to nothing)
    #define DEBUG_BEGIN(baud)
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
    #define DEBUG_FLUSH()
#endif

// Easy to use formatted debug with level prefixes
enum DebugLevel {
    DEBUG_LEVEL_ERROR,
    DEBUG_LEVEL_WARNING,
    DEBUG_LEVEL_INFO,
    DEBUG_LEVEL_DEBUG,
    DEBUG_LEVEL_VERBOSE
};

// Set the maximum debug level that will be displayed
#define CURRENT_DEBUG_LEVEL DEBUG_LEVEL_INFO

#ifdef SERIAL_DEBUG
    #define LOG_ERROR(msg, ...) if(DEBUG_LEVEL_ERROR <= CURRENT_DEBUG_LEVEL) { Serial.print("[ERROR] "); Serial.printf(msg, ##__VA_ARGS__); Serial.println(); }
    #define LOG_WARNING(msg, ...) if(DEBUG_LEVEL_WARNING <= CURRENT_DEBUG_LEVEL) { Serial.print("[WARNING] "); Serial.printf(msg, ##__VA_ARGS__); Serial.println(); }
    #define LOG_INFO(msg, ...) if(DEBUG_LEVEL_INFO <= CURRENT_DEBUG_LEVEL) { Serial.print("[INFO] "); Serial.printf(msg, ##__VA_ARGS__); Serial.println(); }
    #define LOG_DEBUG(msg, ...) if(DEBUG_LEVEL_DEBUG <= CURRENT_DEBUG_LEVEL) { Serial.print("[DEBUG] "); Serial.printf(msg, ##__VA_ARGS__); Serial.println(); }
    #define LOG_VERBOSE(msg, ...) if(DEBUG_LEVEL_VERBOSE <= CURRENT_DEBUG_LEVEL) { Serial.print("[VERBOSE] "); Serial.printf(msg, ##__VA_ARGS__); Serial.println(); }
#else
    #define LOG_ERROR(msg, ...)
    #define LOG_WARNING(msg, ...)
    #define LOG_INFO(msg, ...)
    #define LOG_DEBUG(msg, ...)
    #define LOG_VERBOSE(msg, ...)
#endif

#endif // SERIAL_DEBUG_H