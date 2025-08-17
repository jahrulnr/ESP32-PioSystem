#ifndef MICBAR_H
#define MICBAR_H

#include <Arduino.h>
#include <TFT_eSPI.h>

class MicBar {
	private:
		int _micLevel = 0;
		TFT_eSPI *_display;
		int _x = 0;
		int _y = 0;
		int _width = 200;
		int _height = 10;
		uint16_t _barColor = TFT_GREEN;
		uint16_t _bgColor = TFT_BLACK;
		uint16_t _borderColor = TFT_WHITE;
		
	public:
		MicBar(TFT_eSPI *display);
		MicBar(TFT_eSPI *display, int x, int y, int width, int height);
		void setPosition(int x, int y);
		void setSize(int width, int height);
		void setColors(uint16_t barColor, uint16_t bgColor, uint16_t borderColor);
		void setLevel(int level);
		void drawBar();
		void drawBar(int level);
};

#endif // MICBAR_H