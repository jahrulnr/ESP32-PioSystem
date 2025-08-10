/*
 * Basic File Operations Example
 * 
 * This example demonstrates fundamental file operations using the FileManager library.
 * It shows how to work with both LittleFS (internal flash) and SD card storage.
 * 
 * Features:
 * - File creation, reading, writing, and deletion
 * - Directory operations
 * - File system information
 * - Cross-filesystem file copying
 * - Storage space monitoring
 * 
 * Hardware Requirements:
 * - ESP32 development board
 * - Optional: SD card module and microSD card
 * 
 * SD Card Connections (if used):
 * - CS:   GPIO 5 (configurable)
 * - MOSI: GPIO 23
 * - MISO: GPIO 19
 * - CLK:  GPIO 18
 * 
 * Author: Your Name
 * Date: 2025
 */

#include <Arduino.h>
#include "file_manager.h"

// Create FileManager instance
FileManager* fileMgr;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== FileManager Basic Operations Demo ===");
    Serial.println();
    
    // Initialize FileManager (SD CS pin = 5)
    fileMgr = new FileManager(5);
    
    if (!fileMgr->init()) {
        Serial.println("❌ FileManager initialization failed!");
        return;
    }
    
    Serial.println("✅ FileManager initialized successfully!");
    
    // Show available file systems
    showFileSystemStatus();
    
    // Run demonstrations
    demonstrateBasicOperations();
    demonstrateDirectoryOperations();
    demonstrateStorageInfo();
    
    if (fileMgr->isSDAvailable()) {
        demonstrateSDOperations();
        demonstrateCrossFilesystemCopy();
    }
    
    Serial.println("\n🎉 Demo completed! Check the files created.");
}

void loop() {
    // Monitor storage space every 10 seconds
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck >= 10000) {
        monitorStorage();
        lastCheck = millis();
    }
    
    delay(1000);
}

void showFileSystemStatus() {
    Serial.println("📊 File System Status:");
    Serial.println("======================");
    
    if (fileMgr->isLittleFSAvailable()) {
        Serial.println("✅ LittleFS: Available");
        Serial.printf("   Total: %u bytes\n", fileMgr->getTotalBytes(FS_LITTLEFS));
        Serial.printf("   Used:  %u bytes\n", fileMgr->getUsedBytes(FS_LITTLEFS));
        Serial.printf("   Free:  %u bytes\n", fileMgr->getFreeBytes(FS_LITTLEFS));
    } else {
        Serial.println("❌ LittleFS: Not available");
    }
    
    if (fileMgr->isSDAvailable()) {
        Serial.println("✅ SD Card: Available");
        Serial.printf("   Total: %u bytes\n", fileMgr->getTotalBytes(FS_SD_CARD));
        Serial.printf("   Used:  %u bytes\n", fileMgr->getUsedBytes(FS_SD_CARD));
        Serial.printf("   Free:  %u bytes\n", fileMgr->getFreeBytes(FS_SD_CARD));
    } else {
        Serial.println("❌ SD Card: Not available");
    }
    
    Serial.println();
}

void demonstrateBasicOperations() {
    Serial.println("📝 Basic File Operations Demo:");
    Serial.println("==============================");
    
    // Create a test file
    String testContent = "Hello, FileManager!\\nThis is a test file.\\nTimestamp: " + String(millis());
    
    if (fileMgr->createFile("/test.txt", testContent)) {
        Serial.println("✅ Created test.txt");
    } else {
        Serial.println("❌ Failed to create test.txt");
        return;
    }
    
    // Check if file exists
    if (fileMgr->exists("/test.txt")) {
        Serial.println("✅ File exists confirmed");
    }
    
    // Read the file
    String readContent = fileMgr->readFile("/test.txt");
    if (readContent.length() > 0) {
        Serial.println("✅ File read successfully:");
        Serial.println("   Content: " + readContent);
    } else {
        Serial.println("❌ Failed to read file");
    }
    
    // Append to the file
    String appendText = "\\nAppended line at: " + String(millis());
    if (fileMgr->appendFile("/test.txt", appendText)) {
        Serial.println("✅ Appended to file");
        
        // Read again to show appended content
        String newContent = fileMgr->readFile("/test.txt");
        Serial.println("   Updated content: " + newContent);
    }
    
    // Write new content (overwrite)
    String newContent = "Overwritten content at: " + String(millis());
    if (fileMgr->writeFile("/test.txt", newContent)) {
        Serial.println("✅ File overwritten");
        
        String finalContent = fileMgr->readFile("/test.txt");
        Serial.println("   Final content: " + finalContent);
    }
    
    Serial.println();
}

void demonstrateDirectoryOperations() {
    Serial.println("📁 Directory Operations Demo:");
    Serial.println("=============================");
    
    // Create directories
    if (fileMgr->createDir("/data")) {
        Serial.println("✅ Created /data directory");
    }
    
    if (fileMgr->createDir("/data/logs")) {
        Serial.println("✅ Created /data/logs directory");
    }
    
    if (fileMgr->createDir("/data/config")) {
        Serial.println("✅ Created /data/config directory");
    }
    
    // Create some files in directories
    fileMgr->createFile("/data/config/settings.json", "{\"version\": \"1.0\", \"debug\": true}");
    fileMgr->createFile("/data/logs/system.log", "System started at: " + String(millis()));
    fileMgr->createFile("/data/logs/error.log", "No errors yet");
    
    Serial.println("✅ Created test files in directories");
    
    // List directory contents
    Serial.println("\\n📋 Directory listing:");
    listDirectory("/");
    listDirectory("/data");
    listDirectory("/data/logs");
    listDirectory("/data/config");
    
    Serial.println();
}

void demonstrateStorageInfo() {
    Serial.println("💾 Storage Information Demo:");
    Serial.println("============================");
    
    // Show detailed storage info
    size_t total = fileMgr->getTotalBytes(FS_LITTLEFS);
    size_t used = fileMgr->getUsedBytes(FS_LITTLEFS);
    size_t free = fileMgr->getFreeBytes(FS_LITTLEFS);
    
    Serial.printf("LittleFS Storage:\\n");
    Serial.printf("  Total: %u bytes (%.2f KB)\\n", total, total / 1024.0);
    Serial.printf("  Used:  %u bytes (%.2f KB)\\n", used, used / 1024.0);
    Serial.printf("  Free:  %u bytes (%.2f KB)\\n", free, free / 1024.0);
    Serial.printf("  Usage: %.1f%%\\n", (used * 100.0) / total);
    
    // Print file system tree
    Serial.println("\\n🌳 File System Tree:");
    fileMgr->printFileSystem(FS_LITTLEFS);
    
    Serial.println();
}

void demonstrateSDOperations() {
    Serial.println("💳 SD Card Operations Demo:");
    Serial.println("===========================");
    
    // Create SD card test file
    String sdContent = "SD Card test file\\nCreated at: " + String(millis());
    
    if (fileMgr->createFile("/sd_test.txt", sdContent, FS_SD_CARD)) {
        Serial.println("✅ Created SD card test file");
        
        // Read from SD card
        String sdRead = fileMgr->readFile("/sd_test.txt", FS_SD_CARD);
        Serial.println("✅ Read from SD card: " + sdRead);
    } else {
        Serial.println("❌ Failed to create SD card file");
    }
    
    // Create directory on SD card
    if (fileMgr->createDir("/backup", FS_SD_CARD)) {
        Serial.println("✅ Created /backup directory on SD card");
    }
    
    // Show SD card info
    Serial.printf("\\nSD Card Storage:\\n");
    Serial.printf("  Total: %u bytes (%.2f MB)\\n", 
                  fileMgr->getTotalBytes(FS_SD_CARD), 
                  fileMgr->getTotalBytes(FS_SD_CARD) / (1024.0 * 1024.0));
    Serial.printf("  Used:  %u bytes (%.2f MB)\\n", 
                  fileMgr->getUsedBytes(FS_SD_CARD),
                  fileMgr->getUsedBytes(FS_SD_CARD) / (1024.0 * 1024.0));
    Serial.printf("  Free:  %u bytes (%.2f MB)\\n", 
                  fileMgr->getFreeBytes(FS_SD_CARD),
                  fileMgr->getFreeBytes(FS_SD_CARD) / (1024.0 * 1024.0));
    
    Serial.println();
}

void demonstrateCrossFilesystemCopy() {
    Serial.println("🔄 Cross-Filesystem Copy Demo:");
    Serial.println("==============================");
    
    // Copy file from LittleFS to SD card
    if (fileMgr->copyFile("/test.txt", "/backup/test_backup.txt", FS_LITTLEFS, FS_SD_CARD)) {
        Serial.println("✅ Copied file from LittleFS to SD card");
        
        // Verify the copy
        String originalContent = fileMgr->readFile("/test.txt", FS_LITTLEFS);
        String copiedContent = fileMgr->readFile("/backup/test_backup.txt", FS_SD_CARD);
        
        if (originalContent == copiedContent) {
            Serial.println("✅ Copy verified - contents match");
        } else {
            Serial.println("❌ Copy failed - contents differ");
        }
    } else {
        Serial.println("❌ Failed to copy file between filesystems");
    }
    
    Serial.println();
}

void listDirectory(const String& path) {
    Serial.println("\\n📂 " + path + ":");
    
    std::vector<FileInfo> files = fileMgr->listDir(path);
    
    if (files.empty()) {
        Serial.println("   (empty)");
        return;
    }
    
    for (const auto& file : files) {
        if (file.isDirectory) {
            Serial.printf("   📁 %s/\\n", file.name.c_str());
        } else {
            Serial.printf("   📄 %s (%u bytes)\\n", file.name.c_str(), file.size);
        }
    }
}

void monitorStorage() {
    static bool firstRun = true;
    
    if (firstRun) {
        Serial.println("\\n📊 Storage Monitor Started (updates every 10s)");
        Serial.println("================================================");
        firstRun = false;
    }
    
    // Check LittleFS usage
    size_t used = fileMgr->getUsedBytes(FS_LITTLEFS);
    size_t total = fileMgr->getTotalBytes(FS_LITTLEFS);
    float usage = (used * 100.0) / total;
    
    Serial.printf("LittleFS: %.1f%% used (%u/%u bytes)\\n", usage, used, total);
    
    // Check SD card usage if available
    if (fileMgr->isSDAvailable()) {
        size_t sdUsed = fileMgr->getUsedBytes(FS_SD_CARD);
        size_t sdTotal = fileMgr->getTotalBytes(FS_SD_CARD);
        float sdUsage = (sdUsed * 100.0) / sdTotal;
        
        Serial.printf("SD Card:  %.1f%% used (%u/%u bytes)\\n", sdUsage, sdUsed, sdTotal);
    }
    
    // Warning for high usage
    if (usage > 80.0) {
        Serial.println("⚠️  WARNING: LittleFS usage above 80%!");
    }
}
