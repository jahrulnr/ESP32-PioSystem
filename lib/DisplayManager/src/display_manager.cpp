#include "display_manager.h"
#include "cpu_freq.h"

DisplayManager *displayManagerInstance = nullptr;

DisplayManager::DisplayManager(TFT_eSPI* display) {
    tft = display;
    screenOn = true;
    lastActivity = 0;
    toastActive = false;
    sleepTimeout = SLEEP_TIMEOUT; // Initialize with default value
    displayManagerInstance = this;
}

DisplayManager *DisplayManager::getInstance() {
    return displayManagerInstance;
}

void DisplayManager::init() {
    tft->init();
    tft->setRotation(1);
    tft->setSwapBytes(true);
    clearScreen();
    detachInterrupt(TFT_BL);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    lastActivity = millis();
}

void DisplayManager::sleep() {
    setCPU(CPU_LOW);
    screenOn = false;
    clearScreen();
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(2);
    drawCenteredText("SLEEP", tft->height() / 2 - 10, COLOR_TEXT, 2);
    tft->setTextSize(1);
    drawCenteredText("Press any button", tft->height() / 2 + 20, COLOR_TEXT, 1);
    delay(1000);
    tft->writecommand(TFT_DISPOFF);
    digitalWrite(TFT_BL, LOW);
}

void DisplayManager::wake() {
    setCPU(CPU_MEDIUM);
    screenOn = true;
    tft->writecommand(TFT_DISPON);
    updateActivity();
    digitalWrite(TFT_BL, HIGH);
}

bool DisplayManager::isScreenOn() {
    return screenOn;
}

void DisplayManager::updateActivity() {
    lastActivity = millis();
}

void DisplayManager::checkSleepTimeout() {
    if (screenOn && (millis() - lastActivity > sleepTimeout)) {
        sleep();
    }
    
    // Check if toast needs to be removed
    if (toastActive && (millis() - toastStartTime > toastDuration)) {
        toastActive = false;
    }
}

void DisplayManager::setSleepTimeout(unsigned long timeout) {
    // Set new sleep timeout in milliseconds
    sleepTimeout = timeout;
}

void DisplayManager::clearStatusBar() {
    // Clear status bar
    tft->fillRect(0, 0, tft->width(), 16, COLOR_TITLE);
    tft->setTextColor(COLOR_BG);
    tft->setTextSize(1);
}

void DisplayManager::drawStatusBar(const String& time, bool wifiConnected, bool hotspotActive, bool keyboardConnected, int batteryLevel, bool isCharging) {
    clearStatusBar();
    
    // Time
    // Calculate position to center the time
    int timeWidth = time.length() * 6; // 6 pixels per character at size 1
    int timeX = (tft->width() / 2) - (timeWidth / 2);
    tft->drawString(time, timeX, 4);
    
    // Keyboard indicator
    if (keyboardConnected) {
        tft->drawString("KB", 60, 4);
    }
    
    // Battery indicator
    int battWidth = 25;
    int battHeight = 12;
    int battX = tft->width() - 30;
    int battY = 2;
    
    // Choose battery color based on level
    uint16_t batteryColor = COLOR_BG;
    if (batteryLevel <= 10) {
        batteryColor = TFT_RED; // Critical
    } else if (batteryLevel <= 25) {
        batteryColor = TFT_ORANGE; // Low
    } else if (batteryLevel <= 50) {
        batteryColor = TFT_YELLOW; // Medium
    } else {
        batteryColor = TFT_GREEN; // Good
    }
    
    // Draw battery outline
    tft->drawRect(battX, battY, battWidth, battHeight, COLOR_BG);
    
    // Fill battery based on level
    tft->fillRect(battX + 1, battY + 1, (batteryLevel * (battWidth - 2)) / 100, battHeight - 2, batteryColor);
    
    // Draw battery terminal
    tft->fillRect(battX + battWidth, battY + 3, 2, battHeight - 6, COLOR_BG);
    
    // Show charging indicator if needed
    if (isCharging) {
        // Draw lightning bolt or charging symbol
        tft->drawLine(battX + battWidth/2 - 2, battY + 2, battX + battWidth/2 + 2, battY + battHeight/2, TFT_YELLOW);
        tft->drawLine(battX + battWidth/2 + 2, battY + battHeight/2, battX + battWidth/2 - 2, battY + battHeight - 2, TFT_YELLOW);
    }
    
    // WiFi indicator - improved with signal strength bars
    if (wifiConnected) {
        // Draw signal strength bars (3 bars)
        int wifiX = tft->width() - 50;
        int wifiY = 4;
        
        // Draw 3 bars with increasing height
        for (int i = 0; i < 3; i++) {
            int barHeight = 3 + (i * 2); // Increasing heights: 3, 5, 7
            int barWidth = 2;
            int barX = wifiX + (i * 4);
            int barY = wifiY + (7 - barHeight);
            
            tft->fillRect(barX, barY, barWidth, barHeight, COLOR_BG);
        }
    } else {
        // Draw an empty wifi icon (outline)
        tft->drawTriangle(tft->width() - 50, 2, tft->width() - 40, 2, tft->width() - 45, 12, COLOR_BG);
        tft->drawLine(tft->width() - 48, 4, tft->width() - 42, 4, COLOR_BG);
    }
    
    // Hotspot indicator - improved circular icon
    if (hotspotActive) {
        int apX = tft->width() - 65;
        int apY = 8;
        int apRadius = 6;
        
        // Draw AP symbol (concentric circles with radiating lines)
        tft->drawCircle(apX, apY, apRadius, COLOR_BG);
        tft->drawCircle(apX, apY, apRadius-3, COLOR_BG);
        tft->fillCircle(apX, apY, 1, COLOR_BG);
        
        // Draw radiating lines
        for (int i = 0; i < 4; i++) {
            float angle = i * PI / 2; // 0, 90, 180, 270 degrees
            int x1 = apX + (apRadius-3) * cos(angle);
            int y1 = apY + (apRadius-3) * sin(angle);
            int x2 = apX + apRadius * cos(angle);
            int y2 = apY + apRadius * sin(angle);
            
            tft->drawLine(x1, y1, x2, y2, COLOR_BG);
        }
    }

    showToast();
}

void DisplayManager::clearScreen() {
    tft->fillScreen(COLOR_BG);
    clearStatusBar();
}

void DisplayManager::drawProgressBar(int x, int y, int width, int height, int progress, uint16_t color) {
    tft->drawRect(x, y, width, height, COLOR_BORDER);
    int fillWidth = (progress * (width - 2)) / 100;
    tft->fillRect(x + 1, y + 1, fillWidth, height - 2, color);
}

void DisplayManager::drawBorder(int x, int y, int width, int height, uint16_t color) {
    tft->drawRect(x, y, width, height, color);
}

void DisplayManager::drawCenteredText(const String& text, int y, uint16_t color, int textSize) {
    tft->setTextColor(color);
    tft->setTextSize(textSize);
    int textWidth = text.length() * 6 * textSize;
    int x = (tft->width() - textWidth) / 2;
    tft->drawString(text, x, y);
}

void DisplayManager::drawTitle(const String& title) {
    drawCenteredText(title, 20, COLOR_TITLE, 1);
}

void DisplayManager::showToast(const String& message, unsigned int duration) {
    // Store toast information
    if (message != "-") {
        toastMessage = message;
        toastStartTime = millis();
        toastDuration = duration;
        toastActive = true;
    }

    if (!toastActive) {
        return;
    }

    // Calculate toast dimensions
    int padding = 10;
    int toastWidth = message.length() * 6 + (padding * 2);
    int toastHeight = 20;
    int toastX = (tft->width() - toastWidth) / 2;
    int toastY = tft->height() - 40;
    
    // Draw toast background
    tft->fillRoundRect(toastX, toastY, toastWidth, toastHeight, 5, COLOR_TOAST_BG);
    tft->drawRoundRect(toastX, toastY, toastWidth, toastHeight, 5, COLOR_TOAST_BORDER);
    
    // Draw toast text
    tft->setTextColor(COLOR_TOAST_TEXT);
    tft->setTextSize(1);
    tft->drawString(message, toastX + padding, toastY + 6);
}
