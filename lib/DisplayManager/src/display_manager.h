#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <TFT_eSPI.h>
#include "display_config.h"

class DisplayManager {
private:
    TFT_eSPI* tft;
    bool screenOn;
    unsigned long lastActivity;
    unsigned long sleepTimeout;
    
public:
    DisplayManager(TFT_eSPI* display);
    static DisplayManager *getInstance();

    void init();
    void sleep();
    void wake();
    void updateActivity();
    bool isScreenOn();
    void checkSleepTimeout();
    void setSleepTimeout(unsigned long timeout); // Add setter for sleep timeout
    
    // Drawing utilities
    void drawStatusBar(const String& time, bool wifiConnected, bool hotspotActive, bool keyboardConnected, int batteryLevel = 85, bool isCharging = false);
    void drawProgressBar(int x, int y, int width, int height, int progress, uint16_t color);
    void drawBorder(int x, int y, int width, int height, uint16_t color);
    void clearScreen();
    void clearStatusBar();
    
    // Text utilities
    void drawCenteredText(const String& text, int y, uint16_t color, int textSize = 1);
    void drawTitle(const String& title);
    void showToast(const String& message = "-", unsigned int duration = 2000);
    
    // Getters
    TFT_eSPI* getTFT() { return tft; }
    
private:
    // Toast variables
    String toastMessage;
    unsigned long toastStartTime;
    unsigned int toastDuration;
    bool toastActive;
};

#endif // DISPLAY_MANAGER_H
