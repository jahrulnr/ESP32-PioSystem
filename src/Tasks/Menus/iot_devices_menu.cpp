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
  displayManager.drawCenteredText("BACK: Return  SELECT: Refresh", 200, TFT_LIGHTGREY, 1);
}
