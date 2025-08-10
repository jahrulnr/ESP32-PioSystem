#include "battery_manager.h"
#include "SerialDebug.h"

#define BATTERY_CRITICAL   10    // Critical battery level
#define BATTERY_LOW        25    // Low battery level
#define BATTERY_MEDIUM     50    // Medium battery level
#define BATTERY_HIGH       75    // High battery level
#define BATTERY_SAMPLES    10    // Number of samples to average for stable reading

#define BATTERY_NOTIFY_CRITICAL true  // Notify when battery is critical
#define BATTERY_NOTIFY_LOW      true  // Notify when battery is low

BatteryManager::BatteryManager() {
    voltageMax = 4.2;
    voltageMin = 3.3;
    voltageDivider = 2;
    adcResolution = 4095.0; // 12-bit ADC
    batteryPin = 1;
    chargePin = -1;
    updateInterval = 5000; // 5 seconds

    lastUpdate = 0;
    currentVoltage = 0;
    currentLevel = 0;
    currentState = BATTERY_STATE_CRITICAL;
    chargingState = CHARGING_UNKNOWN;
    
    notifyCritical = BATTERY_NOTIFY_CRITICAL;
    notifyLow = BATTERY_NOTIFY_LOW;
    wasLowNotified = false;
    wasCriticalNotified = false;
}

BatteryManager::~BatteryManager() {
}

void BatteryManager::init(int pin){
    setPin(pin);
    setup();
}

void BatteryManager::setup() {
    DEBUG_PRINTLN("BatteryManager: Initializing...");
    
    // Configure ADC
    analogReadResolution(12); // Set ADC resolution to 12 bits (0-4095)
    adcResolution = 4095.0;   // 12-bit ADC = 2^12 - 1
    
    // Get initial readings
    update();
    
    DEBUG_PRINTLN("BatteryManager: Initialization complete");
    printStatus();
}

void BatteryManager::setVoltage(float min, float max, float divider){
    // Update member variables
    setVoltageMin(min);
    setVoltageMax(max);
    setVoltageDivider(divider);
}

void BatteryManager::update() {
    unsigned long currentTime = millis();
    
    // Only update at the specified interval
    if ((currentTime - lastUpdate) >= updateInterval) {
        // Read and calculate battery voltage and level
        currentVoltage = readVoltage();
        currentLevel = calculateLevel(currentVoltage);
        BatteryState newState = determineState(currentLevel);
        
        // Check if state has changed
        if (newState != currentState) {
            currentState = newState;
            
            // Handle notifications for low and critical states
            if (currentState == BATTERY_STATE_CRITICAL && notifyCritical && !wasCriticalNotified) {
                // Critical battery notification
                DEBUG_PRINTLN("BatteryManager: CRITICAL BATTERY LEVEL!");
                wasCriticalNotified = true;
            } 
            else if (currentState == BATTERY_STATE_LOW && notifyLow && !wasLowNotified) {
                // Low battery notification
                DEBUG_PRINTLN("BatteryManager: Low battery level");
                wasLowNotified = true;
            }
            
            // Reset notification flags if battery level improved
            if (currentState > BATTERY_STATE_LOW) {
                wasLowNotified = false;
            }
            if (currentState > BATTERY_STATE_CRITICAL) {
                wasCriticalNotified = false;
            }
        }
        
        // Update timestamp
        lastUpdate = currentTime;
    }
}

float BatteryManager::readVoltage() {
    // Take multiple samples to stabilize reading
    long sum = 0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
        sum += analogRead(batteryPin);
        delay(10); // Increased delay for better ADC stabilization
    }
    
    // Average the readings
    float rawValue = (float)sum / BATTERY_SAMPLES;
    
    // Convert ADC reading to voltage (considering voltage divider)
    // First calculate the voltage at the ADC pin: ADC value * (3.3V reference / resolution)
    float adcVoltage = rawValue * (3.3 / adcResolution);
    
    // Then calculate the actual battery voltage using the voltage divider formula
    // For two equal resistors (100k), the voltage is doubled from what the ADC reads
    float voltage = adcVoltage * voltageDivider;
    
    DEBUG_PRINTF("BatteryManager: Raw ADC: %.0f, ADC Voltage: %.2fV, Voltage: %.2fV, Level: %d%%\n", 
                 rawValue, adcVoltage, voltage, calculateLevel(voltage));
    
    return voltage;
}

int BatteryManager::calculateLevel(float voltage) {
    // Calculate battery percentage based on voltage
    // Linear mapping from min voltage (0%) to max voltage (100%)
    if (voltage <= voltageMin) return 0;
    if (voltage >= voltageMax) return 100;
    
    // Linear interpolation
    int level = (int)(((voltage - voltageMin) / (voltageMax - voltageMin)) * 100.0);
    return constrain(level, 0, 100); // Ensure level is between 0-100
}

BatteryState BatteryManager::determineState(int level) {
    // Determine battery state based on percentage
    if (level <= BATTERY_CRITICAL) return BATTERY_STATE_CRITICAL;
    if (level <= BATTERY_LOW) return BATTERY_STATE_LOW;
    if (level <= BATTERY_MEDIUM) return BATTERY_STATE_MEDIUM;
    if (level <= BATTERY_HIGH) return BATTERY_STATE_HIGH;
    return BATTERY_STATE_FULL;
}

void BatteryManager::setUpdateInterval(unsigned long interval) {
    updateInterval = interval;
}

void BatteryManager::setChargingState(ChargingState state) {
    chargingState = state;
}

int BatteryManager::getBatteryIconIndex() const {
    // Return icon index based on battery state and charging state
    if (chargingState == CHARGING_IN_PROGRESS) {
        return 5; // Charging icon
    }
    
    // Return icon based on battery level (0-4)
    switch (currentState) {
        case BATTERY_STATE_CRITICAL: return 0;
        case BATTERY_STATE_LOW:      return 1;
        case BATTERY_STATE_MEDIUM:   return 2;
        case BATTERY_STATE_HIGH:     return 3;
        case BATTERY_STATE_FULL:     return 4;
        default:                     return 2; // Medium as default
    }
}

void BatteryManager::printStatus() const {
    DEBUG_PRINTLN("======== Battery Status ========");
    DEBUG_PRINTF("Voltage: %.2fV\n", currentVoltage);
    DEBUG_PRINTF("Level: %d%%\n", currentLevel);
    
    // Print state
    DEBUG_PRINT("State: ");
    switch (currentState) {
        case BATTERY_STATE_CRITICAL: DEBUG_PRINTLN("CRITICAL"); break;
        case BATTERY_STATE_LOW:      DEBUG_PRINTLN("LOW"); break;
        case BATTERY_STATE_MEDIUM:   DEBUG_PRINTLN("MEDIUM"); break;
        case BATTERY_STATE_HIGH:     DEBUG_PRINTLN("HIGH"); break;
        case BATTERY_STATE_FULL:     DEBUG_PRINTLN("FULL"); break;
        default:                     DEBUG_PRINTLN("UNKNOWN"); break;
    }
    
    // Print charging state
    DEBUG_PRINT("Charging: ");
    switch (chargingState) {
        case CHARGING_NOT_CONNECTED: DEBUG_PRINTLN("Not connected"); break;
        case CHARGING_IN_PROGRESS:   DEBUG_PRINTLN("In progress"); break;
        case CHARGING_COMPLETE:      DEBUG_PRINTLN("Complete"); break;
        default:                     DEBUG_PRINTLN("Unknown"); break;
    }
    
    // Print calibration info
    DEBUG_PRINTF("Voltage range: %.2fV - %.2fV\n", voltageMin, voltageMax);
    DEBUG_PRINTF("Voltage divider: %.2f\n", voltageDivider);
    DEBUG_PRINTLN("==============================");
}
