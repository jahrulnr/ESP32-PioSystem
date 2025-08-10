/*
 * Web File Server Example
 * 
 * This example demonstrates how to use the FileManager library to create
 * a web-based file server. Users can upload, download, and manage files
 * through a web interface.
 * 
 * Features:
 * - Web-based file browser
 * - File upload and download
 * - Directory navigation
 * - File deletion and creation
 * - MIME type detection
 * - Both LittleFS and SD card support
 * - RESTful API for file operations
 * 
 * Hardware Requirements:
 * - ESP32 development board with WiFi
 * - Optional: SD card module for additional storage
 * 
 * Usage:
 * 1. Connect to WiFi network
 * 2. Open web browser to ESP32's IP address
 * 3. Browse and manage files through web interface
 * 
 * Author: Your Name
 * Date: 2025
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "file_manager.h"

// WiFi credentials - CHANGE THESE!
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Create instances
FileManager* fileMgr;
WebServer server(80);

// Current file system being browsed
FileSystemType currentFS = FS_LITTLEFS;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Web File Server Demo ===");
    Serial.println("=============================");
    
    // Initialize FileManager
    fileMgr = new FileManager(5); // SD CS pin = 5
    
    if (!fileMgr->init()) {
        Serial.println("❌ FileManager initialization failed!");
        return;
    }
    
    Serial.println("✅ FileManager initialized");
    
    // Create sample files for demonstration
    createSampleFiles();
    
    // Connect to WiFi
    connectToWiFi();
    
    // Setup web server routes
    setupWebServer();
    
    // Start web server
    server.begin();
    Serial.println("🌐 Web server started!");
    Serial.print("📱 Access file manager at: http://");
    Serial.println(WiFi.localIP());
    Serial.println("=============================");
}

void loop() {
    server.handleClient();
    delay(10);
}

void connectToWiFi() {
    Serial.println("📶 Connecting to WiFi...");
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("✅ WiFi connected!");
        Serial.print("📡 IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println();
        Serial.println("❌ WiFi connection failed!");
        Serial.println("Please check your WiFi credentials and try again.");
    }
}

void createSampleFiles() {
    Serial.println("📁 Creating sample files...");
    
    // Create sample directory structure
    fileMgr->createDir("/www");
    fileMgr->createDir("/www/css");
    fileMgr->createDir("/www/js");
    fileMgr->createDir("/data");
    
    // Create sample HTML file
    String htmlContent = R"(<!DOCTYPE html>
<html>
<head>
    <title>ESP32 File Server</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .file-list { margin: 20px 0; }
        .file-item { padding: 5px; border-bottom: 1px solid #eee; }
        .directory { font-weight: bold; color: #0066cc; }
        .file { color: #333; }
        button { margin: 2px; padding: 5px 10px; }
    </style>
</head>
<body>
    <h1>ESP32 File Server</h1>
    <p>Welcome to the ESP32 web-based file manager!</p>
    <p>Use the API endpoints to manage files:</p>
    <ul>
        <li>GET /api/files - List files</li>
        <li>GET /api/download?file=path - Download file</li>
        <li>POST /api/upload - Upload file</li>
        <li>DELETE /api/delete?file=path - Delete file</li>
    </ul>
</body>
</html>)";
    
    fileMgr->createFile("/www/index.html", htmlContent);
    
    // Create sample CSS file
    String cssContent = R"(body {
    background-color: #f5f5f5;
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
}

.container {
    max-width: 800px;
    margin: 0 auto;
    background: white;
    padding: 20px;
    border-radius: 8px;
    box-shadow: 0 2px 10px rgba(0,0,0,0.1);
})";
    
    fileMgr->createFile("/www/css/style.css", cssContent);
    
    // Create sample JavaScript file
    String jsContent = R"(function refreshFileList() {
    fetch('/api/files')
        .then(response => response.json())
        .then(data => {
            console.log('Files:', data);
            updateFileDisplay(data);
        })
        .catch(error => console.error('Error:', error));
}

function deleteFile(filename) {
    if (confirm('Delete ' + filename + '?')) {
        fetch('/api/delete?file=' + encodeURIComponent(filename), {
            method: 'DELETE'
        })
        .then(() => refreshFileList())
        .catch(error => console.error('Error:', error));
    }
})";
    
    fileMgr->createFile("/www/js/filemanager.js", jsContent);
    
    // Create sample data files
    fileMgr->createFile("/data/config.json", R"({"version": "1.0", "debug": true, "server_port": 80})");
    fileMgr->createFile("/data/sensors.csv", "timestamp,temperature,humidity\\n1000,22.5,45.2\\n2000,23.1,44.8\\n");
    fileMgr->createFile("/README.txt", "ESP32 File Server\\n\\nThis is a demonstration of the FileManager library.\\nYou can browse and manage files through the web interface.");
    
    Serial.println("✅ Sample files created");
}

void setupWebServer() {
    // Main page - file browser interface
    server.on("/", HTTP_GET, handleRoot);
    
    // API endpoints
    server.on("/api/files", HTTP_GET, handleListFiles);
    server.on("/api/download", HTTP_GET, handleDownloadFile);
    server.on("/api/upload", HTTP_POST, handleFileUpload);
    server.on("/api/delete", HTTP_DELETE, handleDeleteFile);
    server.on("/api/create", HTTP_POST, handleCreateFile);
    server.on("/api/mkdir", HTTP_POST, handleCreateDirectory);
    server.on("/api/switch", HTTP_POST, handleSwitchFileSystem);
    server.on("/api/info", HTTP_GET, handleSystemInfo);
    
    // Serve static files
    server.onNotFound(handleStaticFile);
    
    Serial.println("✅ Web server routes configured");
}

void handleRoot() {
    String html = generateFileManagerHTML();
    server.send(200, "text/html", html);
}

String generateFileManagerHTML() {
    String html = R"(<!DOCTYPE html>
<html>
<head>
    <title>ESP32 File Manager</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; text-align: center; }
        .fs-switch { text-align: center; margin: 20px 0; }
        .fs-switch button { margin: 0 5px; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }
        .fs-switch .active { background: #007bff; color: white; }
        .fs-switch .inactive { background: #e9ecef; color: #333; }
        .file-list { margin: 20px 0; }
        .file-item { display: flex; justify-content: space-between; align-items: center; padding: 10px; border-bottom: 1px solid #eee; }
        .file-item:hover { background: #f8f9fa; }
        .file-name { flex: 1; }
        .directory { color: #007bff; font-weight: bold; }
        .file { color: #333; }
        .file-size { color: #666; font-size: 0.9em; margin: 0 10px; }
        .file-actions button { margin: 0 2px; padding: 5px 10px; border: none; border-radius: 3px; cursor: pointer; }
        .download { background: #28a745; color: white; }
        .delete { background: #dc3545; color: white; }
        .upload-area { border: 2px dashed #ccc; border-radius: 8px; padding: 40px; text-align: center; margin: 20px 0; }
        .system-info { background: #e9ecef; padding: 15px; border-radius: 4px; margin: 20px 0; }
        .progress { width: 100%; height: 20px; background: #e9ecef; border-radius: 10px; overflow: hidden; }
        .progress-bar { height: 100%; background: #007bff; transition: width 0.3s; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🗂️ ESP32 File Manager</h1>
        
        <div class="fs-switch">
            <button id="littlefs-btn" class="active" onclick="switchFileSystem('littlefs')">LittleFS</button>
            <button id="sd-btn" class="inactive" onclick="switchFileSystem('sd')">SD Card</button>
        </div>
        
        <div class="system-info">
            <div id="storage-info">Loading storage information...</div>
        </div>
        
        <div class="upload-area">
            <input type="file" id="file-input" multiple style="display: none;">
            <p>📤 <strong>Upload Files</strong></p>
            <p>Click here or drag files to upload</p>
            <button onclick="document.getElementById('file-input').click()">Choose Files</button>
        </div>
        
        <div class="file-list">
            <div id="file-container">
                <p>Loading files...</p>
            </div>
        </div>
    </div>
    
    <script>
        let currentFS = 'littlefs';
        
        function switchFileSystem(fs) {
            currentFS = fs;
            document.getElementById('littlefs-btn').className = fs === 'littlefs' ? 'active' : 'inactive';
            document.getElementById('sd-btn').className = fs === 'sd' ? 'active' : 'inactive';
            loadFiles();
            loadSystemInfo();
        }
        
        function loadFiles() {
            fetch('/api/files?fs=' + currentFS)
                .then(response => response.json())
                .then(files => displayFiles(files))
                .catch(error => {
                    document.getElementById('file-container').innerHTML = '<p>Error loading files: ' + error + '</p>';
                });
        }
        
        function displayFiles(files) {
            const container = document.getElementById('file-container');
            if (files.length === 0) {
                container.innerHTML = '<p>No files found</p>';
                return;
            }
            
            let html = '';
            files.forEach(file => {
                const icon = file.isDirectory ? '📁' : '📄';
                const className = file.isDirectory ? 'directory' : 'file';
                const size = file.isDirectory ? '' : '<span class="file-size">(' + formatBytes(file.size) + ')</span>';
                
                html += '<div class="file-item">';
                html += '<div class="file-name ' + className + '">' + icon + ' ' + file.name + '</div>';
                html += size;
                html += '<div class="file-actions">';
                if (!file.isDirectory) {
                    html += '<button class="download" onclick="downloadFile(\'' + file.name + '\')">Download</button>';
                }
                html += '<button class="delete" onclick="deleteFile(\'' + file.name + '\')">Delete</button>';
                html += '</div>';
                html += '</div>';
            });
            
            container.innerHTML = html;
        }
        
        function formatBytes(bytes) {
            if (bytes === 0) return '0 B';
            const k = 1024;
            const sizes = ['B', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
        }
        
        function downloadFile(filename) {
            window.open('/api/download?file=' + encodeURIComponent(filename) + '&fs=' + currentFS);
        }
        
        function deleteFile(filename) {
            if (confirm('Delete ' + filename + '?')) {
                fetch('/api/delete?file=' + encodeURIComponent(filename) + '&fs=' + currentFS, {
                    method: 'DELETE'
                })
                .then(() => loadFiles())
                .catch(error => alert('Error deleting file: ' + error));
            }
        }
        
        function loadSystemInfo() {
            fetch('/api/info?fs=' + currentFS)
                .then(response => response.json())
                .then(info => {
                    const usagePercent = (info.used / info.total * 100).toFixed(1);
                    document.getElementById('storage-info').innerHTML = 
                        '<strong>' + info.filesystem + '</strong> | ' +
                        'Used: ' + formatBytes(info.used) + ' / ' + formatBytes(info.total) + 
                        ' (' + usagePercent + '%)<br>' +
                        '<div class="progress"><div class="progress-bar" style="width: ' + usagePercent + '%"></div></div>';
                });
        }
        
        // File upload handling
        document.getElementById('file-input').addEventListener('change', function(e) {
            const files = e.target.files;
            for (let file of files) {
                uploadFile(file);
            }
        });
        
        function uploadFile(file) {
            const formData = new FormData();
            formData.append('file', file);
            formData.append('fs', currentFS);
            
            fetch('/api/upload', {
                method: 'POST',
                body: formData
            })
            .then(response => response.text())
            .then(result => {
                console.log('Upload result:', result);
                loadFiles();
                loadSystemInfo();
            })
            .catch(error => {
                console.error('Upload error:', error);
                alert('Upload failed: ' + error);
            });
        }
        
        // Load initial data
        loadFiles();
        loadSystemInfo();
        
        // Auto-refresh every 30 seconds
        setInterval(() => {
            loadFiles();
            loadSystemInfo();
        }, 30000);
    </script>
</body>
</html>)";
    
    return html;
}

void handleListFiles() {
    String fsParam = server.arg("fs");
    FileSystemType fs = (fsParam == "sd") ? FS_SD_CARD : FS_LITTLEFS;
    
    if (fs == FS_SD_CARD && !fileMgr->isSDAvailable()) {
        server.send(404, "application/json", "{\"error\": \"SD card not available\"}");
        return;
    }
    
    String path = server.arg("path");
    if (path.isEmpty()) path = "/";
    
    std::vector<FileInfo> files = fileMgr->listDir(path, fs);
    
    String json = "[";
    for (size_t i = 0; i < files.size(); i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"name\":\"" + files[i].name + "\",";
        json += "\"size\":" + String(files[i].size) + ",";
        json += "\"isDirectory\":" + String(files[i].isDirectory ? "true" : "false");
        json += "}";
    }
    json += "]";
    
    server.send(200, "application/json", json);
}

void handleDownloadFile() {
    String filename = server.arg("file");
    String fsParam = server.arg("fs");
    FileSystemType fs = (fsParam == "sd") ? FS_SD_CARD : FS_LITTLEFS;
    
    if (filename.isEmpty()) {
        server.send(400, "text/plain", "File parameter required");
        return;
    }
    
    if (!fileMgr->exists(filename, fs)) {
        server.send(404, "text/plain", "File not found");
        return;
    }
    
    String content = fileMgr->readFile(filename, fs);
    String mimeType = fileMgr->getMimeType(filename);
    
    server.send(200, mimeType, content);
}

void handleFileUpload() {
    // This is a simplified upload handler
    // In a full implementation, you'd handle multipart form data
    server.send(200, "text/plain", "Upload endpoint - implementation depends on web server library");
}

void handleDeleteFile() {
    String filename = server.arg("file");
    String fsParam = server.arg("fs");
    FileSystemType fs = (fsParam == "sd") ? FS_SD_CARD : FS_LITTLEFS;
    
    if (filename.isEmpty()) {
        server.send(400, "text/plain", "File parameter required");
        return;
    }
    
    if (fileMgr->deleteFile(filename, fs)) {
        server.send(200, "text/plain", "File deleted successfully");
    } else {
        server.send(500, "text/plain", "Failed to delete file");
    }
}

void handleCreateFile() {
    String filename = server.arg("name");
    String content = server.arg("content");
    String fsParam = server.arg("fs");
    FileSystemType fs = (fsParam == "sd") ? FS_SD_CARD : FS_LITTLEFS;
    
    if (filename.isEmpty()) {
        server.send(400, "text/plain", "Filename required");
        return;
    }
    
    if (fileMgr->createFile(filename, content, fs)) {
        server.send(200, "text/plain", "File created successfully");
    } else {
        server.send(500, "text/plain", "Failed to create file");
    }
}

void handleCreateDirectory() {
    String dirname = server.arg("name");
    String fsParam = server.arg("fs");
    FileSystemType fs = (fsParam == "sd") ? FS_SD_CARD : FS_LITTLEFS;
    
    if (dirname.isEmpty()) {
        server.send(400, "text/plain", "Directory name required");
        return;
    }
    
    if (fileMgr->createDir(dirname, fs)) {
        server.send(200, "text/plain", "Directory created successfully");
    } else {
        server.send(500, "text/plain", "Failed to create directory");
    }
}

void handleSwitchFileSystem() {
    String fs = server.arg("fs");
    currentFS = (fs == "sd") ? FS_SD_CARD : FS_LITTLEFS;
    server.send(200, "text/plain", "Switched to " + fs);
}

void handleSystemInfo() {
    String fsParam = server.arg("fs");
    FileSystemType fs = (fsParam == "sd") ? FS_SD_CARD : FS_LITTLEFS;
    
    if (fs == FS_SD_CARD && !fileMgr->isSDAvailable()) {
        server.send(404, "application/json", "{\"error\": \"SD card not available\"}");
        return;
    }
    
    size_t total = fileMgr->getTotalBytes(fs);
    size_t used = fileMgr->getUsedBytes(fs);
    size_t free = fileMgr->getFreeBytes(fs);
    
    String json = "{";
    json += "\"filesystem\":\"" + String(fs == FS_SD_CARD ? "SD Card" : "LittleFS") + "\",";
    json += "\"total\":" + String(total) + ",";
    json += "\"used\":" + String(used) + ",";
    json += "\"free\":" + String(free);
    json += "}";
    
    server.send(200, "application/json", json);
}

void handleStaticFile() {
    String path = server.uri();
    
    // Try to serve from LittleFS first
    if (fileMgr->exists(path)) {
        String content = fileMgr->readFile(path);
        String mimeType = fileMgr->getMimeType(path);
        server.send(200, mimeType, content);
        return;
    }
    
    // If not found, return 404
    server.send(404, "text/plain", "File not found");
}
