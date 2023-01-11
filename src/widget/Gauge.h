#ifndef VESCTEST_GAUGE_H
#define VESCTEST_GAUGE_H

#include "TFT_eSPI.h"

class Gauge {
	public:
	uint16_t size;
	int32_t padding;
	uint32_t borderColor;
	float min;
	float max;
	uint8_t sectionsCount;
	uint8_t sectionsLength;
	float sectionsWidth;
	int32_t x;
	int32_t y;
	float value;
	const char* topLabel;
	const char* bottomLabel;
	const uint8_t* smallFont;
	const uint8_t* bigFont;
	const uint8_t* sectionsFont;
	TFT_eSprite sprite;
	TFT_eSPI* tft;

	Gauge(TFT_eSPI* tft, uint16_t size, int32_t padding, uint32_t borderColor);
	void clear();
	void setSections(float min, float max, uint8_t sectionsCount, uint8_t sectionsLength, float sectionsWidth);
	void drawSections();
	void setPosition(int32_t x, int32_t y);
	void setValue(float v);
	void setTopLabel(const char* label);
	void setBottomLabel(const char* label);
	void drawTopLabel();
	void drawBottomLabel();
	void drawValue();
	void setFonts(const uint8_t* small, const uint8_t* big, const uint8_t* sections);
	void draw();
};

#endif
