#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <Arduino.h>
#include "FS.h"
#include "LittleFS.h"
#include "SD.h"
#include "SPI.h"
#include <vector>

enum FileSystemType {
    FS_LITTLEFS,
    FS_SD_CARD
};

struct FileInfo {
    String name;
    size_t size;
    bool isDirectory;
    String path;
};

struct SPIPins {
    int sck = 18;   // Serial Clock
    int miso = 19;  // Master In Slave Out
    int mosi = 23;  // Master Out Slave In
    int cs = 5;     // Chip Select
};

class FileManager {
private:
    bool littlefsInitialized;
    bool sdInitialized;
    SPIPins spiPins;
    SPIClass* spiInstance;
    
    // Helper methods
    bool initializeLittleFS();
    bool initializeSD();
    String getContentType(String filename);
    
public:
    FileManager(int sdCardCSPin = 5);
    FileManager(const SPIPins& customPins);
    FileManager(int sck, int miso, int mosi, int cs);
    ~FileManager();
    
    // Initialization
    bool init();
    bool formatLittleFS();
    
    // File system status
    bool isLittleFSAvailable() { return littlefsInitialized; }
    bool isSDAvailable() { return sdInitialized; }
    SPIPins getSPIPins() { return spiPins; }
    
    // File operations
    bool exists(const String& path, FileSystemType fsType = FS_LITTLEFS);
    bool createFile(const String& path, const String& content, FileSystemType fsType = FS_LITTLEFS);
    String readFile(const String& path, FileSystemType fsType = FS_LITTLEFS);
    bool writeFile(const String& path, const String& content, FileSystemType fsType = FS_LITTLEFS);
    bool appendFile(const String& path, const String& content, FileSystemType fsType = FS_LITTLEFS);
    bool deleteFile(const String& path, FileSystemType fsType = FS_LITTLEFS);
    
    // Directory operations
    bool createDir(const String& path, FileSystemType fsType = FS_LITTLEFS);
    bool removeDir(const String& path, FileSystemType fsType = FS_LITTLEFS);
    std::vector<FileInfo> listDir(const String& path, FileSystemType fsType = FS_LITTLEFS);
    
    // Web server integration
    bool serveFile(const String& path, void (*sendResponse)(int, const String&, const String&), FileSystemType fsType = FS_LITTLEFS);
    String getMimeType(const String& filename);
    
    // Storage info
    size_t getTotalBytes(FileSystemType fsType = FS_LITTLEFS);
    size_t getUsedBytes(FileSystemType fsType = FS_LITTLEFS);
    size_t getFreeBytes(FileSystemType fsType = FS_LITTLEFS);
    
    // Utility methods
    void printFileSystem(FileSystemType fsType = FS_LITTLEFS);
    bool copyFile(const String& sourcePath, const String& destPath, FileSystemType sourceFS, FileSystemType destFS);
};

#endif // FILE_MANAGER_H
