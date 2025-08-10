/*
 * Example Button Configuration
 * 
 * Copy this to your main project and adjust values for your hardware setup.
 * This file shows common configurations for different button setups.
 */

#ifndef EXAMPLE_BUTTON_CONFIG_H
#define EXAMPLE_BUTTON_CONFIG_H

// ===== PIN CONFIGURATIONS =====

// --- Standard ESP32 Development Board ---
#define STANDARD_UP     4
#define STANDARD_DOWN   5
#define STANDARD_SELECT 9
#define STANDARD_BACK   6

// --- ESP32-S3 Development Board ---
#define S3_UP     1
#define S3_DOWN   2
#define S3_SELECT 3
#define S3_BACK   4

// --- Custom PCB Layout ---
#define CUSTOM_UP     12
#define CUSTOM_DOWN   13
#define CUSTOM_SELECT 14
#define CUSTOM_BACK   15

// --- GPIO Matrix Layout ---
#define MATRIX_UP     21
#define MATRIX_DOWN   22
#define MATRIX_SELECT 23
#define MATRIX_BACK   25

// ===== TIMING CONFIGURATIONS =====

// Debounce settings (in milliseconds)
#define FAST_DEBOUNCE    20    // High-quality buttons
#define NORMAL_DEBOUNCE  50    // Standard buttons (default)
#define SLOW_DEBOUNCE    100   // Poor quality/noisy buttons

// Hold threshold settings (in milliseconds)
#define QUICK_HOLD     500     // 0.5 seconds - responsive
#define NORMAL_HOLD    1000    // 1.0 seconds - standard
#define LONG_HOLD      2000    // 2.0 seconds - deliberate

// ===== BUTTON TYPE CONFIGURATIONS =====

// --- Tactile Push Buttons ---
// Good quality tactile switches
#define TACTILE_DEBOUNCE    FAST_DEBOUNCE
#define TACTILE_HOLD        NORMAL_HOLD

// --- Membrane Buttons ---
// Flat membrane switches (often in panels)
#define MEMBRANE_DEBOUNCE   NORMAL_DEBOUNCE
#define MEMBRANE_HOLD       LONG_HOLD

// --- Micro Switches ---
// High-quality micro switches
#define MICRO_DEBOUNCE      FAST_DEBOUNCE
#define MICRO_HOLD          QUICK_HOLD

// --- Cheap Push Buttons ---
// Low-cost buttons with potential bouncing
#define CHEAP_DEBOUNCE      SLOW_DEBOUNCE
#define CHEAP_HOLD          NORMAL_HOLD

// ===== APPLICATION-SPECIFIC CONFIGURATIONS =====

// --- Gaming Controller ---
// Fast, responsive gaming interface
/*
#define BUTTON_UP         STANDARD_UP
#define BUTTON_DOWN       STANDARD_DOWN
#define BUTTON_SELECT     STANDARD_SELECT
#define BUTTON_BACK       STANDARD_BACK
#define DEBOUNCE_DELAY    FAST_DEBOUNCE
#define HOLD_THRESHOLD    QUICK_HOLD
*/

// --- Industrial Control Panel ---
// Reliable, deliberate control interface
/*
#define BUTTON_UP         CUSTOM_UP
#define BUTTON_DOWN       CUSTOM_DOWN
#define BUTTON_SELECT     CUSTOM_SELECT
#define BUTTON_BACK       CUSTOM_BACK
#define DEBOUNCE_DELAY    NORMAL_DEBOUNCE
#define HOLD_THRESHOLD    LONG_HOLD
*/

// --- Menu Navigation System ---
// Standard menu interface
/*
#define BUTTON_UP         STANDARD_UP
#define BUTTON_DOWN       STANDARD_DOWN
#define BUTTON_SELECT     STANDARD_SELECT
#define BUTTON_BACK       STANDARD_BACK
#define DEBOUNCE_DELAY    NORMAL_DEBOUNCE
#define HOLD_THRESHOLD    NORMAL_HOLD
*/

// --- Accessibility Interface ---
// Longer holds for users with motor difficulties
/*
#define BUTTON_UP         MATRIX_UP
#define BUTTON_DOWN       MATRIX_DOWN
#define BUTTON_SELECT     MATRIX_SELECT
#define BUTTON_BACK       MATRIX_BACK
#define DEBOUNCE_DELAY    SLOW_DEBOUNCE
#define HOLD_THRESHOLD    LONG_HOLD
*/

// ===== SPECIAL FEATURES =====

// Enable/disable advanced features
#define ENABLE_MULTI_PRESS      1    // Double-click, triple-click detection
#define ENABLE_SEQUENCES        1    // Button sequence detection
#define ENABLE_COMBINATIONS     1    // Multi-button combinations
#define ENABLE_STATISTICS       1    // Usage statistics tracking

// Multi-press timing
#define MULTI_PRESS_TIMEOUT     300  // Max time between presses (ms)
#define MAX_MULTI_PRESSES       3    // Maximum clicks to detect

// Sequence detection
#define SEQUENCE_TIMEOUT        3000 // Max time for sequence (ms)
#define MAX_SEQUENCE_LENGTH     8    // Maximum sequence length

// ===== HARDWARE PULL-UP CONFIGURATION =====

// Internal pull-up settings
#define USE_INTERNAL_PULLUP     1    // Use ESP32 internal pull-ups
#define PULLUP_RESISTANCE       45000 // Internal ~45kΩ (informational)

// External pull-up settings (if used)
#define USE_EXTERNAL_PULLUP     0    // Use external pull-up resistors
#define EXTERNAL_PULLUP_VALUE   10000 // 10kΩ recommended

// ===== SAFETY AND VALIDATION =====

// Pin validation
#if defined(BUTTON_UP) && (BUTTON_UP < 0 || BUTTON_UP > 48)
#error "BUTTON_UP pin out of valid range (0-48)"
#endif

#if defined(BUTTON_DOWN) && (BUTTON_DOWN < 0 || BUTTON_DOWN > 48)
#error "BUTTON_DOWN pin out of valid range (0-48)"
#endif

#if defined(BUTTON_SELECT) && (BUTTON_SELECT < 0 || BUTTON_SELECT > 48)
#error "BUTTON_SELECT pin out of valid range (0-48)"
#endif

#if defined(BUTTON_BACK) && (BUTTON_BACK < 0 || BUTTON_BACK > 48)
#error "BUTTON_BACK pin out of valid range (0-48)"
#endif

// Timing validation
#if defined(DEBOUNCE_DELAY) && (DEBOUNCE_DELAY < 10 || DEBOUNCE_DELAY > 500)
#warning "DEBOUNCE_DELAY outside recommended range (10-500ms)"
#endif

#if defined(HOLD_THRESHOLD) && HOLD_THRESHOLD < 100
#warning "HOLD_THRESHOLD very short, may cause false holds"
#endif

// Pin conflict detection
#if defined(BUTTON_UP) && defined(BUTTON_DOWN) && (BUTTON_UP == BUTTON_DOWN)
#error "BUTTON_UP and BUTTON_DOWN cannot use the same pin"
#endif

#if defined(BUTTON_SELECT) && defined(BUTTON_BACK) && (BUTTON_SELECT == BUTTON_BACK)
#error "BUTTON_SELECT and BUTTON_BACK cannot use the same pin"
#endif

// ===== DEFAULT CONFIGURATION =====
// Use standard ESP32 development board layout if nothing specified

#ifndef BUTTON_UP
#define BUTTON_UP STANDARD_UP
#endif

#ifndef BUTTON_DOWN
#define BUTTON_DOWN STANDARD_DOWN
#endif

#ifndef BUTTON_SELECT
#define BUTTON_SELECT STANDARD_SELECT
#endif

#ifndef BUTTON_BACK
#define BUTTON_BACK STANDARD_BACK
#endif

#ifndef DEBOUNCE_DELAY
#define DEBOUNCE_DELAY NORMAL_DEBOUNCE
#endif

// ===== USAGE EXAMPLES =====

/*
Example 1: Gaming Setup
#define BUTTON_UP     4
#define BUTTON_DOWN   5
#define BUTTON_SELECT 9
#define BUTTON_BACK   6
#define DEBOUNCE_DELAY 20
*/

/*
Example 2: Industrial Control
#define BUTTON_UP     12
#define BUTTON_DOWN   13
#define BUTTON_SELECT 14
#define BUTTON_BACK   15
#define DEBOUNCE_DELAY 100
*/

/*
Example 3: Accessibility Interface
#define BUTTON_UP     21
#define BUTTON_DOWN   22
#define BUTTON_SELECT 23
#define BUTTON_BACK   25
#define DEBOUNCE_DELAY 150
*/

#endif // EXAMPLE_BUTTON_CONFIG_H
