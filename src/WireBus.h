
#ifndef VESCTEST_WIREBUS_H
#define VESCTEST_WIREBUS_H

#include <Wire.h>  // must be included here so that Arduino library object file references work

#define I2C_SDA 33
#define I2C_SCL 32

#define AT24C32_I2C_ADDRESS 0x50  // the I2C address of Tiny RTC AT24C32 EEPROM
#define DS1307_I2C_ADDRESS 0x68

#define DS1307_REG_TIMEDATE_SIZE 7

#define DS1307_REG_TIMEDATE 0x00
#define DS1307_REG_STATUS 0x00
#define DS1307_REG_CONTROL 0x07
#define DS1307_REG_RAMSTART 0x08
#define DS1307_REG_RAMEND 0x3f
const uint8_t DS1307_REG_RAMSIZE = DS1307_REG_RAMEND - DS1307_REG_RAMSTART;

class WireBus {
	public:
	static char dateString[20];
	static char timeString[20];
	static void init();
	static void eepromWrite(unsigned int uiAddress, int bData);
	static int eepromRead(unsigned int uiAddress);
	static void getTime();
	static void
	setTime(uint8_t second, uint8_t minute, uint8_t hour, uint8_t dayOfWeek, uint8_t day, uint8_t month, uint16_t year);
};

#endif
