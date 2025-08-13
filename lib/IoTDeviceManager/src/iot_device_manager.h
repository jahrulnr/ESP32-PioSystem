#ifndef IOT_DEVICE_MANAGER_H
#define IOT_DEVICE_MANAGER_H

/**
 * @brief Modular IoT Device Manager
 * 
 * This is the main include file for the IoT Device Manager system.
 * It provides a clean interface to all the modular components:
 * 
 * - Types: Device types, capabilities, and data structures
 * - Drivers: Device-specific protocol handlers
 * - Discovery: Network scanning and device identification
 * - Core: Main management interface
 * 
 * Usage:
 *   #include "iot_device_manager.h"
 *   
 *   IoTDeviceManager* manager = new IoTDeviceManager(wifiManager, httpClient);
 *   manager->begin();
 *   manager->startDiscovery();
 */

// Core types and data structures
#include "types/device_types.h"

// Device driver interface and built-in drivers
#include "drivers/device_driver.h"
#include "drivers/esp32_camera_driver.h"
#include "drivers/esp32_mvc_driver.h"
#include "drivers/generic_rest_driver.h"

// Device discovery engine
#include "discovery/device_discovery.h"

// Main device manager
#include "core/iot_device_manager_core.h"

#endif // IOT_DEVICE_MANAGER_H