#include "micbar.h"

MicBar::MicBar(TFT_eSPI *display) : _display(display) {
	// Default constructor with default position and size
	_x = 10;
	_y = 200;
	_width = 200;
	_height = 10;
}

MicBar::MicBar(TFT_eSPI *display, int x, int y, int width, int height) : _display(display), _x(x), _y(y), _width(width), _height(height) {
	// Constructor with custom position and size
}

void MicBar::setPosition(int x, int y) {
	_x = x;
	_y = y;
}

void MicBar::setSize(int width, int height) {
	_width = width;
	_height = height;
}

void MicBar::setColors(uint16_t barColor, uint16_t bgColor, uint16_t borderColor) {
	_barColor = barColor;
	_bgColor = bgColor;
	_borderColor = borderColor;
}

void MicBar::setLevel(int level) {
	_micLevel = constrain(level, 0, 4095);
}

void MicBar::drawBar() {
	drawBar(_micLevel);
}

void MicBar::drawBar(int level) {
	if (_display == nullptr) return;

	// Constrain level to valid range
	level = constrain(level, 0, 4095);
	
	// Center the bar on the TFT display
	int tftCenterX = TFT_WIDTH / 2;
	int tftCenterY = TFT_HEIGHT / 2;
	int barCenterX = tftCenterX;
	int barCenterY = tftCenterY;

	// Calculate bar width based on level (0-4095 mapped to 0-width)
	int barWidth = map(level, 0, 4095, 0, _width - 4); // -4 for border

	// Calculate top-left position to center the bar
	int barX = barCenterX - (_width / 2);
	int barY = _y;

	// Draw border
	_display->drawRect(barX, barY, _width, _height, _borderColor);

	// Clear background inside border
	_display->fillRect(barX + 1, barY + 1, _width - 2, _height - 2, _bgColor);

	// Draw level bar from center outwards
	if (barWidth > 0) {
		int centerX = barX + (_width / 2);
		int halfBarWidth = barWidth / 2;
		int fillBarY = barY + 2;
		int barHeight = _height - 4;

		// Ensure we don't draw outside the border
		int leftX = max(barX + 2, centerX - halfBarWidth);
		int rightX = min(barX + _width - 2, centerX + halfBarWidth);
		int actualBarWidth = rightX - leftX;

		if (actualBarWidth > 0) {
			// Use gradient effect for better visual appeal
			if (level < 1365) { // Low level - green
				_display->fillRect(leftX, fillBarY, actualBarWidth, barHeight, TFT_GREEN);
			} else if (level < 2730) { // Medium level - yellow
				_display->fillRect(leftX, fillBarY, actualBarWidth, barHeight, TFT_YELLOW);
			} else if (level < 3640) { // High level - orange
				_display->fillRect(leftX, fillBarY, actualBarWidth, barHeight, TFT_ORANGE);
			} else { // Very high level - red
				_display->fillRect(leftX, fillBarY, actualBarWidth, barHeight, TFT_RED);
			}
		}
	}
	
	// Update stored level
	_micLevel = level;
}