//
// JPEGDEC Performance Monitor for ESP32-S3
// Provides decode time and memory usage statistics
//
#ifndef JPEGDEC_PERFORMANCE_H
#define JPEGDEC_PERFORMANCE_H

#include <Arduino.h>
#include "SerialDebug.h"

class JPEGPerformanceMonitor {
private:
    static uint32_t decodeStartTime;
    static uint32_t totalDecodeTime;
    static uint32_t decodeCount;
    static uint32_t maxDecodeTime;
    static uint32_t minDecodeTime;
    static size_t maxMemoryUsed;
    
public:
    // Start timing a decode operation
    static void startDecode() {
        decodeStartTime = millis();
    }
    
    // End timing and update statistics
    static void endDecode(size_t memoryUsed = 0) {
        uint32_t decodeTime = millis() - decodeStartTime;
        
        totalDecodeTime += decodeTime;
        decodeCount++;
        
        if (decodeTime > maxDecodeTime) {
            maxDecodeTime = decodeTime;
        }
        
        if (minDecodeTime == 0 || decodeTime < minDecodeTime) {
            minDecodeTime = decodeTime;
        }
        
        if (memoryUsed > maxMemoryUsed) {
            maxMemoryUsed = memoryUsed;
        }
        
        DEBUG_PRINTF("JPEG decode: %ums, memory: %u bytes\n", decodeTime, memoryUsed);
    }
    
    // Get average decode time
    static uint32_t getAverageDecodeTime() {
        return decodeCount > 0 ? totalDecodeTime / decodeCount : 0;
    }
    
    // Print performance statistics
    static void printStats() {
        if (decodeCount == 0) return;
        
        DEBUG_PRINTLN("=== JPEG Performance Statistics ===");
        DEBUG_PRINTF("Total decodes: %u\n", decodeCount);
        DEBUG_PRINTF("Average time: %ums\n", getAverageDecodeTime());
        DEBUG_PRINTF("Min time: %ums\n", minDecodeTime);
        DEBUG_PRINTF("Max time: %ums\n", maxDecodeTime);
        DEBUG_PRINTF("Max memory: %u bytes\n", maxMemoryUsed);
        DEBUG_PRINTF("Free heap: %u bytes\n", ESP.getFreeHeap());
        #ifdef BOARD_HAS_PSRAM
        DEBUG_PRINTF("Free PSRAM: %u bytes\n", ESP.getFreePsram());
        #endif
    }
    
    // Reset statistics
    static void resetStats() {
        decodeStartTime = 0;
        totalDecodeTime = 0;
        decodeCount = 0;
        maxDecodeTime = 0;
        minDecodeTime = 0;
        maxMemoryUsed = 0;
    }
    
    // Check if decode time is acceptable for real-time display
    static bool isPerformanceAcceptable(uint32_t maxAcceptableMs = 100) {
        return getAverageDecodeTime() <= maxAcceptableMs;
    }
};

// Convenience macros for performance monitoring
#ifdef SERIAL_DEBUG
#define JPEG_PERF_START() JPEGPerformanceMonitor::startDecode()
#define JPEG_PERF_END(mem) JPEGPerformanceMonitor::endDecode(mem)
#define JPEG_PERF_STATS() JPEGPerformanceMonitor::printStats()
#else
#define JPEG_PERF_START()
#define JPEG_PERF_END(mem)
#define JPEG_PERF_STATS()
#endif

#endif // JPEGDEC_PERFORMANCE_H
