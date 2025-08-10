# FileManager Quick Start Guide

Get up and running with the FileManager library in 5 minutes!

## 🚀 Quick Setup

### 1. Hardware Check
- ✅ ESP32-S3 board
- ✅ USB connection for Serial Monitor
- ⚡ Optional: MicroSD card + module

### 2. Copy Library
Ensure FileManager is in your `lib/` directory:
```
your_project/
├── src/main.cpp
├── lib/
│   └── FileManager/
│       ├── library.json
│       ├── src/
│       └── examples/
└── platformio.ini
```

### 3. Basic Test
Copy this code to test your setup:

```cpp
#include <Arduino.h>
#include "file_manager.h"

FileManager* fileMgr;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("FileManager Quick Test");
    Serial.println("======================");
    
    // Option 1: Default SPI pins (SCK:18, MISO:19, MOSI:23, CS:5)
    fileMgr = new FileManager(5);
    
    // Option 2: Custom SPI pins using struct
    // SPIPins customPins = {.sck = 14, .miso = 12, .mosi = 13, .cs = 15};
    // fileMgr = new FileManager(customPins);
    
    // Option 3: Custom SPI pins using parameters  
    // fileMgr = new FileManager(14, 12, 13, 15); // sck, miso, mosi, cs
    
    if (!fileMgr->init()) {
        Serial.println("❌ FileManager init failed!");
        return;
    }
    
    Serial.println("✅ FileManager initialized");
    
    // Show SPI pin configuration
    SPIPins pins = fileMgr->getSPIPins();
    Serial.printf("🔧 SPI pins - SCK:%d, MISO:%d, MOSI:%d, CS:%d\\n",
                  pins.sck, pins.miso, pins.mosi, pins.cs);
    
    // Test LittleFS
    if (fileMgr->isLittleFSAvailable()) {
        Serial.println("✅ LittleFS available");
        
        // Create test file
        if (fileMgr->createFile("/test.txt", "Hello World!", FS_LITTLEFS)) {
            Serial.println("✅ File created on LittleFS");
            
            // Read it back
            String content = fileMgr->readFile("/test.txt", FS_LITTLEFS);
            Serial.println("📄 Content: " + content);
        }
    }
    
    // Test SD card (if available)
    if (fileMgr->isSDAvailable()) {
        Serial.println("✅ SD card available");
        fileMgr->createFile("/sd_test.txt", "SD card works!", FS_SD_CARD);
        Serial.println("✅ File created on SD card");
    } else {
        Serial.println("ℹ️  SD card not detected (check wiring/pins)");
    }
    
    // Show storage info
    size_t lfsTotal = fileMgr->getTotalBytes(FS_LITTLEFS);
    size_t lfsUsed = fileMgr->getUsedBytes(FS_LITTLEFS);
    Serial.printf("📊 LittleFS: %u/%u bytes used\\n", lfsUsed, lfsTotal);
    
    Serial.println("\\n🎉 FileManager is working correctly!");
}

void loop() {
    // Nothing to do in loop for this test
    delay(1000);
}
```

### 4. Upload and Test
1. Upload the code to your ESP32-S3
2. Open Serial Monitor (115200 baud)
3. You should see success messages

## 📝 Next Steps

### Try the Examples

#### For Basic Learning:
```bash
# Start with BasicFileOperations
cp lib/FileManager/examples/BasicFileOperations/BasicFileOperations.ino src/main.cpp
```

#### For Data Logging:
```bash
# Try DataLogger for sensor data
cp lib/FileManager/examples/DataLogger/DataLogger.ino src/main.cpp
```

#### For Web Interface:
```bash
# Use WebFileServer for remote access
cp lib/FileManager/examples/WebFileServer/WebFileServer.ino src/main.cpp
# Don't forget to configure WiFi credentials!
```

#### For Custom Hardware:
```bash
# Try CustomSPIPins for non-standard pin layouts
cp lib/FileManager/examples/CustomSPIPins/CustomSPIPins.ino src/main.cpp
# Configure your custom SPI pins in the code
```

### Common Patterns

#### Save Configuration
```cpp
String config = "{\\"wifi\\": \\"MyNetwork\\", \\"timeout\\": 30}";
fileMgr->createFile("/config.json", config, FS_LITTLEFS);
```

#### Read Configuration
```cpp
String config = fileMgr->readFile("/config.json", FS_LITTLEFS);
if (config.length() > 0) {
    // Parse JSON and use config
}
```

#### Log Sensor Data
```cpp
String timestamp = "2025-01-11 10:30:00";
String data = timestamp + ",25.6,60.2,1013.25\\n"; // temp,humidity,pressure
fileMgr->appendFile("/sensors.csv", data, FS_SD_CARD);
```

#### List Files
```cpp
auto files = fileMgr->listDir("/", FS_LITTLEFS);
for (const auto& file : files) {
    Serial.println(file.isDirectory ? "DIR: " : "FILE: " + file.name);
}
```

## 🔧 Troubleshooting

### Problem: "Init failed"
**Solution**: Check that FileManager library is in correct location

### Problem: "SD card not detected" 
**Solutions**: 
- Check wiring (Default: CS=pin 5, SCK=18, MISO=19, MOSI=23)
- For custom pins, use: `FileManager(sck, miso, mosi, cs)` 
- Ensure SD card is FAT32 formatted
- Try different SD card
- Use `getSPIPins()` to verify pin configuration

### Problem: "Out of memory"
**Solutions**:
- Use smaller file chunks
- Monitor with `ESP.getFreeHeap()`
- Reduce buffer sizes

### Problem: "File not found"
**Solutions**:
- Ensure path starts with "/"
- Check if directory exists first
- Verify correct filesystem (FS_LITTLEFS vs FS_SD_CARD)

## 💡 Pro Tips

1. **Always check return values**: File operations can fail
2. **Use LittleFS for config**: Faster and more reliable than SD
3. **Use SD for large data**: Better for logs and media files
4. **Handle filesystem full**: Check free space before writing
5. **Test without SD card**: Ensure your code works with just LittleFS
6. **Custom SPI pins**: Use `FileManager(sck, miso, mosi, cs)` for non-standard layouts
7. **Verify pin config**: Use `getSPIPins()` to debug SPI setup

## 📚 Learn More

- Read the full [README.md](README.md) for detailed examples
- Check the source code in `src/file_manager.h` for all methods
- Try different examples to see various use cases
- Integrate with other libraries (Battery, Display, etc.)

---

**Ready to build something awesome? Start with BasicFileOperations and work your way up!** 🚀
