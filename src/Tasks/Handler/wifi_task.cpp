#include "tasks.h"
#include "Models/WifiClient.h"

void connectToWiFi(void* param) {
	DEBUG_PRINTLN("Starting WiFi auto-connect task");
	
	// Scan for available networks
	int networkCount = wifiManager.scanNetworks();
	DEBUG_PRINTF("Found %d networks in scan\n", networkCount);

	if (networkCount == 0) {
		DEBUG_PRINTLN("No networks found, ending task");
		vTaskDelete(NULL);
		return;
	}

	// Get saved networks from database
	WifiClient* wifi = new WifiClient();
	std::vector<WifiClient*> savedNetworks = wifi->savedNetworks();
	std::vector<NetworkInfo> availableNetworks = wifiManager.getNetworkList();
	
	DEBUG_PRINTF("Found %d saved networks in database\n", savedNetworks.size());
	
	// Clean up the temporary wifi object
	delete wifi;

	// Try to connect to saved networks that are available
	bool connected = false;
	for (const auto& savedNetwork : savedNetworks) {
		String savedSSID = savedNetwork->getHostname(); // Using hostname as SSID
		String savedPassword = savedNetwork->getAttribute("password");
		
		DEBUG_PRINTF("Checking saved network: %s\n", savedSSID.c_str());
		
		// Check if this saved network is in the available networks
		bool networkFound = false;
		int32_t signalStrength = 0;
		
		for (const auto& availableNetwork : availableNetworks) {
			if (availableNetwork.ssid == savedSSID) {
				networkFound = true;
				signalStrength = availableNetwork.rssi;
				DEBUG_PRINTF("Network %s found with signal strength %d dBm\n", 
					savedSSID.c_str(), signalStrength);
				break;
			}
		}
		
		if (networkFound) {
			DEBUG_PRINTF("Attempting to connect to %s\n", savedSSID.c_str());
			displayManager.showToast("Connecting to " + savedSSID);
			
			// Attempt to connect
			bool success = wifiManager.connectToNetwork(savedSSID, savedPassword);
			
			if (success) {
				DEBUG_PRINTF("Successfully connected to %s\n", savedSSID.c_str());
				displayManager.showToast("Connected to " + savedSSID);
				connected = true;

				DEBUG_PRINTF("%s bridged accesspoin to wifi connection\n", wifiManager.enableBridgeMode() ? "Successfully" : "Failed");
				break; // Exit loop after successful connection
			} else {
				DEBUG_PRINTF("Failed to connect to %s\n", savedSSID.c_str());
			}
		} else {
			DEBUG_PRINTF("Saved network %s not in range\n", savedSSID.c_str());
		}
	}
	
	if (!connected) {
		DEBUG_PRINTLN("No saved networks available or connection failed");
		displayManager.showToast("No known networks found");
	}
	
	// Clean up saved networks
	for (auto* network : savedNetworks) {
		delete network;
	}
	
	// End task
	vTaskDelete(NULL);
}