#include "Clock.h"
#define I2C_SDA 33
#define I2C_SCL 32

char datestring[20] = {0};
char timestring[20] = {0};

Clock::Clock(TFT_eSPI* tft, uint16_t width, int32_t height) : sprite(tft) {
	this->width = width;
	this->height = height;

	sprite.setColorDepth(16);
	sprite.createSprite(width, height);
	Wire1.begin(17, 16);

}

void Clock::getTime() {
	// now = Rtc.GetDateTime();
	Wire1.beginTransmission(DS1307_ADDRESS);
	Wire1.write(DS1307_REG_TIMEDATE);
	uint8_t _lastError = Wire1.endTransmission();
	if (_lastError != 0) {
		Serial.println("fail");
	}

	uint8_t bytesRead = Wire1.requestFrom(DS1307_ADDRESS, DS1307_REG_TIMEDATE_SIZE);
	Serial.println(bytesRead);

	uint8_t second = BcdToUint8(Wire1.read() & 0x7F);
	uint8_t minute = BcdToUint8(Wire1.read());
	uint8_t hour = BcdToBin24Hour(Wire1.read());

	Wire1.read();  // throwing away day of week as we calculate it

	uint8_t dayOfMonth = BcdToUint8(Wire1.read());
	uint8_t month = BcdToUint8(Wire1.read());
	uint16_t year = BcdToUint8(Wire1.read()) + 2000;

	snprintf(
		timestring,
		sizeof(timestring),
		PSTR("%02u:%02u:%02u"),	 // %02u/%02u/%04u
		hour,
		minute,
		second
	);

	snprintf(
		datestring,
		sizeof(datestring),
		PSTR("%04u-%02u-%02u"),	 // %02u/%02u/%04u
		year,
		month,
		dayOfMonth
	);
}

void Clock::setSmallFont(const uint8_t* font) {
	smallFont = font;
}

void Clock::setBigFont(const uint8_t* font) {
	bigFont = font;
}

void Clock::setPosition(int32_t x, int32_t y) {
	this->x = x;
	this->y = y;
}

void Clock::draw() {
	sprite.loadFont(bigFont);
	sprite.setTextDatum(MC_DATUM);
	sprite.fillRect(0, 0, width, height, TFT_BLACK);

	
	Serial.println(timestring);
	sprite.drawString(timestring, width / 2, height / 2, 1);

	sprite.setFreeFont(&FreeMono12pt7b);
	Serial.println(datestring);
	sprite.drawString(datestring, width / 2, height / 2 + 40, 1);
	sprite.pushSprite(x, y);
}