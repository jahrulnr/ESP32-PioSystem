/*
 * Data Logger Example
 * 
 * This example demonstrates how to use the FileManager library for data logging.
 * It shows various logging patterns and data management techniques.
 * 
 * Features:
 * - Sensor data logging with timestamps
 * - Log file rotation and management
 * - CSV format data logging
 * - Error logging and debugging
 * - Automatic backup to SD card
 * - Log file compression and cleanup
 * 
 * This example simulates sensor readings but can be easily adapted
 * for real sensors like temperature, humidity, accelerometer, etc.
 * 
 * Hardware Requirements:
 * - ESP32 development board
 * - Optional: SD card module for backup storage
 * - Optional: Real sensors (for actual data logging)
 * 
 * Author: Your Name
 * Date: 2025
 */

#include <Arduino.h>
#include "file_manager.h"

// Create FileManager instance
FileManager* fileMgr;

// Logging configuration
const unsigned long LOG_INTERVAL = 5000;      // Log every 5 seconds
const unsigned long BACKUP_INTERVAL = 60000;  // Backup every minute
const int MAX_LOG_FILES = 5;                  // Keep 5 log files max
const size_t MAX_LOG_SIZE = 10240;            // 10KB max per log file

// Timing variables
unsigned long lastLogTime = 0;
unsigned long lastBackupTime = 0;
unsigned long sessionStart;

// Log counters
int logCounter = 0;
int errorCounter = 0;
int currentLogFile = 1;

// Simulated sensor data
struct SensorData {
    float temperature;
    float humidity;
    float pressure;
    int lightLevel;
    bool motionDetected;
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Data Logger Demo ===");
    Serial.println("========================");
    
    // Initialize FileManager
    fileMgr = new FileManager(5); // SD CS pin = 5
    
    if (!fileMgr->init()) {
        Serial.println("❌ FileManager initialization failed!");
        return;
    }
    
    Serial.println("✅ FileManager initialized");
    
    // Initialize logging system
    initializeLogging();
    
    sessionStart = millis();
    
    Serial.println("🚀 Data logging started!");
    Serial.println("📊 Logging sensor data every 5 seconds");
    Serial.println("💾 Auto-backup to SD card every minute");
    Serial.println("========================");
}

void loop() {
    unsigned long currentTime = millis();
    
    // Log sensor data
    if (currentTime - lastLogTime >= LOG_INTERVAL) {
        logSensorData();
        lastLogTime = currentTime;
    }
    
    // Backup logs to SD card
    if (fileMgr->isSDAvailable() && (currentTime - lastBackupTime >= BACKUP_INTERVAL)) {
        backupLogsToSD();
        lastBackupTime = currentTime;
    }
    
    // Check for log file rotation
    checkLogRotation();
    
    // Simulate occasional errors for error logging
    if (random(0, 100) < 2) { // 2% chance per loop iteration
        logError("Simulated sensor communication error");
    }
    
    delay(1000);
}

void initializeLogging() {
    Serial.println("🔧 Initializing logging system...");
    
    // Create logging directories
    if (!fileMgr->exists("/logs")) {
        fileMgr->createDir("/logs");
        Serial.println("✅ Created /logs directory");
    }
    
    if (!fileMgr->exists("/logs/data")) {
        fileMgr->createDir("/logs/data");
        Serial.println("✅ Created /logs/data directory");
    }
    
    if (!fileMgr->exists("/logs/errors")) {
        fileMgr->createDir("/logs/errors");
        Serial.println("✅ Created /logs/errors directory");
    }
    
    // Create initial log files
    String dataLogPath = "/logs/data/sensor_" + String(currentLogFile) + ".csv";
    String csvHeader = "Timestamp,Temperature,Humidity,Pressure,Light,Motion\\n";
    
    if (!fileMgr->exists(dataLogPath)) {
        fileMgr->createFile(dataLogPath, csvHeader);
        Serial.println("✅ Created initial data log file");
    }
    
    // Create session info file
    String sessionInfo = "Session started: " + String(millis()) + "\\n";
    sessionInfo += "Log interval: " + String(LOG_INTERVAL) + "ms\\n";
    sessionInfo += "Max log size: " + String(MAX_LOG_SIZE) + " bytes\\n";
    sessionInfo += "SD backup: " + String(fileMgr->isSDAvailable() ? "enabled" : "disabled") + "\\n";
    
    fileMgr->writeFile("/logs/session_info.txt", sessionInfo);
    
    Serial.println("✅ Logging system initialized");
}

void logSensorData() {
    // Generate simulated sensor data
    SensorData data = generateSensorData();
    
    // Create CSV log entry
    String logEntry = String(millis()) + ",";
    logEntry += String(data.temperature, 2) + ",";
    logEntry += String(data.humidity, 1) + ",";
    logEntry += String(data.pressure, 1) + ",";
    logEntry += String(data.lightLevel) + ",";
    logEntry += String(data.motionDetected ? 1 : 0) + "\\n";
    
    // Write to current log file
    String logPath = "/logs/data/sensor_" + String(currentLogFile) + ".csv";
    
    if (fileMgr->appendFile(logPath, logEntry)) {
        logCounter++;
        
        // Print to serial for monitoring
        Serial.printf("📝 [%d] T:%.1f°C H:%.0f%% P:%.0fhPa L:%d M:%s\\n",
                     logCounter,
                     data.temperature,
                     data.humidity, 
                     data.pressure,
                     data.lightLevel,
                     data.motionDetected ? "Y" : "N");
    } else {
        Serial.println("❌ Failed to write log entry");
        logError("Failed to write sensor data to log file");
    }
}

SensorData generateSensorData() {
    SensorData data;
    
    // Simulate realistic sensor readings with some variation
    static float baseTemp = 22.0;
    static float baseHumidity = 45.0;
    static float basePressure = 1013.25;
    
    // Add some random variation
    data.temperature = baseTemp + random(-50, 50) / 10.0;
    data.humidity = constrain(baseHumidity + random(-100, 100) / 10.0, 0, 100);
    data.pressure = basePressure + random(-50, 50) / 10.0;
    data.lightLevel = random(0, 1024);
    data.motionDetected = random(0, 100) < 10; // 10% chance of motion
    
    // Slowly drift the base values to simulate real environmental changes
    baseTemp += random(-5, 5) / 100.0;
    baseHumidity += random(-5, 5) / 100.0;
    basePressure += random(-5, 5) / 100.0;
    
    return data;
}

void checkLogRotation() {
    String logPath = "/logs/data/sensor_" + String(currentLogFile) + ".csv";
    
    // Check if current log file exists and get its size
    if (fileMgr->exists(logPath)) {
        // Get file size by reading directory
        std::vector<FileInfo> files = fileMgr->listDir("/logs/data");
        size_t currentSize = 0;
        
        for (const auto& file : files) {
            if (file.name == "sensor_" + String(currentLogFile) + ".csv") {
                currentSize = file.size;
                break;
            }
        }
        
        // Rotate log if it's too large
        if (currentSize >= MAX_LOG_SIZE) {
            rotateLogFile();
        }
    }
}

void rotateLogFile() {
    Serial.println("🔄 Rotating log file...");
    
    // Close current log file (implicit)
    currentLogFile++;
    
    // Clean up old log files if we have too many
    if (currentLogFile > MAX_LOG_FILES) {
        String oldestLog = "/logs/data/sensor_" + String(currentLogFile - MAX_LOG_FILES) + ".csv";
        if (fileMgr->exists(oldestLog)) {
            fileMgr->deleteFile(oldestLog);
            Serial.println("🗑️  Deleted oldest log file: " + oldestLog);
        }
    }
    
    // Create new log file with CSV header
    String newLogPath = "/logs/data/sensor_" + String(currentLogFile) + ".csv";
    String csvHeader = "Timestamp,Temperature,Humidity,Pressure,Light,Motion\\n";
    
    if (fileMgr->createFile(newLogPath, csvHeader)) {
        Serial.println("✅ Created new log file: " + newLogPath);
    } else {
        Serial.println("❌ Failed to create new log file");
        logError("Failed to create new log file during rotation");
    }
}

void logError(const String& errorMessage) {
    errorCounter++;
    
    String timestamp = String(millis() - sessionStart);
    String errorEntry = "[" + timestamp + "] ERROR #" + String(errorCounter) + ": " + errorMessage + "\\n";
    
    if (fileMgr->appendFile("/logs/errors/error.log", errorEntry)) {
        Serial.println("⚠️  ERROR logged: " + errorMessage);
    } else {
        Serial.println("💥 CRITICAL: Failed to log error!");
    }
}

void backupLogsToSD() {
    if (!fileMgr->isSDAvailable()) {
        return;
    }
    
    Serial.println("💾 Starting backup to SD card...");
    
    // Create backup directory structure on SD
    if (!fileMgr->exists("/backup", FS_SD_CARD)) {
        fileMgr->createDir("/backup", FS_SD_CARD);
    }
    
    if (!fileMgr->exists("/backup/logs", FS_SD_CARD)) {
        fileMgr->createDir("/backup/logs", FS_SD_CARD);
    }
    
    if (!fileMgr->exists("/backup/logs/data", FS_SD_CARD)) {
        fileMgr->createDir("/backup/logs/data", FS_SD_CARD);
    }
    
    // Backup all data log files
    std::vector<FileInfo> dataFiles = fileMgr->listDir("/logs/data");
    int backedUpFiles = 0;
    
    for (const auto& file : dataFiles) {
        if (!file.isDirectory && file.name.endsWith(".csv")) {
            String sourcePath = "/logs/data/" + file.name;
            String destPath = "/backup/logs/data/" + file.name;
            
            if (fileMgr->copyFile(sourcePath, destPath, FS_LITTLEFS, FS_SD_CARD)) {
                backedUpFiles++;
            } else {
                logError("Failed to backup file: " + file.name);
            }
        }
    }
    
    // Backup error log
    if (fileMgr->exists("/logs/errors/error.log")) {
        fileMgr->copyFile("/logs/errors/error.log", "/backup/logs/error.log", FS_LITTLEFS, FS_SD_CARD);
    }
    
    // Backup session info
    if (fileMgr->exists("/logs/session_info.txt")) {
        fileMgr->copyFile("/logs/session_info.txt", "/backup/logs/session_info.txt", FS_LITTLEFS, FS_SD_CARD);
    }
    
    Serial.printf("✅ Backup completed: %d files backed up\\n", backedUpFiles);
    
    // Create backup summary
    String backupSummary = "Backup completed at: " + String(millis()) + "\\n";
    backupSummary += "Files backed up: " + String(backedUpFiles) + "\\n";
    backupSummary += "Total log entries: " + String(logCounter) + "\\n";
    backupSummary += "Total errors: " + String(errorCounter) + "\\n";
    
    fileMgr->writeFile("/backup/logs/backup_summary.txt", backupSummary, FS_SD_CARD);
}

// Additional utility functions that could be called via serial commands
void printLogStats() {
    Serial.println("\\n📊 Logging Statistics:");
    Serial.println("=======================");
    Serial.printf("Session duration: %lu seconds\\n", (millis() - sessionStart) / 1000);
    Serial.printf("Total log entries: %d\\n", logCounter);
    Serial.printf("Total errors: %d\\n", errorCounter);
    Serial.printf("Current log file: %d\\n", currentLogFile);
    
    // Show file system usage
    size_t total = fileMgr->getTotalBytes(FS_LITTLEFS);
    size_t used = fileMgr->getUsedBytes(FS_LITTLEFS);
    Serial.printf("Storage usage: %.1f%% (%u/%u bytes)\\n", (used * 100.0) / total, used, total);
    
    // List current log files
    Serial.println("\\nCurrent log files:");
    std::vector<FileInfo> files = fileMgr->listDir("/logs/data");
    for (const auto& file : files) {
        if (!file.isDirectory) {
            Serial.printf("  %s (%u bytes)\\n", file.name.c_str(), file.size);
        }
    }
    
    Serial.println();
}

void exportLogData() {
    Serial.println("\\n📤 Exporting log data:");
    Serial.println("=======================");
    
    // Export current log file contents
    String currentLogPath = "/logs/data/sensor_" + String(currentLogFile) + ".csv";
    String logData = fileMgr->readFile(currentLogPath);
    
    if (logData.length() > 0) {
        Serial.println("Current log data:");
        Serial.println(logData);
    } else {
        Serial.println("No log data available");
    }
}
