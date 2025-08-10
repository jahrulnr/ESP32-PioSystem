/*
 * File System Explorer Example
 * 
 * This example provides an interactive command-line interface for exploring
 * and managing files on both LittleFS and SD card storage systems.
 * 
 * Features:
 * - Interactive shell-like interface
 * - File and directory navigation
 * - File creation, editing, and deletion
 * - Directory operations
 * - File system analysis and statistics
 * - Hex dump and file inspection
 * - Search functionality
 * - Backup and restore operations
 * 
 * Commands Available:
 * - ls [path]           - List directory contents
 * - cd <path>           - Change directory
 * - pwd                 - Show current directory
 * - cat <file>          - Display file contents
 * - mkdir <dir>         - Create directory
 * - touch <file>        - Create empty file
 * - rm <file>           - Delete file
 * - rmdir <dir>         - Remove directory
 * - cp <src> <dest>     - Copy file
 * - mv <src> <dest>     - Move/rename file
 * - find <pattern>      - Search for files
 * - du [path]           - Show disk usage
 * - df                  - Show filesystem info
 * - hexdump <file>      - Show hex dump of file
 * - switch <fs>         - Switch filesystem (littlefs/sd)
 * - help                - Show all commands
 * 
 * Author: Your Name
 * Date: 2025
 */

#include <Arduino.h>
#include "file_manager.h"

// Create FileManager instance
FileManager* fileMgr;

// Shell state
String currentPath = "/";
FileSystemType currentFS = FS_LITTLEFS;
String commandHistory[10];
int historyCount = 0;

// Command structure
struct Command {
    String name;
    String description;
    void (*handler)(const String& args);
};

// Forward declarations
void cmdList(const String& args);
void cmdChangeDir(const String& args);
void cmdPrintWorkingDir(const String& args);
void cmdCat(const String& args);
void cmdMakeDir(const String& args);
void cmdTouch(const String& args);
void cmdRemove(const String& args);
void cmdRemoveDir(const String& args);
void cmdCopy(const String& args);
void cmdMove(const String& args);
void cmdFind(const String& args);
void cmdDiskUsage(const String& args);
void cmdDiskFree(const String& args);
void cmdHexDump(const String& args);
void cmdSwitch(const String& args);
void cmdHelp(const String& args);
void cmdClear(const String& args);
void cmdHistory(const String& args);
void cmdTree(const String& args);
void cmdStat(const String& args);
void cmdEdit(const String& args);

// Available commands
Command commands[] = {
    {"ls", "List directory contents", cmdList},
    {"cd", "Change directory", cmdChangeDir},
    {"pwd", "Print working directory", cmdPrintWorkingDir},
    {"cat", "Display file contents", cmdCat},
    {"mkdir", "Create directory", cmdMakeDir},
    {"touch", "Create empty file", cmdTouch},
    {"rm", "Remove file", cmdRemove},
    {"rmdir", "Remove directory", cmdRemoveDir},
    {"cp", "Copy file", cmdCopy},
    {"mv", "Move/rename file", cmdMove},
    {"find", "Search for files", cmdFind},
    {"du", "Show disk usage", cmdDiskUsage},
    {"df", "Show filesystem info", cmdDiskFree},
    {"hexdump", "Show hex dump of file", cmdHexDump},
    {"switch", "Switch filesystem (littlefs/sd)", cmdSwitch},
    {"clear", "Clear screen", cmdClear},
    {"history", "Show command history", cmdHistory},
    {"tree", "Show directory tree", cmdTree},
    {"stat", "Show file statistics", cmdStat},
    {"edit", "Simple file editor", cmdEdit},
    {"help", "Show this help", cmdHelp}
};

const int numCommands = sizeof(commands) / sizeof(Command);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Clear screen and show banner
    Serial.println("\\033[2J\\033[H"); // ANSI clear screen and move cursor to home
    
    Serial.println("╔══════════════════════════════════════════════════╗");
    Serial.println("║            ESP32 File System Explorer           ║");
    Serial.println("║                                                  ║");
    Serial.println("║  Interactive shell for file system management   ║");
    Serial.println("║  Type 'help' for available commands             ║");
    Serial.println("╚══════════════════════════════════════════════════╝");
    Serial.println();
    
    // Initialize FileManager
    fileMgr = new FileManager(5); // SD CS pin = 5
    
    if (!fileMgr->init()) {
        Serial.println("❌ FileManager initialization failed!");
        return;
    }
    
    Serial.println("✅ FileManager initialized");
    
    // Show initial file system status
    showFileSystemStatus();
    
    // Create some sample files if they don't exist
    createSampleContent();
    
    Serial.println();
    showPrompt();
}

void loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\\n');
        input.trim();
        
        if (input.length() > 0) {
            addToHistory(input);
            processCommand(input);
        }
        
        showPrompt();
    }
    
    delay(10);
}

void showFileSystemStatus() {
    Serial.println("📊 File System Status:");
    
    // LittleFS status
    if (fileMgr->isLittleFSAvailable()) {
        size_t total = fileMgr->getTotalBytes(FS_LITTLEFS);
        size_t used = fileMgr->getUsedBytes(FS_LITTLEFS);
        float usage = (used * 100.0) / total;
        
        Serial.printf("✅ LittleFS: %.1f%% used (%u/%u bytes)\\n", usage, used, total);
    } else {
        Serial.println("❌ LittleFS: Not available");
    }
    
    // SD card status
    if (fileMgr->isSDAvailable()) {
        size_t total = fileMgr->getTotalBytes(FS_SD_CARD);
        size_t used = fileMgr->getUsedBytes(FS_SD_CARD);
        float usage = (used * 100.0) / total;
        
        Serial.printf("✅ SD Card: %.1f%% used (%u/%u bytes)\\n", usage, used, total);
    } else {
        Serial.println("❌ SD Card: Not available");
    }
}

void createSampleContent() {
    // Create sample directory structure
    if (!fileMgr->exists("/docs")) {
        fileMgr->createDir("/docs");
        fileMgr->createFile("/docs/readme.txt", "Welcome to ESP32 File System Explorer!\\n\\nThis is a sample document.");
        fileMgr->createFile("/docs/commands.txt", "Available commands:\\nls, cd, pwd, cat, mkdir, touch, rm, help");
    }
    
    if (!fileMgr->exists("/data")) {
        fileMgr->createDir("/data");
        fileMgr->createFile("/data/config.json", "{\\"version\\": \\"1.0\\", \\"debug\\": true}");
        fileMgr->createFile("/data/log.txt", "System log started\\nNo errors recorded");
    }
    
    if (!fileMgr->exists("/temp")) {
        fileMgr->createDir("/temp");
    }
}

void showPrompt() {
    String fsName = (currentFS == FS_LITTLEFS) ? "LFS" : "SD";
    Serial.printf("\\033[32m[%s]\\033[0m \\033[34m%s\\033[0m $ ", fsName.c_str(), currentPath.c_str());
}

void processCommand(const String& input) {
    // Parse command and arguments
    int spaceIndex = input.indexOf(' ');
    String cmd = (spaceIndex == -1) ? input : input.substring(0, spaceIndex);
    String args = (spaceIndex == -1) ? "" : input.substring(spaceIndex + 1);
    
    cmd.toLowerCase();
    
    // Find and execute command
    bool found = false;
    for (int i = 0; i < numCommands; i++) {
        if (commands[i].name == cmd) {
            commands[i].handler(args);
            found = true;
            break;
        }
    }
    
    if (!found) {
        Serial.println("Command not found: " + cmd);
        Serial.println("Type 'help' for available commands");
    }
}

void addToHistory(const String& cmd) {
    // Simple circular buffer for command history
    commandHistory[historyCount % 10] = cmd;
    historyCount++;
}

// Command implementations
void cmdList(const String& args) {
    String path = args.isEmpty() ? currentPath : args;
    
    if (!fileMgr->exists(path, currentFS)) {
        Serial.println("Directory not found: " + path);
        return;
    }
    
    std::vector<FileInfo> files = fileMgr->listDir(path, currentFS);
    
    if (files.empty()) {
        Serial.println("Directory is empty");
        return;
    }
    
    Serial.printf("Contents of %s:\\n", path.c_str());
    Serial.println("Type    Size      Name");
    Serial.println("------- --------- ----------------");
    
    for (const auto& file : files) {
        String type = file.isDirectory ? "DIR" : "FILE";
        String size = file.isDirectory ? "-" : String(file.size);
        
        Serial.printf("%-7s %-9s %s\\n", type.c_str(), size.c_str(), file.name.c_str());
    }
}

void cmdChangeDir(const String& args) {
    if (args.isEmpty()) {
        currentPath = "/";
        return;
    }
    
    String newPath = args;
    
    // Handle relative paths
    if (!newPath.startsWith("/")) {
        if (currentPath.endsWith("/")) {
            newPath = currentPath + newPath;
        } else {
            newPath = currentPath + "/" + newPath;
        }
    }
    
    // Handle .. (parent directory)
    if (newPath.endsWith("/..")) {
        int lastSlash = newPath.lastIndexOf("/", newPath.length() - 4);
        if (lastSlash >= 0) {
            newPath = newPath.substring(0, lastSlash);
            if (newPath.isEmpty()) newPath = "/";
        }
    }
    
    if (fileMgr->exists(newPath, currentFS)) {
        currentPath = newPath;
    } else {
        Serial.println("Directory not found: " + newPath);
    }
}

void cmdPrintWorkingDir(const String& args) {
    String fsName = (currentFS == FS_LITTLEFS) ? "LittleFS" : "SD Card";
    Serial.printf("Current directory: %s (%s)\\n", currentPath.c_str(), fsName.c_str());
}

void cmdCat(const String& args) {
    if (args.isEmpty()) {
        Serial.println("Usage: cat <filename>");
        return;
    }
    
    String fullPath = makeFullPath(args);
    
    if (!fileMgr->exists(fullPath, currentFS)) {
        Serial.println("File not found: " + fullPath);
        return;
    }
    
    String content = fileMgr->readFile(fullPath, currentFS);
    Serial.println("Contents of " + fullPath + ":");
    Serial.println("----------------------------------------");
    Serial.println(content);
    Serial.println("----------------------------------------");
}

void cmdMakeDir(const String& args) {
    if (args.isEmpty()) {
        Serial.println("Usage: mkdir <directory>");
        return;
    }
    
    String fullPath = makeFullPath(args);
    
    if (fileMgr->createDir(fullPath, currentFS)) {
        Serial.println("Directory created: " + fullPath);
    } else {
        Serial.println("Failed to create directory: " + fullPath);
    }
}

void cmdTouch(const String& args) {
    if (args.isEmpty()) {
        Serial.println("Usage: touch <filename>");
        return;
    }
    
    String fullPath = makeFullPath(args);
    
    if (fileMgr->createFile(fullPath, "", currentFS)) {
        Serial.println("File created: " + fullPath);
    } else {
        Serial.println("Failed to create file: " + fullPath);
    }
}

void cmdRemove(const String& args) {
    if (args.isEmpty()) {
        Serial.println("Usage: rm <filename>");
        return;
    }
    
    String fullPath = makeFullPath(args);
    
    if (!fileMgr->exists(fullPath, currentFS)) {
        Serial.println("File not found: " + fullPath);
        return;
    }
    
    Serial.print("Delete " + fullPath + "? (y/N): ");
    
    // Simple confirmation (in real implementation, you'd want better input handling)
    delay(100);
    if (Serial.available()) {
        String confirm = Serial.readStringUntil('\\n');
        if (confirm.startsWith("y") || confirm.startsWith("Y")) {
            if (fileMgr->deleteFile(fullPath, currentFS)) {
                Serial.println("File deleted: " + fullPath);
            } else {
                Serial.println("Failed to delete file: " + fullPath);
            }
        } else {
            Serial.println("Operation cancelled");
        }
    }
}

void cmdRemoveDir(const String& args) {
    if (args.isEmpty()) {
        Serial.println("Usage: rmdir <directory>");
        return;
    }
    
    String fullPath = makeFullPath(args);
    
    if (fileMgr->removeDir(fullPath, currentFS)) {
        Serial.println("Directory removed: " + fullPath);
    } else {
        Serial.println("Failed to remove directory: " + fullPath);
    }
}

void cmdCopy(const String& args) {
    int spaceIndex = args.indexOf(' ');
    if (spaceIndex == -1) {
        Serial.println("Usage: cp <source> <destination>");
        return;
    }
    
    String src = args.substring(0, spaceIndex);
    String dest = args.substring(spaceIndex + 1);
    
    String srcPath = makeFullPath(src);
    String destPath = makeFullPath(dest);
    
    if (!fileMgr->exists(srcPath, currentFS)) {
        Serial.println("Source file not found: " + srcPath);
        return;
    }
    
    String content = fileMgr->readFile(srcPath, currentFS);
    if (fileMgr->createFile(destPath, content, currentFS)) {
        Serial.println("File copied: " + srcPath + " -> " + destPath);
    } else {
        Serial.println("Failed to copy file");
    }
}

void cmdMove(const String& args) {
    int spaceIndex = args.indexOf(' ');
    if (spaceIndex == -1) {
        Serial.println("Usage: mv <source> <destination>");
        return;
    }
    
    String src = args.substring(0, spaceIndex);
    String dest = args.substring(spaceIndex + 1);
    
    // For simplicity, implement as copy + delete
    cmdCopy(args);
    cmdRemove(src);
}

void cmdFind(const String& args) {
    if (args.isEmpty()) {
        Serial.println("Usage: find <pattern>");
        return;
    }
    
    Serial.println("Searching for: " + args);
    findInDirectory("/", args);
}

void findInDirectory(const String& path, const String& pattern) {
    std::vector<FileInfo> files = fileMgr->listDir(path, currentFS);
    
    for (const auto& file : files) {
        String fullPath = path + (path.endsWith("/") ? "" : "/") + file.name;
        
        if (file.name.indexOf(pattern) >= 0) {
            String type = file.isDirectory ? "DIR" : "FILE";
            Serial.printf("Found %s: %s\\n", type.c_str(), fullPath.c_str());
        }
        
        if (file.isDirectory) {
            findInDirectory(fullPath, pattern);
        }
    }
}

void cmdDiskUsage(const String& args) {
    String path = args.isEmpty() ? currentPath : args;
    
    Serial.printf("Disk usage for %s:\\n", path.c_str());
    calculateDiskUsage(path);
}

void calculateDiskUsage(const String& path) {
    std::vector<FileInfo> files = fileMgr->listDir(path, currentFS);
    size_t totalSize = 0;
    int fileCount = 0;
    int dirCount = 0;
    
    for (const auto& file : files) {
        if (file.isDirectory) {
            dirCount++;
        } else {
            fileCount++;
            totalSize += file.size;
        }
    }
    
    Serial.printf("Files: %d, Directories: %d, Total size: %u bytes\\n", 
                  fileCount, dirCount, totalSize);
}

void cmdDiskFree(const String& args) {
    String fsName = (currentFS == FS_LITTLEFS) ? "LittleFS" : "SD Card";
    
    size_t total = fileMgr->getTotalBytes(currentFS);
    size_t used = fileMgr->getUsedBytes(currentFS);
    size_t free = fileMgr->getFreeBytes(currentFS);
    float usage = (used * 100.0) / total;
    
    Serial.printf("%s Storage:\\n", fsName.c_str());
    Serial.printf("Total: %u bytes (%.2f KB)\\n", total, total / 1024.0);
    Serial.printf("Used:  %u bytes (%.2f KB) [%.1f%%]\\n", used, used / 1024.0, usage);
    Serial.printf("Free:  %u bytes (%.2f KB)\\n", free, free / 1024.0);
}

void cmdHexDump(const String& args) {
    if (args.isEmpty()) {
        Serial.println("Usage: hexdump <filename>");
        return;
    }
    
    String fullPath = makeFullPath(args);
    String content = fileMgr->readFile(fullPath, currentFS);
    
    if (content.length() == 0) {
        Serial.println("File not found or empty: " + fullPath);
        return;
    }
    
    Serial.println("Hex dump of " + fullPath + ":");
    Serial.println("Offset   00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F  ASCII");
    Serial.println("-------- ------------------------- -------------------------  ----------------");
    
    for (size_t i = 0; i < content.length(); i += 16) {
        Serial.printf("%08X ", i);
        
        // Hex bytes
        for (int j = 0; j < 16; j++) {
            if (i + j < content.length()) {
                Serial.printf("%02X ", (uint8_t)content[i + j]);
            } else {
                Serial.print("   ");
            }
            if (j == 7) Serial.print(" ");
        }
        
        Serial.print(" ");
        
        // ASCII representation
        for (int j = 0; j < 16 && i + j < content.length(); j++) {
            char c = content[i + j];
            Serial.print(isprint(c) ? c : '.');
        }
        
        Serial.println();
    }
}

void cmdSwitch(const String& args) {
    if (args.isEmpty()) {
        Serial.println("Usage: switch <littlefs|sd>");
        return;
    }
    
    if (args == "littlefs" || args == "lfs") {
        currentFS = FS_LITTLEFS;
        currentPath = "/";
        Serial.println("Switched to LittleFS");
    } else if (args == "sd") {
        if (fileMgr->isSDAvailable()) {
            currentFS = FS_SD_CARD;
            currentPath = "/";
            Serial.println("Switched to SD Card");
        } else {
            Serial.println("SD Card not available");
        }
    } else {
        Serial.println("Invalid filesystem. Use 'littlefs' or 'sd'");
    }
}

void cmdHelp(const String& args) {
    Serial.println("\\nAvailable commands:");
    Serial.println("==================");
    
    for (int i = 0; i < numCommands; i++) {
        Serial.printf("%-12s - %s\\n", commands[i].name.c_str(), commands[i].description.c_str());
    }
    
    Serial.println("\\nTips:");
    Serial.println("- Use tab completion (if supported by terminal)");
    Serial.println("- Paths can be absolute (/path) or relative (path)");
    Serial.println("- Use '..' to go to parent directory");
    Serial.println("- Use 'switch' command to change between filesystems");
}

void cmdClear(const String& args) {
    Serial.println("\\033[2J\\033[H"); // ANSI clear screen
}

void cmdHistory(const String& args) {
    Serial.println("Command history:");
    for (int i = 0; i < min(historyCount, 10); i++) {
        int index = (historyCount - 10 + i) % 10;
        if (index < 0) index += 10;
        Serial.printf("%2d: %s\\n", i + 1, commandHistory[index].c_str());
    }
}

void cmdTree(const String& args) {
    String path = args.isEmpty() ? currentPath : args;
    Serial.println("Directory tree for " + path + ":");
    printTree(path, "");
}

void printTree(const String& path, const String& prefix) {
    std::vector<FileInfo> files = fileMgr->listDir(path, currentFS);
    
    for (size_t i = 0; i < files.size(); i++) {
        bool isLast = (i == files.size() - 1);
        String connector = isLast ? "└── " : "├── ";
        String icon = files[i].isDirectory ? "📁 " : "📄 ";
        
        Serial.println(prefix + connector + icon + files[i].name);
        
        if (files[i].isDirectory) {
            String nextPrefix = prefix + (isLast ? "    " : "│   ");
            String fullPath = path + (path.endsWith("/") ? "" : "/") + files[i].name;
            printTree(fullPath, nextPrefix);
        }
    }
}

void cmdStat(const String& args) {
    if (args.isEmpty()) {
        Serial.println("Usage: stat <filename>");
        return;
    }
    
    String fullPath = makeFullPath(args);
    
    if (!fileMgr->exists(fullPath, currentFS)) {
        Serial.println("File not found: " + fullPath);
        return;
    }
    
    // Get file info from directory listing
    std::vector<FileInfo> files = fileMgr->listDir(currentPath, currentFS);
    
    for (const auto& file : files) {
        if (file.name == args || file.path == fullPath) {
            Serial.println("File statistics for " + fullPath + ":");
            Serial.printf("Type: %s\\n", file.isDirectory ? "Directory" : "File");
            Serial.printf("Size: %u bytes\\n", file.size);
            Serial.printf("Path: %s\\n", file.path.c_str());
            return;
        }
    }
}

void cmdEdit(const String& args) {
    if (args.isEmpty()) {
        Serial.println("Usage: edit <filename>");
        return;
    }
    
    String fullPath = makeFullPath(args);
    String content = "";
    
    if (fileMgr->exists(fullPath, currentFS)) {
        content = fileMgr->readFile(fullPath, currentFS);
        Serial.println("Editing existing file: " + fullPath);
    } else {
        Serial.println("Creating new file: " + fullPath);
    }
    
    Serial.println("Current content:");
    Serial.println("----------------------------------------");
    Serial.println(content);
    Serial.println("----------------------------------------");
    Serial.println("Enter new content (end with '.' on a new line):");
    
    String newContent = "";
    while (true) {
        if (Serial.available()) {
            String line = Serial.readStringUntil('\\n');
            if (line == ".") break;
            newContent += line + "\\n";
        }
        delay(10);
    }
    
    if (fileMgr->writeFile(fullPath, newContent, currentFS)) {
        Serial.println("File saved successfully");
    } else {
        Serial.println("Failed to save file");
    }
}

// Helper function to make full path from relative path
String makeFullPath(const String& path) {
    if (path.startsWith("/")) {
        return path;
    } else {
        if (currentPath.endsWith("/")) {
            return currentPath + path;
        } else {
            return currentPath + "/" + path;
        }
    }
}
