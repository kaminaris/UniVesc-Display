#include "Gauge.h"

Gauge::Gauge(TFT_eSPI* tft, uint16_t size, int32_t padding, uint32_t borderColor) : sprite(tft) {
	this->size = size;
	this->min = 0;
	this->max = 100;
	this->padding = padding;
	this->borderColor = borderColor;
	this->value = 0;
	this->topLabel = nullptr;

	sprite.setColorDepth(16);
	sprite.createSprite(size, size);

	sprite.fillCircle(size / 2, size / 2, size / 2, borderColor);
	this->clear();
}

void Gauge::clear() {
	sprite.fillCircle(size / 2, size / 2, size / 2 - 2 * padding, TFT_BLACK);
}

void Gauge::setSections(float min, float max, uint8_t sectionsCount, uint8_t sectionsLength, float sectionsWidth) {
	this->clear();
	this->min = min;
	this->max = max;
	this->sectionsCount = sectionsCount;
	this->sectionsLength = sectionsLength;
	this->sectionsWidth = sectionsWidth;
	sprite.loadFont(sectionsFont);
	this->drawSections();
}

void Gauge::drawSections() {
	float radiusMin = size / 2 - 1 - sectionsLength;
	float radiusText = size / 2 - 1 - sectionsLength - 12;
	float radiusMax = size / 2 - 1;

	//sprite.loadFont(sectionsFont);
	sprite.setTextDatum(MC_DATUM);
	for (int i = 0; i <= sectionsCount; i++) {
		float angle = 60 + 240 - (240 / sectionsCount) * i;
		sprite.drawWideLine(
			size / 2 + radiusMin * sin(PI * 2 * angle / 360),
			size / 2 + radiusMin * cos(PI * 2 * angle / 360),
			size / 2 + radiusMax * sin(PI * 2 * angle / 360),
			size / 2 + radiusMax * cos(PI * 2 * angle / 360),
			sectionsWidth,
			borderColor
		);
		float sectionValue = ((max - min) / sectionsCount) * i;
		sprite.drawString(
			String(sectionValue, 0),
			size / 2 + radiusText * sin(PI * 2 * angle / 360),
			size / 2 + radiusText * cos(PI * 2 * angle / 360),
			1
		);
	}
}

void Gauge::setValue(float v) {
	this->clear();
	this->drawSections();
	this->drawTopLabel();
	this->drawBottomLabel();

	this->value = v;
	this->drawValue();

	float radiusMin = size / 4;
	float radiusMax = size / 2 - 4;
	float perAngle = 240 / (max - min);
	float angle = 60 + 240 - perAngle * v;

	sprite.drawWedgeLine(
		size / 2 + radiusMin * sin(PI * 2 * angle / 360),
		size / 2 + radiusMin * cos(PI * 2 * angle / 360),
		size / 2 + radiusMax * sin(PI * 2 * angle / 360),
		size / 2 + radiusMax * cos(PI * 2 * angle / 360),
		4,
		1,
		TFT_GREEN
	);
}

void Gauge::setTopLabel(const char* label) {
	this->topLabel = label;
	this->drawTopLabel();
}

void Gauge::setBottomLabel(const char* label) {
	this->bottomLabel = label;
	this->drawBottomLabel();
}

void Gauge::drawTopLabel() {
	if (topLabel == nullptr) {
		return;
	}

	// sprite.loadFont(smallFont);

	sprite.setTextDatum(MC_DATUM);
	sprite.drawString(topLabel, size / 2, size / 2 - 10, 1);
}

void Gauge::drawBottomLabel() {
	if (bottomLabel == nullptr) {
		return;
	}

	// sprite.loadFont(smallFont);

	sprite.setTextDatum(MC_DATUM);
	sprite.drawString(bottomLabel, size / 2, size / 2 + 40, 1);
}

void Gauge::drawValue() {
	// sprite.loadFont(bigFont);
	sprite.drawString(String(value, 0u), size / 2, size / 2 + 17, 1);
}

void Gauge::setPosition(int32_t x, int32_t y) {
	this->x = x;
	this->y = y;
}

void Gauge::setFonts(const uint8_t* small, const uint8_t* big, const uint8_t* sections) {
	smallFont = small;
	bigFont = big;
	sectionsFont = sections;
}

void Gauge::draw() {
	sprite.pushSprite(x, y);
}