#include "tasks.h"
#include "iot_device_manager.h"
#include "SerialDebug.h"
#include "httpclient.h"
#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>
#include <JPEGDEC.h>

// External global references
extern HttpClientManager* httpClientManager;

// Camera stream task variables
TaskHandle_t cameraStreamTaskHandle = NULL;
static bool cameraStreamActive = false;
static String currentCameraDeviceId = "";
static unsigned long lastCaptureTime = 0;
static const unsigned long CAPTURE_INTERVAL = 500; // 500ms for 2 FPS (faster streaming)

// JPEG display variables
static int jpegDisplayX, jpegDisplayY, jpegDisplayMaxW, jpegDisplayMaxH;

/**
 * @brief JPEG draw callback for JPEGDEC library
 * 
 * @param pDraw JPEGDRAW structure with pixel data
 * @return 1 to continue, 0 to stop
 */
int jpegDrawCallback(JPEGDRAW *pDraw) {
    // CRITICAL: Do NOT call updateActivity() here as it can cause mutex issues
    // The mutex is already held by the caller (displayJpegOnTFT)
    
    // Calculate final draw position (callback coordinates are relative to decode area)
    int drawX = jpegDisplayX + pDraw->x;
    int drawY = jpegDisplayY + pDraw->y;
    
    // Simple bounds checking - only draw if within bounds
    if (drawX >= 0 && drawY >= 0 && 
        (drawX + pDraw->iWidth) <= (jpegDisplayX + jpegDisplayMaxW) &&
        (drawY + pDraw->iHeight) <= (jpegDisplayY + jpegDisplayMaxH)) {
        
        // Direct draw without clipping for speed
        // The mutex is already held by the calling function
        displayManager.getTFT()->pushImage(drawX, drawY, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
    }
    
    return 1; // Continue decoding
}/**
 * @brief Capture JPEG binary data from camera device
 * 
 * @param device Camera device to capture from
 * @param jpegData Reference to store JPEG binary data
 * @param jpegSize Reference to store JPEG data size
 * @return true if capture was successful
 */
bool captureJpegBinary(const IoTDevice& device, uint8_t*& jpegData, size_t& jpegSize) {
    if (httpClientManager == nullptr) {
        DEBUG_PRINTLN("Camera capture: HttpClientManager not available");
        return false;
    }
    
    String captureUrl = device.baseUrl + "/api/v1/camera/capture";
    DEBUG_PRINTF("Camera capture: Requesting JPEG from %s\n", captureUrl.c_str());
    
    // Make POST request to capture endpoint
    HttpResponse response = httpClientManager->post(captureUrl, "", "application/json");
    
    if (response.statusCode != 200) {
        DEBUG_PRINTF("Camera capture: HTTP error %d\n", response.statusCode);
        return false;
    }
    
    // Check if response is JPEG binary data
    auto contentTypeIt = response.headers.find("Content-Type");
    // if (contentTypeIt != response.headers.end() && 
    //     contentTypeIt->second.indexOf("jpeg") >= 0) {
		if (response.body.length() > 0) {
        
        // Response body contains raw JPEG binary data
        jpegSize = response.body.length();
        if (jpegSize > 0) {
            jpegData = new uint8_t[jpegSize];
            memcpy(jpegData, response.body.c_str(), jpegSize);
            
            DEBUG_PRINTF("Camera capture: Received JPEG binary data (%d bytes)\n", jpegSize);
            return true;
        }
    } else {
        DEBUG_PRINTLN("Camera capture: Response is not JPEG binary data");
        // Try to parse as JSON error response
        JsonDocument errorDoc;
        DeserializationError error = deserializeJson(errorDoc, response.body);
        if (!error && errorDoc["success"].as<bool>() == false) {
            DEBUG_PRINTF("Camera capture: Error - %s\n", errorDoc["message"].as<String>().c_str());
        }
    }
    
    return false;
}

/**
 * @brief Simple JPEG header validation
 * 
 * @param data JPEG binary data
 * @param size Size of data
 * @return true if data appears to be valid JPEG
 */
bool isValidJpeg(const uint8_t* data, size_t size) {
    if (size < 4) return false;
    
    // Check JPEG magic bytes (SOI marker: 0xFFD8)
    return (data[0] == 0xFF && data[1] == 0xD8);
}

/**
 * @brief Display JPEG data on TFT screen using JPEGDEC library
 * 
 * @param jpegData JPEG binary data
 * @param jpegSize Size of JPEG data
 * @param x X position on screen
 * @param y Y position on screen
 * @param maxWidth Maximum width for display
 * @param maxHeight Maximum height for display
 * @return true if display was successful
 */
bool displayJpegOnTFT(const uint8_t* jpegData, size_t jpegSize, 
                      int x, int y, int maxWidth, int maxHeight) {
    if (!isValidJpeg(jpegData, jpegSize)) {
        DEBUG_PRINTLN("Display JPEG: Invalid JPEG data");
        return false;
    }
    
    // Create JPEGDEC instance
    JPEGDEC jpeg;
    
    // Open JPEG from RAM
    int result = jpeg.openRAM((uint8_t*)jpegData, jpegSize, jpegDrawCallback);
    if (result != 1) {
        DEBUG_PRINTF("Display JPEG: Failed to open JPEG, error: %d\n", jpeg.getLastError());
        return false;
    }
    
    // Get image dimensions
    int imageWidth = jpeg.getWidth();
    int imageHeight = jpeg.getHeight();
    
    // Calculate scale and positioning for centering
    int decodeOptions = 0;
    int displayWidth = imageWidth;
    int displayHeight = imageHeight;
    
    // Scale down if image is too large
    if (imageWidth > maxWidth || imageHeight > maxHeight) {
        if (imageWidth > maxWidth * 4 || imageHeight > maxHeight * 4) {
            decodeOptions = JPEG_SCALE_EIGHTH;
            displayWidth = imageWidth / 8;
            displayHeight = imageHeight / 8;
        } else if (imageWidth > maxWidth * 2 || imageHeight > maxHeight * 2) {
            decodeOptions = JPEG_SCALE_QUARTER;
            displayWidth = imageWidth / 4;
            displayHeight = imageHeight / 4;
        } else {
            decodeOptions = JPEG_SCALE_HALF;
            displayWidth = imageWidth / 2;
            displayHeight = imageHeight / 2;
        }
    }
    
    // Center the image in the display area
    int offsetX = (maxWidth - displayWidth) / 2;
    int offsetY = (maxHeight - displayHeight) / 2;
    if (offsetX < 0) offsetX = 0;
    if (offsetY < 0) offsetY = 0;
    
    // Set global variables for the callback function (with proper centering)
    jpegDisplayX = x + offsetX;
    jpegDisplayY = y + offsetY;
    jpegDisplayMaxW = displayWidth;  // Use actual display width, not max
    jpegDisplayMaxH = displayHeight; // Use actual display height, not max
    
    // Take mutex for entire decode operation (single mutex operation for speed)
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        DEBUG_PRINTLN("Display JPEG: Failed to take display mutex for decode");
        jpeg.close();
        return false;
    }
    
    // Clear only the area we'll use (more efficient)
    displayManager.getTFT()->fillRect(x, y, maxWidth, maxHeight, TFT_BLACK);
    
    // Decode and display the JPEG directly
    result = jpeg.decode(0, 0, decodeOptions);  // Use 0,0 since we handle offset in callback
    
    // Release mutex after decode
    xSemaphoreGive(displayMutex);
    
    // Close the JPEG decoder
    jpeg.close();
    
    if (result != 1) {
        DEBUG_PRINTF("Display JPEG: Failed to decode JPEG, error: %d\n", jpeg.getLastError());
        
        // Show error message on display
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            displayManager.getTFT()->fillRect(x, y, maxWidth, maxHeight, TFT_BLACK);
            displayManager.getTFT()->drawRect(x, y, maxWidth, maxHeight, TFT_RED);
            displayManager.getTFT()->setTextColor(TFT_RED);
            displayManager.getTFT()->drawString("JPEG Error", x + 5, y + 5, 1);
            xSemaphoreGive(displayMutex);
        }
        return false;
    }
    
    return true;
}

/**
 * @brief Start camera streaming from selected device
 * 
 * @param deviceId Device ID to stream from
 */
void startCameraStream(const String& deviceId) {
    if (cameraStreamActive) {
        stopCameraStream();
    }
    
    currentCameraDeviceId = deviceId;
    cameraStreamActive = true;
    lastCaptureTime = 0;
    
    DEBUG_PRINTF("Camera stream: Starting stream from device %s\n", deviceId.c_str());
    
    // Create camera stream task on Core 1 (same as display tasks)
    xTaskCreatePinnedToCore(
        cameraStreamTask,
        "camera_stream",
        1024 * 24,  // Increased stack for JPEG processing
        nullptr,
        10, // Higher priority for smooth streaming
        &cameraStreamTaskHandle,
        1   // Pin to Core 1 for display operations
    );
}

/**
 * @brief Stop camera streaming
 */
void stopCameraStream() {
    if (!cameraStreamActive) return;
    
    DEBUG_PRINTLN("Camera stream: Stopping stream");
    
    cameraStreamActive = false;
    currentCameraDeviceId = "";
    
    if (cameraStreamTaskHandle != nullptr) {
        vTaskDelete(cameraStreamTaskHandle);
        cameraStreamTaskHandle = nullptr;
    }
}

/**
 * @brief Check if camera stream is currently active
 */
bool isCameraStreamActive() {
    return cameraStreamActive;
}

/**
 * @brief Get the ID of the currently streaming device
 * 
 * @return Device ID string, empty if not streaming
 */
String getCurrentStreamingDevice() {
    return currentCameraDeviceId;
}

/**
 * @brief FreeRTOS task for camera streaming
 * 
 * @param parameter Task parameter (unused)
 */
void cameraStreamTask(void* parameter) {
    DEBUG_PRINTLN("Camera stream task: Started");
    
    while (cameraStreamActive && iotDeviceManager != nullptr) {
        unsigned long currentTime = millis();
        
        // Check if it's time for next capture
        if (currentTime - lastCaptureTime >= CAPTURE_INTERVAL) {
            // Get current device
            IoTDevice* device = iotDeviceManager->getDevice(currentCameraDeviceId);
            
            if (device == nullptr || !device->isOnline) {
                DEBUG_PRINTF("Camera stream: Device %s not available\n", currentCameraDeviceId.c_str());
                break;
            }
            
            // Check if device still has camera capability
            if (!(device->capabilities & static_cast<uint32_t>(DeviceCapability::CAMERA))) {
                DEBUG_PRINTF("Camera stream: Device %s lost camera capability\n", currentCameraDeviceId.c_str());
                break;
            }
            
            // Capture JPEG binary data directly from camera API
            uint8_t* jpegData = nullptr;
            size_t jpegSize = 0;
            
            bool captureSuccess = captureJpegBinary(*device, jpegData, jpegSize);
            
            if (captureSuccess && jpegData != nullptr) {
                // Display JPEG on TFT screen (displayJpegOnTFT will handle the mutex)
                displayJpegOnTFT(jpegData, jpegSize, 10, 70, 220, 120);
                
                // Update activity separately AFTER the display operation
                DisplayManager* display = DisplayManager::getInstance();
                if (display != nullptr) {
                    display->updateActivity();
                }
                
                // Update display status with mutex
                if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    updateCameraStreamDisplay(true);
                    xSemaphoreGive(displayMutex);
                }
                
                // Clean up JPEG data
                delete[] jpegData;
                jpegData = nullptr;
                
            } else {
                DEBUG_PRINTLN("Camera stream: JPEG capture failed");
                
                // Update display with error
                if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    updateCameraStreamDisplay(false, "JPEG capture failed");
                    xSemaphoreGive(displayMutex);
                }
            }
            
            lastCaptureTime = currentTime;
        }
        
        // Sleep for a shorter time for more responsive streaming
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    DEBUG_PRINTLN("Camera stream task: Ended");
    cameraStreamActive = false;
    vTaskDelete(NULL);
}

/**
 * @brief Update camera stream display
 * 
 * @param success Whether the last capture was successful
 * @param errorMsg Error message if capture failed
 */
void updateCameraStreamDisplay(bool success, const String& errorMsg) {
    if (currentMenu != MENU_CAMERA_STREAM) return;
    
    // Only update the stream area, not the whole screen
    int streamY = 70;
    int streamH = 120;
    
    // Clear stream area
    // displayManager.getTFT()->fillRect(streamX + 1, streamY + 1, streamW - 2, streamH - 2, TFT_BLACK);
    
    if (!success) {
        // Show error state
        displayManager.drawCenteredText("ERROR", streamY + streamH/2 - 10, TFT_RED, 2);
        if (errorMsg.length() > 0) {
            String shortError = errorMsg;
            if (shortError.length() > 20) {
                shortError = shortError.substring(0, 17) + "...";
            }
            displayManager.drawCenteredText(shortError, streamY + streamH/2 + 5, TFT_YELLOW, 1);
        }
        displayManager.drawCenteredText("Connection Lost", streamY + streamH/2 + 20, TFT_LIGHTGREY, 1);
    }
}

/**
 * @brief Manually trigger a camera capture
 * 
 * @return true if capture was successful
 */
bool triggerManualCapture() {
    if (!cameraStreamActive || iotDeviceManager == nullptr) {
        return false;
    }
    
    IoTDevice* device = iotDeviceManager->getDevice(currentCameraDeviceId);
    if (device == nullptr || !device->isOnline) {
        return false;
    }
    
    DEBUG_PRINTF("Camera stream: Manual capture from device %s\n", currentCameraDeviceId.c_str());
    
    // Use new JPEG binary capture
    uint8_t* jpegData = nullptr;
    size_t jpegSize = 0;
    
    bool success = captureJpegBinary(*device, jpegData, jpegSize);
    
    if (success && jpegData != nullptr) {
        DEBUG_PRINTF("Camera stream: Manual JPEG captured (%d bytes)\n", jpegSize);
        
        // Display immediately (displayJpegOnTFT will handle the mutex)
        displayJpegOnTFT(jpegData, jpegSize, 10, 70, 220, 120);
        
        // Update activity separately AFTER the display operation
        DisplayManager* display = DisplayManager::getInstance();
        if (display != nullptr) {
            display->updateActivity();
        }
        
        // Update display status with mutex
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            updateCameraStreamDisplay(true);
            xSemaphoreGive(displayMutex);
        }
        
        // Clean up
        delete[] jpegData;
        jpegData = nullptr;
    } else {
        DEBUG_PRINTLN("Camera stream: Manual JPEG capture failed");
    }
    
    return success;
}
