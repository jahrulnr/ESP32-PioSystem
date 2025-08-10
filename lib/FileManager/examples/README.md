# FileManager Library Examples

This directory contains comprehensive examples demonstrating the capabilities of the FileManager library for ESP32-S3 systems with both LittleFS and SD card support.

## Overview

The FileManager library provides a unified interface for managing files across multiple storage systems. These examples showcase various use cases from basic file operations to advanced web-based file management.

## Examples Included

### 1. BasicFileOperations
**Purpose**: Introduction to fundamental file and directory operations
- ✅ File creation, reading, writing, and deletion
- ✅ Directory management (create, list, remove)
- ✅ Cross-filesystem operations (LittleFS ↔ SD card)
- ✅ File metadata and size information
- ✅ Error handling and validation

**Best for**: Learning basic file operations, simple data storage

### 2. DataLogger
**Purpose**: Robust data logging system with rotation and analysis
- ✅ Timestamped sensor data logging
- ✅ Automatic log file rotation by size/age
- ✅ Data compression and archival
- ✅ Statistical analysis and reporting
- ✅ CSV export functionality
- ✅ Battery-aware logging optimization

**Best for**: IoT sensor logging, system monitoring, data collection

### 3. WebFileServer
**Purpose**: Complete web-based file management interface
- ✅ HTML5 file browser with drag-and-drop upload
- ✅ RESTful API for file operations (/api/files, /api/upload, etc.)
- ✅ Real-time directory navigation
- ✅ File download and deletion via web interface
- ✅ Support for both LittleFS and SD card
- ✅ Mobile-responsive design

**Best for**: Remote file management, web-based configuration, IoT dashboard integration

### 4. FileSystemExplorer
**Purpose**: Interactive shell-like file system explorer
- ✅ Command-line interface with 20+ commands
- ✅ Unix-like commands (ls, cd, cat, mkdir, rm, cp, mv)
- ✅ Advanced features (find, tree, hexdump, stat)
- ✅ File editing capabilities
- ✅ Disk usage analysis
- ✅ Cross-filesystem navigation

**Best for**: Development debugging, system administration, file system analysis

### 5. CustomSPIPins
**Purpose**: Demonstrates custom SPI pin configuration for SD cards
- ✅ Multiple constructor options for SPI pin setup
- ✅ Custom pin validation and reporting
- ✅ Performance testing with different pin configurations
- ✅ SD card type detection and diagnostics
- ✅ SPI troubleshooting and debugging tools

**Best for**: Custom PCB designs, multi-SPI setups, hardware debugging

## Hardware Requirements

### Minimum Requirements
- ESP32-S3 microcontroller
- LittleFS support (built-in flash storage)

### Optional Components
- MicroSD card module (for SD card examples)
- SD card (any size, formatted as FAT32)
- WiFi connection (for WebFileServer example)

## Wiring for SD Card Support

### Default SPI Pins
```
ESP32-S3    SD Card Module
--------    --------------
GPIO 18  -> SCK (Serial Clock)
GPIO 19  -> MISO (Master In Slave Out)
GPIO 23  -> MOSI (Master Out Slave In)
GPIO 5   -> CS (Chip Select)
3.3V     -> VCC
GND      -> GND
```

### Custom SPI Pins
The FileManager supports custom SPI pin configurations for flexible hardware designs:

```cpp
// Method 1: Using SPIPins struct
SPIPins customPins = {
    .sck = 14,   // Custom SCK pin
    .miso = 12,  // Custom MISO pin
    .mosi = 13,  // Custom MOSI pin
    .cs = 15     // Custom CS pin
};
FileManager* fileMgr = new FileManager(customPins);

// Method 2: Individual parameters
FileManager* fileMgr = new FileManager(14, 12, 13, 15); // sck, miso, mosi, cs

// Method 3: Default pins with custom CS
FileManager* fileMgr = new FileManager(15); // Uses default SPI pins, custom CS
```

**Pin Selection Guidelines**:
- Avoid strapping pins (GPIO 0, 2, 15) when possible
- Use GPIO pins that support the required direction (input/output)
- Keep SPI traces short for signal integrity
- Avoid pins used by other peripherals

**Note**: Custom pin assignments can be modified as needed for your hardware design.

## Getting Started

### 1. Installation
Copy the FileManager library to your `lib/` directory:
```
lib/
├── FileManager/
│   ├── library.json
│   ├── src/
│   │   ├── file_manager.h
│   │   └── file_manager.cpp
│   └── examples/          ← This directory
```

### 2. Basic Usage
```cpp
#include "file_manager.h"

// Method 1: Default SPI pins (18, 19, 23) with custom CS
FileManager* fileMgr = new FileManager(5);

// Method 2: Fully custom SPI pins
SPIPins customPins = {.sck = 14, .miso = 12, .mosi = 13, .cs = 15};
FileManager* fileMgr = new FileManager(customPins);

// Method 3: Individual pin specification
FileManager* fileMgr = new FileManager(14, 12, 13, 15); // sck, miso, mosi, cs

void setup() {
    Serial.begin(115200);
    
    if (!fileMgr->init()) {
        Serial.println("FileManager initialization failed!");
        return;
    }
    
    // Get current SPI configuration
    SPIPins pins = fileMgr->getSPIPins();
    Serial.printf("Using SPI pins - SCK:%d, MISO:%d, MOSI:%d, CS:%d\n",
                  pins.sck, pins.miso, pins.mosi, pins.cs);
    
    // Create a file on LittleFS
    fileMgr->createFile("/test.txt", "Hello World!", FS_LITTLEFS);
    
    // Read the file
    String content = fileMgr->readFile("/test.txt", FS_LITTLEFS);
    Serial.println("File content: " + content);
}
```

### 3. Running Examples

#### Option A: Direct Upload
1. Copy example code to your main project
2. Ensure FileManager library is in your `lib/` directory
3. Upload to your ESP32-S3

#### Option B: PlatformIO Example
1. Create new project with examples as separate environments
2. Use `platformio.ini` environments for each example
3. Switch environments to test different examples

## Example Configurations

### For BasicFileOperations
```cpp
// Default SPI pins
FileManager* fileMgr = new FileManager(5); // CS pin only

// Custom SPI pins
SPIPins pins = {.sck = 14, .miso = 12, .mosi = 13, .cs = 15};
FileManager* fileMgr = new FileManager(pins);
```

### For DataLogger
```cpp
// Standard configuration
FileManager* fileMgr = new FileManager(5);

// Custom pins for specialized hardware
FileManager* fileMgr = new FileManager(21, 22, 4, 2); // sck, miso, mosi, cs

// Configure logging parameters
const size_t MAX_LOG_SIZE = 1024 * 100;  // 100KB per file
const int MAX_LOG_FILES = 10;             // Keep 10 files
```

### For WebFileServer
```cpp
// Multi-SPI setup (SD on custom pins, other SPI devices on default)
FileManager* fileMgr = new FileManager(14, 12, 13, 15);
WebServer server(80);
// Requires WiFi configuration
```

### For FileSystemExplorer
```cpp
// Debug configuration with pin reporting
FileManager* fileMgr = new FileManager(5);
// Interactive shell via Serial Monitor at 115200 baud
// Use getSPIPins() to verify configuration
```

### For CustomSPIPins
```cpp
// Demonstrates all constructor methods
FileManager* fm1 = new FileManager(5);                    // Default + CS
FileManager* fm2 = new FileManager(customPins);           // Struct
FileManager* fm3 = new FileManager(14, 12, 13, 15);       // Individual pins
```

## File System Types

The FileManager supports multiple storage systems:

### FS_LITTLEFS (Internal Flash)
- **Capacity**: ~1.5MB (depending on partition scheme)
- **Speed**: Very fast access
- **Durability**: High (wear leveling built-in)
- **Use cases**: Configuration files, small data sets, system files

### FS_SD_CARD (External Storage)
- **Capacity**: Up to 32GB (FAT32 limit)
- **Speed**: Moderate (depends on card class)
- **Durability**: Moderate (depends on card quality)
- **Use cases**: Large data files, logs, media storage

## API Reference

### Core Functions
```cpp
// Initialization
bool init()
bool isLittleFSAvailable()
bool isSDAvailable()

// File Operations
bool createFile(path, content, filesystem)
String readFile(path, filesystem)
bool writeFile(path, content, filesystem)
bool deleteFile(path, filesystem)
bool exists(path, filesystem)

// Directory Operations
bool createDir(path, filesystem)
bool removeDir(path, filesystem)
std::vector<FileInfo> listDir(path, filesystem)

// Storage Information
size_t getTotalBytes(filesystem)
size_t getUsedBytes(filesystem)
size_t getFreeBytes(filesystem)

// Cross-filesystem Operations
bool copyFile(srcPath, destPath, srcFS, destFS)
```

### FileInfo Structure
```cpp
struct FileInfo {
    String name;        // File/directory name
    String path;        // Full path
    size_t size;        // Size in bytes
    bool isDirectory;   // true if directory
};
```

## Performance Considerations

### LittleFS Performance
- **Read speed**: ~500KB/s
- **Write speed**: ~200KB/s
- **Best practices**: 
  - Keep files under 100KB for optimal performance
  - Use for frequently accessed configuration data
  - Minimize write operations to extend flash life

### SD Card Performance
- **Read speed**: 1-10MB/s (card dependent)
- **Write speed**: 0.5-5MB/s (card dependent)
- **Best practices**:
  - Use Class 10 cards for better performance
  - Batch write operations when possible
  - Consider wear leveling for heavy write workloads

## Troubleshooting

### Common Issues

#### "FileManager initialization failed"
- Check SD card wiring connections
- Verify SD card is formatted as FAT32
- Ensure SD card is properly inserted
- Check CS pin assignment matches your hardware

#### "File operation failed"
- Verify file system has enough free space
- Check file path syntax (must start with '/')
- Ensure directory exists before creating files
- Verify file system is properly mounted

#### "Out of memory" errors
- Reduce buffer sizes in large file operations
- Process files in smaller chunks
- Monitor heap usage with `ESP.getFreeHeap()`

### Debug Tips

1. **Enable verbose logging**:
```cpp
#define CORE_DEBUG_LEVEL 5
```

2. **Check available space**:
```cpp
Serial.printf("LittleFS free: %u bytes\\n", fileMgr->getFreeBytes(FS_LITTLEFS));
Serial.printf("SD card free: %u bytes\\n", fileMgr->getFreeBytes(FS_SD_CARD));
```

3. **Monitor file operations**:
```cpp
bool success = fileMgr->createFile("/test.txt", "content", FS_LITTLEFS);
Serial.printf("File creation %s\\n", success ? "succeeded" : "failed");
```

## Advanced Features

### Automatic Failover
The FileManager can automatically fall back to alternative storage:
```cpp
// Try SD card first, fall back to LittleFS
if (!fileMgr->createFile("/data.txt", content, FS_SD_CARD)) {
    fileMgr->createFile("/data.txt", content, FS_LITTLEFS);
}
```

### Batch Operations
For efficiency with multiple files:
```cpp
std::vector<String> files = {"file1.txt", "file2.txt", "file3.txt"};
for (const auto& filename : files) {
    fileMgr->createFile("/" + filename, "content", FS_LITTLEFS);
}
```

### Cross-Filesystem Sync
Synchronize files between storage systems:
```cpp
// Copy all files from LittleFS to SD card
auto files = fileMgr->listDir("/", FS_LITTLEFS);
for (const auto& file : files) {
    if (!file.isDirectory) {
        String content = fileMgr->readFile(file.path, FS_LITTLEFS);
        fileMgr->createFile(file.path, content, FS_SD_CARD);
    }
}
```

## Integration Examples

### With Battery Management
```cpp
#include "battery_manager.h"
#include "file_manager.h"

// Reduce logging frequency on low battery
if (batteryMgr->getBatteryPercentage() < 20) {
    // Log every 60 seconds instead of 10
    logInterval = 60000;
}
```

### With Display System
```cpp
#include "display_manager.h"
#include "file_manager.h"

// Show file operation status on display
displayMgr->showMessage("Saving data...");
bool success = fileMgr->createFile("/sensor.log", data, FS_LITTLEFS);
displayMgr->showMessage(success ? "Saved!" : "Error!");
```

### With IoT Manager
```cpp
#include "iot_manager.h"
#include "file_manager.h"

// Save device configurations
String deviceConfig = iotMgr->getDeviceConfigJSON();
fileMgr->createFile("/config/devices.json", deviceConfig, FS_LITTLEFS);
```

## Contributing

To add new examples or improve existing ones:

1. Follow the existing code structure and commenting style
2. Include comprehensive error handling
3. Add documentation for any new features
4. Test with both LittleFS and SD card storage
5. Ensure examples work with minimal hardware requirements

## License

This examples collection is provided under the same license as the main project.

---

**Need Help?** 
- Check the main FileManager library documentation
- Review the platformio.ini configuration
- Test with the BasicFileOperations example first
- Use the FileSystemExplorer for debugging file system issues
