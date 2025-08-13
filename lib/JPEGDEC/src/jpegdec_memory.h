//
// ESP32-S3 Optimized Memory Management for JPEGDEC
// Provides PSRAM-aware allocation and 16-byte alignment for SIMD
//
#ifndef JPEGDEC_MEMORY_H
#define JPEGDEC_MEMORY_H

#include <Arduino.h>
#include "esp_heap_caps.h"

// Memory allocation strategy for ESP32-S3
class JPEGMemoryManager {
public:
    // Allocate aligned memory for SIMD operations
    static void* allocateAligned(size_t size, size_t alignment = 16) {
        void* ptr = nullptr;
        
        #ifdef BOARD_HAS_PSRAM
        // Try PSRAM first for large allocations
        if (size > 8192) {
            ptr = heap_caps_aligned_alloc(alignment, size, MALLOC_CAP_SPIRAM);
        }
        #endif
        
        // Fall back to internal RAM
        if (!ptr) {
            ptr = heap_caps_aligned_alloc(alignment, size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        }
        
        return ptr;
    }
    
    // Free aligned memory
    static void freeAligned(void* ptr) {
        if (ptr) {
            heap_caps_free(ptr);
        }
    }
    
    // Get optimal buffer size based on available memory
    static size_t getOptimalBufferSize(size_t requested) {
        size_t available = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        
        // Reserve 20KB for system operations
        if (available < 20480) {
            return 512;  // Minimal buffer
        } else if (available < 51200) {
            return 1024; // Reduced buffer
        } else {
            return requested; // Full requested size
        }
    }
    
    // Check if PSRAM is available and recommend usage
    static bool shouldUsePSRAM(size_t size) {
        #ifdef BOARD_HAS_PSRAM
        return size > 4096 && heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > size;
        #else
        return false;
        #endif
    }
};

// Optimized buffer allocation macros
#define JPEG_ALLOC_ALIGNED(size) JPEGMemoryManager::allocateAligned(size)
#define JPEG_FREE_ALIGNED(ptr) JPEGMemoryManager::freeAligned(ptr)
#define JPEG_OPTIMAL_BUFFER_SIZE(size) JPEGMemoryManager::getOptimalBufferSize(size)

#endif // JPEGDEC_MEMORY_H
