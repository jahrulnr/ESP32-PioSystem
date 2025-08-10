# WiFiManager

A comprehensive WiFi management library for ESP32 microcontrollers, providing smartphone-like WiFi features:

- Scanning for available networks
- Connecting to selected networks
- Creating access points (hotspots)
- Bridging WiFi client and AP modes
- Saving and managing multiple network credentials

## Features

- **Network Scanning**: List all available WiFi networks with signal strength
- **Network Management**: Connect to and disconnect from WiFi networks
- **Access Point**: Create a hotspot with customizable SSID and password
- **Bridge Mode**: Connect to a WiFi network while also serving as an access point
- **Credential Storage**: Save multiple WiFi credentials for automatic reconnection
- **Event Handling**: Register callbacks for WiFi events

## Installation

### PlatformIO

1. Open your project in PlatformIO
2. Add the library to your `platformio.ini`:
   ```ini
   lib_deps =
     # Other dependencies...
     https://github.com/yourusername/WiFiManager.git
   ```

### Arduino IDE

1. Download this repository as a ZIP file
2. In the Arduino IDE, go to Sketch → Include Library → Add .ZIP Library
3. Select the downloaded ZIP file

## Quick Start

```cpp
#include <Arduino.h>
#include "wifi_manager.h"

WiFiManager wifiManager("MyESP32_AP", "password123");

void setup() {
  Serial.begin(115200);
  
  // Initialize in station mode
  wifiManager.begin(WIFI_STA);
  
  // Scan for networks
  int networksFound = wifiManager.scanNetworks();
  Serial.printf("Found %d networks\n", networksFound);
  
  // List all networks
  std::vector<NetworkInfo> networks = wifiManager.getNetworkList();
  for (auto& network : networks) {
    Serial.printf("SSID: %s, Signal: %d dBm\n", network.ssid.c_str(), network.rssi);
  }
  
  // Connect to a specific network
  if (wifiManager.connectToNetwork("YourHomeWiFi", "wifi_password")) {
    Serial.println("Connected successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  
  // Enable hotspot in bridge mode (both connected to WiFi and serving as AP)
  wifiManager.startAccessPoint();
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  // Your code here
}
```

## API Reference

See the header file for detailed documentation of all available methods.

## Examples

Check the examples folder for more detailed usage examples:

- BasicWiFiManager: Connect to a WiFi network or create an AP
- BridgeMode: Connect to WiFi while also serving as an access point
- SavedNetworks: Save and manage multiple network credentials
- EventHandling: Register callbacks for WiFi events

## License

This library is licensed under the MIT License.
