#include <Arduino.h>

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "VescUart.h"

#include <SPI.h>

#define PORTRAIT 0
#define LANDSCAPE 1
#define TFT_CS PA4		   //   (Orange)
#define TFT_DC PB1		   //   (Green)
#define TFT_RST PB0		   //   (Yellow)
#define TFT_LED PB10	   // Backlight
#define TEST_WAVE_PIN PB1  // PB1 PWM 500 Hz

// Create the lcd object
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);  // Using hardware SPI


#define BOARD_LED PC13	// PB0

// Display colours
#define BEAM1_COLOUR ILI9341_GREEN
#define BEAM2_COLOUR ILI9341_RED
#define GRATICULE_COLOUR 0x07FF
#define BEAM_OFF_COLOUR ILI9341_BLACK
#define CURSOR_COLOUR ILI9341_GREEN

#define WHEEL_DIAMETER 51.8

/** Initiate VescUart class */
VescUart UART;

bool blink = false;

void setup() {
	/** Setup Serial port to display data */
	Serial.begin(9600);

	// /** Setup UART port (Serial1 on Atmega32u4) */
	Serial1.begin(115200);
	// Serial1.begin(19200);

	while (!Serial) {
		;
	}

	// /** Define which ports to use as UART */
	UART.setSerialPort(&Serial1);

	tft.begin();
	tft.setRotation(LANDSCAPE);

	pinMode(TFT_LED, OUTPUT);
	digitalWrite(TFT_LED, HIGH);

	tft.fillScreen(ILI9341_BLACK);
}

unsigned long testText() {
	// tft.fillScreen(ILI9341_BLACK);
	unsigned long start = micros();
	tft.setCursor(0, 0);

	tft.setTextColor(ILI9341_GREEN);
	tft.setTextSize(4);
	tft.println("VESC TEST");
	tft.setTextSize(1);
	tft.println("UBOX + STM32 + IL 2.4 Display");
	tft.setTextSize(2);

	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.print("Speed: ");
	tft.println(UART.data.rpm * 0.001885 * WHEEL_DIAMETER);
	tft.print("RPM: ");
	tft.println(UART.data.rpm);
	tft.print("Voltage: ");
	tft.println(UART.data.inpVoltage);
	tft.print("Ah: ");
	tft.println(UART.data.ampHours);
	tft.print("Tacho: ");
	tft.println(UART.data.tachometerAbs);
	tft.print("Temp Mosfet: ");
	tft.println(UART.data.tempMosfet);
	tft.print("Temp Motor: ");
	tft.println(UART.data.tempMotor);

	return micros() - start;
}

void loop() {
	/** Call the function getVescValues() to acquire data from VESC */
	if (UART.getVescValues()) {
		// Serial.println(UART.data.rpm);
		// Serial.println(UART.data.inpVoltage);
		// Serial.println(UART.data.ampHours);
		// Serial.println(UART.data.tachometerAbs);
		// Serial.println(UART.data.tempMosfet);
		// Serial.println(UART.data.tempMotor);
		// Serial.println(UART.appData.pitch);
		// Serial.println(UART.appData.roll);
	}
	else {
		Serial.println("Failed to get data!");
	}

	Serial.println(" --");
	testText();
	blink = !blink;
	digitalWrite(PC13, blink ? 1 : 0);
	delay(200);
}