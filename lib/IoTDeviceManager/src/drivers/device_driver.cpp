#include "device_driver.h"
#include "SerialDebug.h"

bool DeviceDriver::checkEndpoint(const String& baseUrl, const String& endpoint, HttpClientManager& httpClient) {
    String url = baseUrl + endpoint;
    HttpResponse response = httpClient.get(url);
    
    // Consider 200, 401, 403 as valid responses (endpoint exists)
    return (response.statusCode == 200 || response.statusCode == 401 || response.statusCode == 403);
}

void DeviceDriver::setDeviceProperties(IoTDevice& device, DeviceType type, uint32_t capabilities, const String& name) {
    device.type = type;
    device.capabilities = capabilities;
    device.isOnline = true;
    
    if (!name.isEmpty()) {
        device.name = name;
    } else {
        device.name = DeviceTypeUtils::deviceTypeToString(type) + " (" + device.ipAddress + ")";
    }
    
    DEBUG_PRINTF("DeviceDriver: Set device %s as %s with capabilities: 0x%X\n",
                device.ipAddress.c_str(), DeviceTypeUtils::deviceTypeToString(type).c_str(), capabilities);
}
