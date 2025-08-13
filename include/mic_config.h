#ifndef MICROPHONE_CONFIG_H
#define MICROPHONE_CONFIG_H

#define MIC_ENABLED true

// Analog microphone settings (MAX9814 or similar)
#define MIC_ANALOG_PIN 2              // ADC pin for analog mic
#define MIC_AR_PIN 1                  // Attack/Release pin (optional)
#define MIC_GAIN_PIN 39               // Gain control pin (optional)

// Common mic settings
#define MIC_SAMPLE_RATE 8000          // Sample rate in Hz
#define MIC_BITS_PER_SAMPLE 16        // Bits per sample
#define MIC_TASK_STACK_SIZE 1024 * 20  // Stack size for microphone task

#endif
