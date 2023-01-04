#ifndef VESCTEST_PROGRESSBAR_H
#define VESCTEST_PROGRESSBAR_H

#include "TFT_eSPI.h"

class ProgressBar {
	public:
	enum Mode { Horizontal, Vertical, HorizontalReversed, VerticalReversed };

	Mode mode;
	int32_t width;
	int32_t height;
	int32_t padding;
	int32_t x;
	int32_t y;
	uint32_t fillColor;
	float min;
	float max;
	float progress;
	TFT_eSprite sprite;
	TFT_eSPI* tft;

	ProgressBar(
		TFT_eSPI* tft,
		int32_t padding,
		int32_t width,
		int32_t height,
		uint32_t borderColor,
		uint32_t fillColor
	);
	void fillProgress();
	void setMinMax(float minimum, float maximum);
	void setProgress(float p);

	void setPosition(int32_t x, int32_t y);
	void draw();
};

#endif
