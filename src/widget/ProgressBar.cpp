#include "ProgressBar.h"

ProgressBar::ProgressBar(
	TFT_eSPI* tft,
	int32_t padding,
	int32_t width,
	int32_t height,
	uint32_t borderColor,
	uint32_t fillColor
)
	: sprite(tft) {
	this->width = width;
	this->height = height;
	this->padding = padding;
	this->fillColor = fillColor;
	this->min = 0;
	this->max = 100;
	this->progress = min;
	sprite.setColorDepth(16);
	sprite.createSprite(width, height);

	sprite.fillRect(0, 0, width, height, borderColor);
	this->fillProgress();
}

void ProgressBar::fillProgress() {
	sprite.fillRect(padding, padding, width - 2 * padding, height - 2 * padding, TFT_BLACK);

	uint32_t maxSize =
		mode == Mode::Horizontal || mode == Mode::HorizontalReversed ? width - 4 * padding : height - 4 * padding;
	float valPerPx = maxSize / (max - min);
	int size = valPerPx * progress;
	if (mode == Mode::Horizontal) {
		sprite.fillRect(padding * 2, padding * 2, size, height - 4 * padding, fillColor);
	}
	else if (mode == Mode::HorizontalReversed) {
		sprite.fillRect(padding * 2 + maxSize - size, padding * 2, size, height - 4 * padding, fillColor);
	}
	else if (mode == Mode::Vertical) {
		sprite.fillRect(padding * 2, padding * 2, width - 4 * padding, size, fillColor);
	}
	else {
		sprite.fillRect(padding * 2, padding * 2 + maxSize - size, width - 4 * padding, size, fillColor);
	}
}

void ProgressBar::setPosition(int32_t x, int32_t y) {
	this->x = x;
	this->y = y;
}

void ProgressBar::draw() {
	sprite.pushSprite(x, y);
}

void ProgressBar::setMinMax(float minimum, float maximum) {
	min = minimum;
	max = maximum;
}

void ProgressBar::setProgress(float p) {
	progress = p;
	if (progress > max) {
		progress = max;
	}

	if (progress < min || isnan(progress)) {
		progress = min;
	}

	this->fillProgress();
}