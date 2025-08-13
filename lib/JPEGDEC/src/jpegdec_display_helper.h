//
// JPEGDEC Display Helper for ESP32-S3 TFT Integration
// Optimized for the project's DisplayManager and TFT display
//
#ifndef JPEGDEC_DISPLAY_HELPER_H
#define JPEGDEC_DISPLAY_HELPER_H

#include "JPEGDEC.h"
#include "jpegdec_performance.h"
#include "../DisplayManager/src/display_manager.h"
#include "SerialDebug.h"

class JPEGDisplayHelper {
private:
    static JPEGDEC jpeg;
    static DisplayManager* displayManager;
    static int16_t displayX, displayY;
    static bool useDoubleBuffer;
    static uint16_t* lineBuffer;
    static size_t lineBufferSize;
    
    // Optimized draw callback for TFT display
    static int drawCallback(JPEGDRAW* pDraw) {
        if (!displayManager || !pDraw) return 0;
        
        // Get display mutex with timeout
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
            LOG_ERROR("Failed to acquire display mutex for JPEG draw");
            return 0;
        }
        
        // Calculate actual coordinates
        int x = displayX + pDraw->x;
        int y = displayY + pDraw->y;
        
        // Bounds checking
        if (x >= 240 || y >= 320 || x + pDraw->iWidth <= 0 || y + pDraw->iHeight <= 0) {
            xSemaphoreGive(displayMutex);
            return 1; // Continue decoding but skip drawing
        }
        
        // Clip to display bounds
        int drawWidth = min(pDraw->iWidth, 240 - x);
        int drawHeight = min(pDraw->iHeight, 320 - y);
        
        if (drawWidth > 0 && drawHeight > 0) {
            // Use optimized bulk pixel transfer
            displayManager->getTft()->setAddrWindow(x, y, drawWidth, drawHeight);
            displayManager->getTft()->pushPixels(pDraw->pPixels, drawWidth * drawHeight);
        }
        
        xSemaphoreGive(displayMutex);
        return 1; // Continue
    }
    
public:
    // Initialize with display manager
    static bool initialize(DisplayManager* dm) {
        displayManager = dm;
        displayX = 0;
        displayY = 0;
        useDoubleBuffer = false;
        
        // Allocate line buffer for smooth rendering
        lineBufferSize = 240 * 16 * sizeof(uint16_t); // 16 lines buffer
        lineBuffer = (uint16_t*)JPEG_ALLOC_ALIGNED(lineBufferSize);
        
        DEBUG_PRINTLN("JPEG Display Helper initialized");
        return lineBuffer != nullptr;
    }
    
    // Cleanup resources
    static void cleanup() {
        if (lineBuffer) {
            JPEG_FREE_ALIGNED(lineBuffer);
            lineBuffer = nullptr;
        }
    }
    
    // Set position for next JPEG decode
    static void setPosition(int16_t x, int16_t y) {
        displayX = x;
        displayY = y;
    }
    
    // Decode and display JPEG from memory
    static bool decodeToDisplay(const uint8_t* jpegData, size_t dataSize, 
                               int16_t x = 0, int16_t y = 0) {
        if (!displayManager || !jpegData || dataSize == 0) {
            LOG_ERROR("Invalid parameters for JPEG decode");
            return false;
        }
        
        JPEG_PERF_START();
        
        setPosition(x, y);
        
        // Open JPEG from memory
        if (jpeg.openRAM((uint8_t*)jpegData, dataSize, drawCallback) != 1) {
            LOG_ERROR("Failed to open JPEG from memory");
            return false;
        }
        
        // Set optimal pixel format for TFT (RGB565 little endian)
        jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
        
        // Get image info
        int width = jpeg.getWidth();
        int height = jpeg.getHeight();
        
        DEBUG_PRINTF("JPEG: %dx%d pixels\n", width, height);
        
        // Decode with optimal options
        int decodeOptions = 0;
        
        // Enable auto-rotation if needed
        if (jpeg.getOrientation() > 1) {
            decodeOptions |= JPEG_AUTO_ROTATE;
        }
        
        // Scale down large images to fit display
        if (width > 240 || height > 320) {
            if (width > 480 || height > 640) {
                decodeOptions |= JPEG_SCALE_QUARTER;
            } else {
                decodeOptions |= JPEG_SCALE_HALF;
            }
        }
        
        // Perform decode
        bool success = jpeg.decode(x, y, decodeOptions) == 1;
        
        jpeg.close();
        
        JPEG_PERF_END(dataSize);
        
        if (!success) {
            LOG_ERROR("JPEG decode failed");
        }
        
        return success;
    }
    
    // Decode JPEG from SPIFFS file
    static bool decodeFileToDisplay(const char* filename, int16_t x = 0, int16_t y = 0) {
        if (!displayManager || !filename) {
            LOG_ERROR("Invalid parameters for file JPEG decode");
            return false;
        }
        
        File file = SPIFFS.open(filename, "r");
        if (!file) {
            LOG_ERROR("Failed to open JPEG file: %s", filename);
            return false;
        }
        
        size_t fileSize = file.size();
        if (fileSize == 0 || fileSize > 512 * 1024) {
            LOG_ERROR("Invalid JPEG file size: %u", fileSize);
            file.close();
            return false;
        }
        
        // Allocate buffer for file data
        uint8_t* buffer = (uint8_t*)JPEG_ALLOC_ALIGNED(fileSize);
        if (!buffer) {
            LOG_ERROR("Failed to allocate buffer for JPEG file");
            file.close();
            return false;
        }
        
        // Read file data
        size_t bytesRead = file.readBytes((char*)buffer, fileSize);
        file.close();
        
        if (bytesRead != fileSize) {
            LOG_ERROR("Failed to read complete JPEG file");
            JPEG_FREE_ALIGNED(buffer);
            return false;
        }
        
        // Decode to display
        bool success = decodeToDisplay(buffer, fileSize, x, y);
        
        JPEG_FREE_ALIGNED(buffer);
        return success;
    }
    
    // Get performance statistics
    static void printPerformanceStats() {
        JPEG_PERF_STATS();
    }
};

// Static variable definitions
JPEGDEC JPEGDisplayHelper::jpeg;
DisplayManager* JPEGDisplayHelper::displayManager = nullptr;
int16_t JPEGDisplayHelper::displayX = 0;
int16_t JPEGDisplayHelper::displayY = 0;
bool JPEGDisplayHelper::useDoubleBuffer = false;
uint16_t* JPEGDisplayHelper::lineBuffer = nullptr;
size_t JPEGDisplayHelper::lineBufferSize = 0;

#endif // JPEGDEC_DISPLAY_HELPER_H
