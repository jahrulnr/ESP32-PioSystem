#include "menus.h"
#include "iot_device_manager.h"

extern IoTDeviceManager* iotDeviceManager;

void displayIoTDevices() {
  displayManager.clearScreen();
  displayManager.drawTitle("IoT Devices");
  
  // Get discovered IoT devices
  if (iotDeviceManager == nullptr) {
    displayManager.drawCenteredText("IoT Manager not available", 50, TFT_RED);
    displayManager.drawCenteredText("Press BACK to return", 200, TFT_LIGHTGREY, 1);
    return;
  }
  
  std::vector<IoTDevice> devices = iotDeviceManager->getDiscoveredDevices();
  int y = 40;
  
  if (devices.empty()) {
    displayManager.drawCenteredText("No IoT devices found", y, TFT_YELLOW);
    y += 20;
    displayManager.drawCenteredText("Searching for devices...", y, TFT_LIGHTGREY);
    
    // Show discovery status
    y += 30;
    String statusText = "Discovery: ";
    statusText += iotDeviceManager->isDiscoveryActive() ? "ACTIVE" : "STOPPED";
    displayManager.drawCenteredText(statusText, y, TFT_CYAN);
    
    // Show scan count
    y += 15;
    displayManager.drawCenteredText("Scan #" + String(iotDeviceManager->getScanCount()), y, TFT_LIGHTGREY, 1);
  } else {
    // Display found devices
    String headerText = String(devices.size()) + " devices found";
    displayManager.drawCenteredText(headerText, y, TFT_GREEN);
    y += 25;
    
    for (size_t i = 0; i < devices.size() && i < 5; i++) {  // Limit to 5 devices for screen space
      const IoTDevice& device = devices[i];
      
      // Device number and name/IP
      String deviceName = device.name.isEmpty() ? device.ipAddress : device.name;
      String deviceInfo = String(i+1) + ". " + deviceName;
      displayManager.drawCenteredText(deviceInfo, y, TFT_WHITE);
      y += 15;
      
      // Device type and status
      String typeStatus = IoTDeviceManager::deviceTypeToString(device.type);
      typeStatus += " | ";
      typeStatus += device.isOnline ? "ONLINE" : "OFFLINE";
      uint16_t statusColor = device.isOnline ? TFT_GREEN : TFT_RED;
      displayManager.drawCenteredText(typeStatus, y, statusColor, 1);
      y += 20;
      
      // Add spacing between devices
      if (i < devices.size() - 1) {
        y += 5;
      }
    }
    
    // Show if there are more devices
    if (devices.size() > 5) {
      y += 10;
      String moreText = "+" + String(devices.size() - 5) + " more devices";
      displayManager.drawCenteredText(moreText, y, TFT_LIGHTGREY, 1);
    }
  }
  
  // Show button hints
  displayManager.drawCenteredText("BACK: Return  SELECT: List  UP/DOWN: Refresh", 200, TFT_LIGHTGREY, 1);
}

void displayIoTDeviceList() {
  displayManager.clearScreen();
  displayManager.drawTitle("Device Selection");
  
  if (iotDeviceManager == nullptr) {
    displayManager.drawCenteredText("IoT Manager not available", 50, TFT_RED);
    displayManager.drawCenteredText("Press BACK to return", 200, TFT_LIGHTGREY, 1);
    return;
  }
  
  std::vector<IoTDevice> devices = iotDeviceManager->getOnlineDevices();
  int y = 40;
  
  if (devices.empty()) {
    displayManager.drawCenteredText("No online devices found", y, TFT_YELLOW);
    y += 20;
    displayManager.drawCenteredText("Press BACK to return", y, TFT_LIGHTGREY);
    return;
  }
  
  // Display selectable device list
  String headerText = "Select device (" + String(devices.size()) + " online)";
  displayManager.drawCenteredText(headerText, y, TFT_GREEN);
  y += 25;
  
  // Show up to 6 devices on screen
  int startIndex = selectedDeviceIndex;
  int endIndex = min((int)devices.size(), startIndex + 6);
  
  // Adjust start index to show current selection
  if (selectedDeviceIndex >= 6) {
    startIndex = selectedDeviceIndex - 5;
    endIndex = min((int)devices.size(), startIndex + 6);
  }
  
  for (int i = startIndex; i < endIndex; i++) {
    const IoTDevice& device = devices[i];
    
    // Highlight selected device
    uint16_t textColor = (i == selectedDeviceIndex) ? TFT_YELLOW : TFT_WHITE;
    String prefix = (i == selectedDeviceIndex) ? "> " : "  ";
    
    // Device name or IP
    String deviceName = device.name.isEmpty() ? device.ipAddress : device.name;
    String deviceInfo = prefix + String(i+1) + ". " + deviceName;
    displayManager.drawCenteredText(deviceInfo, y, textColor);
    y += 15;
    
    // Device type and capabilities
    String typeInfo = IoTDeviceManager::deviceTypeToString(device.type);
    
    // Show capabilities for selected device
    if (i == selectedDeviceIndex) {
      std::vector<String> caps = IoTDeviceManager::capabilitiesToStrings(device.capabilities);
      if (!caps.empty()) {
        typeInfo += " | ";
        for (size_t j = 0; j < caps.size() && j < 2; j++) {
          typeInfo += caps[j];
          if (j < caps.size() - 1) typeInfo += ", ";
        }
      }
    }
    
    uint16_t typeColor = (i == selectedDeviceIndex) ? TFT_CYAN : TFT_LIGHTGREY;
    displayManager.drawCenteredText(typeInfo, y, typeColor, 1);
    y += 20;
  }
  
  // Show scroll indicators
  if (devices.size() > 6) {
    String scrollInfo = String(selectedDeviceIndex + 1) + "/" + String(devices.size());
    displayManager.drawCenteredText(scrollInfo, y + 10, TFT_LIGHTGREY, 1);
  }
  
  // Show button hints
  displayManager.drawCenteredText("UP/DOWN: Navigate  SELECT: Connect  BACK: Return", 200, TFT_LIGHTGREY, 1);
}

void displayCameraStream() {
  if (iotDeviceManager == nullptr) {
    displayManager.clearScreen();
    displayManager.drawTitle("Camera Stream");
    displayManager.drawCenteredText("IoT Manager not available", 50, TFT_RED);
    displayManager.drawCenteredText("Press BACK to return", 200, TFT_LIGHTGREY, 1);
    return;
  }
  
  std::vector<IoTDevice> onlineDevices = iotDeviceManager->getOnlineDevices();
  
  if (selectedDeviceIndex >= (int)onlineDevices.size()) {
    displayManager.clearScreen();
    displayManager.drawTitle("Camera Stream");
    displayManager.drawCenteredText("Device no longer available", 50, TFT_RED);
    displayManager.drawCenteredText("Press BACK to return", 200, TFT_LIGHTGREY, 1);
    return;
  }
  
  const IoTDevice& device = onlineDevices[selectedDeviceIndex];
  
  // Check if device has camera capability
  if (!(device.capabilities & static_cast<uint32_t>(DeviceCapability::CAMERA))) {
    displayManager.clearScreen();
    displayManager.drawTitle("Camera Stream");
    displayManager.drawCenteredText("Device has no camera", 50, TFT_RED);
    displayManager.drawCenteredText("Capability not detected", 70, TFT_YELLOW);
    displayManager.drawCenteredText("Press BACK to return", 200, TFT_LIGHTGREY, 1);
    return;
  }
  
  // Display camera stream interface
  displayManager.clearScreen();
  displayManager.drawTitle("Camera: " + device.name);
  
  int y = 40;
  displayManager.drawCenteredText("IP: " + device.ipAddress, y, TFT_CYAN, 1);
  y += 20;
  
  // Camera stream area
  int streamX = 10;
  int streamY = y + 10;
  int streamW = 220;  // 240 - 20 for margins
  int streamH = 120;   // Reasonable height for camera display
  
  // Draw stream border
  displayManager.drawBorder(streamX, streamY, streamW, streamH, TFT_WHITE);
  
  // Check if streaming is active for this device
  bool isStreaming = isCameraStreamActive() && (getCurrentStreamingDevice() == device.id);
  
  if (isStreaming) {
    // Stream is active - display will be updated by stream task
    displayManager.drawCenteredText("LIVE", streamY + streamH/2 - 10, TFT_GREEN, 2);
    displayManager.drawCenteredText("Camera Feed", streamY + streamH/2 + 10, TFT_LIGHTGREY, 1);
  } else {
    // Stream not active
    displayManager.drawCenteredText("STOPPED", streamY + streamH/2 - 10, TFT_YELLOW, 2);
    displayManager.drawCenteredText("Press UP to start", streamY + streamH/2 + 10, TFT_LIGHTGREY, 1);
  }
  
  // Control buttons area
  y = streamY + streamH + 20;
  displayManager.drawCenteredText("Controls:", y, TFT_WHITE, 1);
  y += 15;
  
  if (isStreaming) {
    displayManager.drawCenteredText("UP: Capture  DOWN: Stop", y, TFT_LIGHTGREY, 1);
    y += 12;
    displayManager.drawCenteredText("SELECT: Settings  BACK: Exit", y, TFT_LIGHTGREY, 1);
  } else {
    displayManager.drawCenteredText("UP: Start Stream  DOWN: Test", y, TFT_LIGHTGREY, 1);
    y += 12;
    displayManager.drawCenteredText("SELECT: Settings  BACK: Exit", y, TFT_LIGHTGREY, 1);
  }
}

void handleCameraStreamMenu() {
  if (iotDeviceManager == nullptr) return;
  
  std::vector<IoTDevice> onlineDevices = iotDeviceManager->getOnlineDevices();
  
  if (selectedDeviceIndex >= (int)onlineDevices.size()) {
    // Device no longer available, return to device list
    stopCameraStream(); // Stop streaming if active
    currentMenu = MENU_IOT_DEVICE_LIST;
    selectedDeviceIndex = 0;
    displayIoTDeviceList();
    return;
  }
  
  const IoTDevice& device = onlineDevices[selectedDeviceIndex];
  bool isStreaming = isCameraStreamActive() && (getCurrentStreamingDevice() == device.id);
  
  // Handle camera commands
  if (inputManager.wasPressed(BTN_UP)) {
    if (isStreaming) {
      // Manual capture during streaming
      DEBUG_PRINTF("Camera: Manual capture from device %s\n", device.ipAddress.c_str());
      
      if (triggerManualCapture()) {
        displayManager.showToast("Image captured", 1500);
      } else {
        displayManager.showToast("Capture failed", 2000);
      }
    } else {
      // Start streaming
      DEBUG_PRINTF("Camera: Starting stream from device %s\n", device.ipAddress.c_str());
      
      startCameraStream(device.id);
      displayManager.showToast("Stream started", 1500);
      
      // Refresh display to show streaming state
      vTaskDelay(pdMS_TO_TICKS(200)); // Small delay for toast
      displayCameraStream();
    }
    
    inputManager.clearButton(BTN_UP);
  }
  
  if (inputManager.wasPressed(BTN_DOWN)) {
    if (isStreaming) {
      // Stop streaming
      DEBUG_PRINTF("Camera: Stopping stream from device %s\n", device.ipAddress.c_str());
      
      stopCameraStream();
      displayManager.showToast("Stream stopped", 1500);
      
      // Refresh display to show stopped state
      vTaskDelay(pdMS_TO_TICKS(200)); // Small delay for toast
      displayCameraStream();
    } else {
      // Test single capture
      DEBUG_PRINTF("Camera: Test capture from device %s\n", device.ipAddress.c_str());
      
      JsonDocument params;
      JsonDocument result = iotDeviceManager->executeDeviceCommand(device.id, "capture", params);
      
      if (result["success"].as<bool>()) {
        displayManager.showToast("Test capture OK", 2000);
      } else {
        String error = result["error"].as<String>();
        displayManager.showToast("Test failed: " + error, 3000);
      }
    }
    
    inputManager.clearButton(BTN_DOWN);
  }
  
  if (inputManager.wasPressed(BTN_SELECT)) {
    // Camera settings (placeholder for future implementation)
    displayManager.showToast("Settings not implemented", 2000);
    inputManager.clearButton(BTN_SELECT);
  }
}
