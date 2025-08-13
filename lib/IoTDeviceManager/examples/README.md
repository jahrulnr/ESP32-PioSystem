# IoT Device Manager Examples

This directory contains examples demonstrating how to use the modular IoT Device Manager.

## Available Examples

1. **basic_usage.cpp** - Basic device discovery and management
2. **custom_driver.cpp** - Creating a custom device driver
3. **device_commands.cpp** - Executing commands on discovered devices

## Running Examples

These examples are for reference and demonstration. To use them in your project:

1. Copy the relevant code into your main application
2. Ensure you have initialized WiFiManager and HttpClientManager
3. Follow the patterns shown for your specific use case

## Integration with PioSystem

The IoT Device Manager is already integrated into the main PioSystem application in:
- `src/Tasks/init.cpp` - Initialization
- `src/Controllers/IoTDeviceController.cpp` - API endpoints
- `src/Routes/iot.cpp` - HTTP routes
