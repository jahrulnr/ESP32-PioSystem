//
// JPEGDEC Optimization Configuration for ESP32-S3 PioSystem
// Optimized for memory usage and performance
//
#ifndef JPEGDEC_CONFIG_H
#define JPEGDEC_CONFIG_H

// ESP32-S3 specific optimizations
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)
    // Enable ESP32-S3 SIMD optimizations
    #ifndef ESP32S3_SIMD
    #define ESP32S3_SIMD
    #endif
    
#endif

#define JPEG_FILE_BUF_SIZE 2048 
#define MAX_BUFFERED_PIXELS 2048  
#define DC_TABLE_SIZE 1024      

#endif // JPEGDEC_CONFIG_H
