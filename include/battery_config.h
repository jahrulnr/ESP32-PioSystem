#ifndef BATTERY_CONFIG_H
#define BATTERY_CONFIG_H

// Battery level constants
#define BATTERY_PIN            1    // GPIO pin for battery level measurement (ADC)
#define BATTERY_CHARGING_PIN   20    // GPIO pin for battery charging

// ADC configuration
#define BATTERY_ADC_RESOLUTION 4095  // 12-bit ADC (0-4095)
#define BATTERY_VOLTAGE_DIVIDER 2    // Voltage divider ratio (R1+R2)/R2 = (100k+100k)/100k = 2

// Update intervals
#define BATTERY_UPDATE_INTERVAL 60000 // Update battery level every 60 seconds

// Notification thresholds

#endif // BATTERY_CONFIG_H
