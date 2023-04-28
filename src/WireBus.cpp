#include "WireBus.h"

uint8_t BcdToUint8(uint8_t val) {
	return val - 6 * (val >> 4);
}

uint8_t Uint8ToBcd(uint8_t val) {
	return val + 6 * (val / 10);
}

uint8_t BcdToBin24Hour(uint8_t bcdHour) {
	uint8_t hour;
	if (bcdHour & 0x40) {
		// 12 hour mode, convert to 24
		bool isPm = ((bcdHour & 0x20) != 0);

		hour = BcdToUint8(bcdHour & 0x1f);
		if (isPm) {
			hour += 12;
		}
	}
	else {
		hour = BcdToUint8(bcdHour);
	}
	return hour;
}

char WireBus::dateString[20] = {0};
char WireBus::timeString[20] = {0};

void WireBus::init() {
	Wire1.begin(I2C_SDA, I2C_SCL);	// 17, 16
}

void WireBus::eepromWrite(unsigned int uiAddress, int bData) {
	int iCount = 0;
	do {
		Wire1.beginTransmission(AT24C32_I2C_ADDRESS);
		Wire1.write((uint8_t)(uiAddress >> 8));	 // MSB
		Wire1.write((uint8_t)uiAddress);		 // LSB
		Wire1.write(bData);
	} while (Wire1.endTransmission() != 0 && ++iCount < 10);
}

int WireBus::eepromRead(unsigned int uiAddress) {
	int iCount = 0;
	do {
		Wire1.beginTransmission(AT24C32_I2C_ADDRESS);
		Wire1.write((uint8_t)(uiAddress >> 8));	 // MSB
		Wire1.write((uint8_t)uiAddress);		 // LSB
	} while (Wire1.endTransmission() != 0 && ++iCount < 10);
	Wire1.requestFrom(AT24C32_I2C_ADDRESS, 1);

	return Wire1.read();
}

void WireBus::getTime() {
	Wire1.beginTransmission(DS1307_I2C_ADDRESS);
	Wire1.write(DS1307_REG_TIMEDATE);
	uint8_t _lastError = Wire1.endTransmission();
	if (_lastError != 0) {
		// Serial.println("fail");
		// Serial.println(_lastError);
	}

	uint8_t bytesRead = Wire1.requestFrom(DS1307_I2C_ADDRESS, DS1307_REG_TIMEDATE_SIZE);

	uint8_t second = BcdToUint8(Wire1.read() & 0x7F);
	uint8_t minute = BcdToUint8(Wire1.read());
	uint8_t hour = BcdToBin24Hour(Wire1.read());

	Wire1.read();  // throwing away day of week as we calculate it

	uint8_t dayOfMonth = BcdToUint8(Wire1.read());
	uint8_t month = BcdToUint8(Wire1.read());
	uint16_t year = BcdToUint8(Wire1.read()) + 2000;

	snprintf(
		timeString,
		sizeof(timeString),
		PSTR("%02u:%02u:%02u"),	 // %02u/%02u/%04u
		hour,
		minute,
		second
	);

	snprintf(
		dateString,
		sizeof(dateString),
		PSTR("%04u-%02u-%02u"),	 // %02u/%02u/%04u
		year,
		month,
		dayOfMonth
	);
}

void WireBus::setTime(
	uint8_t second,
	uint8_t minute,
	uint8_t hour,
	uint8_t dayOfWeek,
	uint8_t day,
	uint8_t month,
	uint16_t year
) {
	// uint8_t sreg = getReg(DS1307_REG_STATUS) & _BV(DS1307_CH);

	// set the date time
	Wire1.beginTransmission(DS1307_I2C_ADDRESS);
	Wire1.write(DS1307_REG_TIMEDATE);

	Wire1.write(Uint8ToBcd(second));  // | sreg
	Wire1.write(Uint8ToBcd(minute));
	Wire1.write(Uint8ToBcd(hour));	// 24 hour mode only

	Wire1.write(Uint8ToBcd(dayOfWeek));
	Wire1.write(Uint8ToBcd(day));
	Wire1.write(Uint8ToBcd(month));
	Wire1.write(Uint8ToBcd(year - 2000));

	uint8_t _lastError = Wire1.endTransmission();
	if (_lastError != 0) {
		// Serial.println("fail");
	}
}