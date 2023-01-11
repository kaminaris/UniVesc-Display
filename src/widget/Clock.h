#ifndef VESCTEST_CLOCK_H
#define VESCTEST_CLOCK_H

#include "TFT_eSPI.h"
#include "WireBus.h"

class Clock {
	public:
	uint16_t width;
	uint16_t height;
	int32_t x;
	int32_t y;
	const uint8_t* smallFont;
	const uint8_t* bigFont;
	TFT_eSprite sprite;
	TFT_eSPI* tft;

	Clock(TFT_eSPI* tft, uint16_t width, int32_t height);

	void setSmallFont(const uint8_t* font);
	void setBigFont(const uint8_t* font);

	void setPosition(int32_t x, int32_t y);
	void draw();
};

#endif
