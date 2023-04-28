#include "Clock.h"

Clock::Clock(TFT_eSPI* tft, uint16_t width, int32_t height) : sprite(tft) {
	this->width = width;
	this->height = height;
	this->tft = tft;

	sprite.setColorDepth(16);
	sprite.createSprite(width, height);
}

void Clock::setSmallFont(const uint8_t* font) {
	smallFont = font;
}

void Clock::setBigFont(const uint8_t* font) {
	bigFont = font;
	sprite.loadFont(bigFont);
}

void Clock::setPosition(int32_t x, int32_t y) {
	this->x = x;
	this->y = y;
}

void Clock::draw() {
	sprite.setTextDatum(MC_DATUM);
	sprite.fillRect(0, 0, width, height, TFT_BLACK);

	// sprite.loadFont(bigFont);
	sprite.drawString(WireBus::timeString, width/2,  26 / 2);

	// sprite.loadFont(smallFont);
	// sprite.drawString(datestring, width / 2, sprite.fontHeight() / 2 + 40, 1);
	sprite.pushSprite(x, y);
}