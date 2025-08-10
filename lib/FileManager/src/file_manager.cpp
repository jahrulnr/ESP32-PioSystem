#include "file_manager.h"
#include "SerialDebug.h"

FileManager::FileManager(int sdCardCSPin) {
    littlefsInitialized = false;
    sdInitialized = false;
    spiPins.cs = sdCardCSPin;
    spiPins.sck = 18;
    spiPins.miso = 19;
    spiPins.mosi = 23;
    spiInstance = nullptr;
}

FileManager::FileManager(const SPIPins& customPins) {
    littlefsInitialized = false;
    sdInitialized = false;
    spiPins = customPins;
    spiInstance = nullptr;
}

FileManager::FileManager(int sck, int miso, int mosi, int cs) {
    littlefsInitialized = false;
    sdInitialized = false;
    spiPins.sck = sck;
    spiPins.miso = miso;
    spiPins.mosi = mosi;
    spiPins.cs = cs;
    spiInstance = nullptr;
}

FileManager::~FileManager() {
    if (littlefsInitialized) {
        LittleFS.end();
    }
    if (sdInitialized) {
        SD.end();
    }
    if (spiInstance) {
        spiInstance->end();
        delete spiInstance;
    }
}

bool FileManager::init() {
    DEBUG_PRINTLN("FileManager: Initializing file systems...");
    
    bool littlefsOk = initializeLittleFS();
    bool sdOk = false;
    if (spiPins.cs != -1){
        sdOk = initializeSD();
    }
    
    if (littlefsOk) {
        DEBUG_PRINTLN("FileManager: LittleFS initialized successfully");
        printFileSystem(FS_LITTLEFS);
    } else {
        DEBUG_PRINTLN("FileManager: LittleFS initialization failed");
    }
    
    if (sdOk) {
        DEBUG_PRINTLN("FileManager: SD Card initialized successfully");
        DEBUG_PRINTF("FileManager: Using SPI pins - SCK: %d, MISO: %d, MOSI: %d, CS: %d\n", 
                     spiPins.sck, spiPins.miso, spiPins.mosi, spiPins.cs);
    } else {
        DEBUG_PRINTLN("FileManager: SD Card not available or failed to initialize");
    }
    
    return littlefsOk; // Return true if at least LittleFS works
}

bool FileManager::initializeLittleFS() {
    if (!LittleFS.begin(true)) {
        DEBUG_PRINTLN("FileManager: LittleFS Mount Failed, trying to format...");
        if (LittleFS.format()) {
            DEBUG_PRINTLN("FileManager: LittleFS Formatted successfully");
            if (LittleFS.begin(true)) {
                littlefsInitialized = true;
                return true;
            }
        }
        return false;
    }
    littlefsInitialized = true;
    return true;
}

bool FileManager::initializeSD() {
    DEBUG_PRINTF("FileManager: Initializing SD card with custom SPI pins - SCK: %d, MISO: %d, MOSI: %d, CS: %d\n", 
                 spiPins.sck, spiPins.miso, spiPins.mosi, spiPins.cs);
    
    // Create custom SPI instance if pins are not default
    if (spiPins.sck != 18 || spiPins.miso != 19 || spiPins.mosi != 23) {
        spiInstance = new SPIClass(HSPI);
        spiInstance->begin(spiPins.sck, spiPins.miso, spiPins.mosi, spiPins.cs);
        
        if (!SD.begin(spiPins.cs, *spiInstance)) {
            DEBUG_PRINTLN("FileManager: SD initialization failed with custom SPI");
            return false;
        }
    } else {
        // Use default SPI
        if (!SD.begin(spiPins.cs)) {
            DEBUG_PRINTLN("FileManager: SD initialization failed with default SPI");
            return false;
        }
    }
    
    // Verify SD card type
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        DEBUG_PRINTLN("FileManager: No SD card attached");
        return false;
    }
    
    DEBUG_PRINT("FileManager: SD Card Type: ");
    if (cardType == CARD_MMC) {
        DEBUG_PRINTLN("MMC");
    } else if (cardType == CARD_SD) {
        DEBUG_PRINTLN("SDSC");
    } else if (cardType == CARD_SDHC) {
        DEBUG_PRINTLN("SDHC");
    } else {
        DEBUG_PRINTLN("UNKNOWN");
    }
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    DEBUG_PRINTF("FileManager: SD Card Size: %lluMB\n", cardSize);
    
    sdInitialized = true;
    return true;
}

bool FileManager::formatLittleFS() {
    if (LittleFS.format()) {
        DEBUG_PRINTLN("FileManager: LittleFS formatted successfully");
        return initializeLittleFS();
    }
    return false;
}

bool FileManager::exists(const String& path, FileSystemType fsType) {
    if (fsType == FS_LITTLEFS && littlefsInitialized) {
        return LittleFS.exists(path);
    } else if (fsType == FS_SD_CARD && sdInitialized) {
        return SD.exists(path);
    }
    return false;
}

bool FileManager::createFile(const String& path, const String& content, FileSystemType fsType) {
    return writeFile(path, content, fsType);
}

String FileManager::readFile(const String& path, FileSystemType fsType) {
    File file;
    
    if (fsType == FS_LITTLEFS && littlefsInitialized) {
        file = LittleFS.open(path, "r");
    } else if (fsType == FS_SD_CARD && sdInitialized) {
        file = SD.open(path, FILE_READ);
    } else {
        return "";
    }
    
    if (!file) {
        DEBUG_PRINTLN("FileManager: Failed to open file for reading: " + path);
        return "";
    }
    
    String content = file.readString();
    file.close();
    return content;
}

bool FileManager::writeFile(const String& path, const String& content, FileSystemType fsType) {
    File file;
    
    if (fsType == FS_LITTLEFS && littlefsInitialized) {
        file = LittleFS.open(path, "w");
    } else if (fsType == FS_SD_CARD && sdInitialized) {
        file = SD.open(path, FILE_WRITE);
    } else {
        return false;
    }
    
    if (!file) {
        DEBUG_PRINTLN("FileManager: Failed to open file for writing: " + path);
        return false;
    }
    
    size_t bytesWritten = file.print(content);
    file.close();
    
    bool success = (bytesWritten == content.length());
    if (success) {
        DEBUG_PRINTLN("FileManager: File written successfully: " + path);
    } else {
        DEBUG_PRINTLN("FileManager: Failed to write file: " + path);
    }
    
    return success;
}

bool FileManager::appendFile(const String& path, const String& content, FileSystemType fsType) {
    File file;
    
    if (fsType == FS_LITTLEFS && littlefsInitialized) {
        file = LittleFS.open(path, "a");
    } else if (fsType == FS_SD_CARD && sdInitialized) {
        file = SD.open(path, FILE_APPEND);
    } else {
        return false;
    }
    
    if (!file) {
        DEBUG_PRINTLN("FileManager: Failed to open file for appending: " + path);
        return false;
    }
    
    size_t bytesWritten = file.print(content);
    file.close();
    return (bytesWritten == content.length());
}

bool FileManager::deleteFile(const String& path, FileSystemType fsType) {
    if (fsType == FS_LITTLEFS && littlefsInitialized) {
        return LittleFS.remove(path);
    } else if (fsType == FS_SD_CARD && sdInitialized) {
        return SD.remove(path);
    }
    return false;
}

bool FileManager::createDir(const String& path, FileSystemType fsType) {
    if (fsType == FS_LITTLEFS && littlefsInitialized) {
        // LittleFS doesn't have true directories, but we can create the path structure
        return true;
    } else if (fsType == FS_SD_CARD && sdInitialized) {
        return SD.mkdir(path);
    }
    return false;
}

bool FileManager::removeDir(const String& path, FileSystemType fsType) {
    if (fsType == FS_LITTLEFS && littlefsInitialized) {
        // For LittleFS, we need to remove all files with this path prefix
        File root = LittleFS.open("/");
        File file = root.openNextFile();
        while (file) {
            if (String(file.name()).startsWith(path)) {
                LittleFS.remove(file.name());
            }
            file = root.openNextFile();
        }
        return true;
    } else if (fsType == FS_SD_CARD && sdInitialized) {
        return SD.rmdir(path);
    }
    return false;
}

std::vector<FileInfo> FileManager::listDir(const String& path, FileSystemType fsType) {
    std::vector<FileInfo> files;
    
    if (fsType == FS_LITTLEFS && littlefsInitialized) {
        File root = LittleFS.open(path.isEmpty() ? "/" : path);
        if (!root || !root.isDirectory()) {
            return files;
        }
        
        File file = root.openNextFile();
        while (file) {
            FileInfo info;
            info.name = String(file.name());
            info.size = file.size();
            info.isDirectory = file.isDirectory();
            info.path = String(file.path());
            files.push_back(info);
            file = root.openNextFile();
        }
    } else if (fsType == FS_SD_CARD && sdInitialized) {
        File root = SD.open(path.isEmpty() ? "/" : path);
        if (!root || !root.isDirectory()) {
            return files;
        }
        
        File file = root.openNextFile();
        while (file) {
            FileInfo info;
            info.name = String(file.name());
            info.size = file.size();
            info.isDirectory = file.isDirectory();
            info.path = String(file.path());
            files.push_back(info);
            file = root.openNextFile();
        }
    }
    
    return files;
}

String FileManager::getMimeType(const String& filename) {
    if (filename.endsWith(".html") || filename.endsWith(".htm")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".svg")) return "image/svg+xml";
    else if (filename.endsWith(".txt")) return "text/plain";
    else if (filename.endsWith(".pdf")) return "application/pdf";
    else if (filename.endsWith(".zip")) return "application/zip";
    else if (filename.endsWith(".xml")) return "text/xml";
    return "application/octet-stream";
}

size_t FileManager::getTotalBytes(FileSystemType fsType) {
    if (fsType == FS_LITTLEFS && littlefsInitialized) {
        return LittleFS.totalBytes();
    } else if (fsType == FS_SD_CARD && sdInitialized) {
        return SD.totalBytes();
    }
    return 0;
}

size_t FileManager::getUsedBytes(FileSystemType fsType) {
    if (fsType == FS_LITTLEFS && littlefsInitialized) {
        return LittleFS.usedBytes();
    } else if (fsType == FS_SD_CARD && sdInitialized) {
        return SD.usedBytes();
    }
    return 0;
}

size_t FileManager::getFreeBytes(FileSystemType fsType) {
    return getTotalBytes(fsType) - getUsedBytes(fsType);
}

void FileManager::printFileSystem(FileSystemType fsType) {
    String fsName = (fsType == FS_LITTLEFS) ? "LittleFS" : "SD Card";
    
    DEBUG_PRINTLN("=== " + fsName + " File System Info ===");
    DEBUG_PRINTF("Total: %u bytes\n", getTotalBytes(fsType));
    DEBUG_PRINTF("Used: %u bytes\n", getUsedBytes(fsType));
    DEBUG_PRINTF("Free: %u bytes\n", getFreeBytes(fsType));
    
    DEBUG_PRINTLN("\n=== Files ===");
    auto files = listDir("/", fsType);
    for (const auto& file : files) {
        DEBUG_PRINTF("%s %s (%u bytes)\n", 
                     file.isDirectory ? "[DIR]" : "[FILE]", 
                     file.name.c_str(), 
                     file.size);
    }
    DEBUG_PRINTLN("================");
}

bool FileManager::copyFile(const String& sourcePath, const String& destPath, FileSystemType sourceFS, FileSystemType destFS) {
    String content = readFile(sourcePath, sourceFS);
    if (content.length() == 0 && !exists(sourcePath, sourceFS)) {
        return false;
    }
    return writeFile(destPath, content, destFS);
}
