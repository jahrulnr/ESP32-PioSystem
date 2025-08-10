/*
 * Custom SPI Pins Example
 * 
 * This example demonstrates how to use the FileManager library with custom SPI pins
 * for SD card communication. This is useful when the default SPI pins are occupied
 * by other peripherals or when using custom PCB layouts.
 * 
 * Features:
 * - Custom SPI pin configuration for SD card
 * - Multiple constructor options
 * - SPI pin validation and reporting
 * - SD card type detection and information
 * - Performance comparison between different pin configurations
 * 
 * Hardware Requirements:
 * - ESP32-S3 development board
 * - SD card module with configurable SPI pins
 * - MicroSD card (any size, formatted as FAT32)
 * 
 * Example Custom SPI Connections:
 * ESP32-S3    SD Card Module
 * --------    --------------
 * GPIO 14  -> SCK (Serial Clock)
 * GPIO 12  -> MISO (Master In Slave Out)  
 * GPIO 13  -> MOSI (Master Out Slave In)
 * GPIO 15  -> CS (Chip Select)
 * 3.3V     -> VCC
 * GND      -> GND
 * 
 * Author: Your Name
 * Date: 2025
 */

#include <Arduino.h>
#include "file_manager.h"

// Test different SPI configurations
void testDefaultSPI();
void testCustomSPIStruct();
void testCustomSPIParams();
void performanceTest(FileManager* fm, const String& testName);
void demonstrateSPIInfo(FileManager* fm);

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== FileManager Custom SPI Pins Demo ===");
    Serial.println();
    
    // Test 1: Default SPI pins (GPIO 18, 19, 23, 5)
    Serial.println("🔧 Test 1: Default SPI Configuration");
    testDefaultSPI();
    
    delay(2000);
    
    // Test 2: Custom SPI using struct
    Serial.println("\\n🔧 Test 2: Custom SPI using SPIPins struct");
    testCustomSPIStruct();
    
    delay(2000);
    
    // Test 3: Custom SPI using individual parameters
    Serial.println("\\n🔧 Test 3: Custom SPI using individual parameters");
    testCustomSPIParams();
    
    Serial.println("\\n🎉 All SPI configuration tests completed!");
}

void loop() {
    // Nothing to do in loop
    delay(1000);
}

void testDefaultSPI() {
    Serial.println("Using default SPI pins (SCK:18, MISO:19, MOSI:23, CS:5)");
    
    // Default constructor uses standard ESP32 SPI pins
    FileManager* defaultFM = new FileManager(5); // Only CS pin specified
    
    if (!defaultFM->init()) {
        Serial.println("❌ Default SPI initialization failed");
        delete defaultFM;
        return;
    }
    
    Serial.println("✅ Default SPI initialized successfully");
    demonstrateSPIInfo(defaultFM);
    
    if (defaultFM->isSDAvailable()) {
        performanceTest(defaultFM, "Default SPI");
    }
    
    delete defaultFM;
}

void testCustomSPIStruct() {
    Serial.println("Using custom SPI pins via SPIPins struct");
    
    // Define custom SPI pins using struct
    SPIPins customPins;
    customPins.sck = 14;   // Custom SCK pin
    customPins.miso = 12;  // Custom MISO pin  
    customPins.mosi = 13;  // Custom MOSI pin
    customPins.cs = 15;    // Custom CS pin
    
    Serial.printf("Custom pins - SCK:%d, MISO:%d, MOSI:%d, CS:%d\\n", 
                  customPins.sck, customPins.miso, customPins.mosi, customPins.cs);
    
    FileManager* customFM = new FileManager(customPins);
    
    if (!customFM->init()) {
        Serial.println("❌ Custom SPI (struct) initialization failed");
        Serial.println("ℹ️  This is normal if SD card is not connected to custom pins");
        delete customFM;
        return;
    }
    
    Serial.println("✅ Custom SPI (struct) initialized successfully");
    demonstrateSPIInfo(customFM);
    
    if (customFM->isSDAvailable()) {
        performanceTest(customFM, "Custom SPI (Struct)");
    }
    
    delete customFM;
}

void testCustomSPIParams() {
    Serial.println("Using custom SPI pins via individual parameters");
    
    // Define custom SPI pins using individual parameters
    int sck = 21;   // Custom SCK pin
    int miso = 22;  // Custom MISO pin
    int mosi = 4;   // Custom MOSI pin  
    int cs = 2;     // Custom CS pin
    
    Serial.printf("Custom pins - SCK:%d, MISO:%d, MOSI:%d, CS:%d\\n", sck, miso, mosi, cs);
    
    FileManager* paramFM = new FileManager(sck, miso, mosi, cs);
    
    if (!paramFM->init()) {
        Serial.println("❌ Custom SPI (params) initialization failed");
        Serial.println("ℹ️  This is normal if SD card is not connected to custom pins");
        delete paramFM;
        return;
    }
    
    Serial.println("✅ Custom SPI (params) initialized successfully");
    demonstrateSPIInfo(paramFM);
    
    if (paramFM->isSDAvailable()) {
        performanceTest(paramFM, "Custom SPI (Params)");
    }
    
    delete paramFM;
}

void demonstrateSPIInfo(FileManager* fm) {
    Serial.println("\\n📊 SPI Configuration Info:");
    Serial.println("==========================");
    
    // Get current SPI pin configuration
    SPIPins pins = fm->getSPIPins();
    Serial.printf("SCK  (Clock):     GPIO %d\\n", pins.sck);
    Serial.printf("MISO (Data In):   GPIO %d\\n", pins.miso);
    Serial.printf("MOSI (Data Out):  GPIO %d\\n", pins.mosi);
    Serial.printf("CS   (Chip Sel):  GPIO %d\\n", pins.cs);
    
    // Show file system status
    Serial.println("\\n💾 Storage Status:");
    if (fm->isLittleFSAvailable()) {
        Serial.printf("✅ LittleFS: %u/%u bytes\\n", 
                      fm->getUsedBytes(FS_LITTLEFS), 
                      fm->getTotalBytes(FS_LITTLEFS));
    }
    
    if (fm->isSDAvailable()) {
        uint64_t totalMB = fm->getTotalBytes(FS_SD_CARD) / (1024 * 1024);
        uint64_t usedMB = fm->getUsedBytes(FS_SD_CARD) / (1024 * 1024);
        Serial.printf("✅ SD Card: %lluMB/%lluMB\\n", usedMB, totalMB);
    } else {
        Serial.println("❌ SD Card: Not available");
    }
}

void performanceTest(FileManager* fm, const String& testName) {
    Serial.println("\\n⚡ Performance Test: " + testName);
    Serial.println("=====================================");
    
    const String testFile = "/performance_test.txt";
    const String testData = "This is a performance test data string that will be written multiple times to test the speed of file operations.\\n";
    const int iterations = 100;
    
    // Write test
    unsigned long startTime = millis();
    
    fm->createFile(testFile, "", FS_SD_CARD); // Create empty file
    
    for (int i = 0; i < iterations; i++) {
        String data = String(i) + ": " + testData;
        fm->appendFile(testFile, data, FS_SD_CARD);
    }
    
    unsigned long writeTime = millis() - startTime;
    
    // Read test
    startTime = millis();
    String content = fm->readFile(testFile, FS_SD_CARD);
    unsigned long readTime = millis() - startTime;
    
    // Calculate performance metrics
    size_t fileSize = content.length();
    float writeSpeed = (fileSize / 1024.0) / (writeTime / 1000.0); // KB/s
    float readSpeed = (fileSize / 1024.0) / (readTime / 1000.0);   // KB/s
    
    Serial.printf("File size: %u bytes (%.2f KB)\\n", fileSize, fileSize / 1024.0);
    Serial.printf("Write time: %lu ms (%.2f KB/s)\\n", writeTime, writeSpeed);
    Serial.printf("Read time: %lu ms (%.2f KB/s)\\n", readTime, readSpeed);
    
    // Cleanup
    fm->deleteFile(testFile, FS_SD_CARD);
    
    Serial.println("✅ Performance test completed");
}

/*
 * Additional SPI Configuration Examples:
 * 
 * 1. For custom PCB with specific routing:
 *    SPIPins pcbPins = {.sck = 25, .miso = 26, .mosi = 27, .cs = 32};
 *    FileManager fm(pcbPins);
 * 
 * 2. When using multiple SPI devices:
 *    FileManager fm(14, 12, 13, 15); // SD card on custom pins
 *    // Other SPI device can use default pins
 * 
 * 3. For debugging SPI issues:
 *    SPIPins debugPins = fm.getSPIPins();
 *    Serial.printf("Current SPI: SCK=%d, MISO=%d, MOSI=%d, CS=%d\\n",
 *                  debugPins.sck, debugPins.miso, debugPins.mosi, debugPins.cs);
 * 
 * Pin Selection Guidelines:
 * - Avoid pins used by other peripherals (UART, I2C, etc.)
 * - Use GPIO pins that support output (for SCK, MOSI, CS)
 * - Use GPIO pins that support input (for MISO)
 * - Avoid strapping pins (GPIO 0, 2, 15, etc.) if possible
 * - Keep SPI traces short for better signal integrity
 * - Use proper pull-up resistors on CS line if needed
 */
