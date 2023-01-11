
#include <Arduino.h>
#include <PNGdec.h>
#include <SPI.h>
#include <TFT_eSPI.h>  // Graphics library

#include "WireBus.h"
#include "AzeretMono.h"
#include "Icons.h"
#include "NotoSansBold15.h"
#include "NotoSansBold36.h"
#include "VescUart.h"
#include "widget/Clock.h"
#include "widget/Gauge.h"
#include "widget/ProgressBar.h"

#define WHEEL_DIAMETER 51.8
#define TFT_LED 21	// Backlight
#define MAX_IMAGE_WIDTH 32
#define TEXTSPR_WIDTH 70

VescUart vescUart;
PNG png;
int32_t imgX;
int32_t imgY;

TFT_eSPI tft = TFT_eSPI();

ProgressBar power = ProgressBar(&tft, 2, 30, 100, TFT_WHITE, TFT_BROWN);
ProgressBar battery = ProgressBar(&tft, 2, 30, 100, TFT_WHITE, TFT_BROWN);
Gauge speedGauge = Gauge(&tft, 140, 1, TFT_WHITE);
Clock clkWidget = Clock(&tft, 100, 30);
auto voltageText = TFT_eSprite(&tft);
auto powerText = TFT_eSprite(&tft);

uint16_t* tft_buffer;
bool buffer_loaded = false;
uint16_t spr_width = 0;
uint16_t bg_color = 0;

void pngDrawLine(PNGDRAW* pDraw) {
	uint16_t lineBuffer[MAX_IMAGE_WIDTH];
	png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
	tft.pushImage(imgX, imgY + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

void drawPng(int32_t ix, int32_t iy, uint8_t* img, int dataSize) {
	imgX = ix;
	imgY = iy;

	int16_t rc = png.openFLASH(img, dataSize, pngDrawLine);
	if (rc == PNG_SUCCESS) {
		tft.startWrite();
		uint32_t dt = millis();
		rc = png.decode(NULL, 0);
		Serial.print(millis() - dt);
		Serial.println("ms");
		tft.endWrite();
		png.close(); // not needed for memory->memory decode
	} else {
		Serial.println("Failed to open image");
		Serial.println(rc);
		Serial.println(sizeof(img));
	}
}

void createBattery() {
	battery.setPosition(20, 40);
	battery.mode = ProgressBar::Mode::VerticalReversed;

	drawPng(21, 5, battery32, sizeof(battery32));

	voltageText.setColorDepth(16);
	voltageText.createSprite(TEXTSPR_WIDTH, 30);
	voltageText.setTextDatum(TL_DATUM);
	voltageText.loadFont(AzeretMono16);
	voltageText.setTextColor(TFT_GREEN);
}

void drawBattery() {
	float pct = random(100);
	uint8_t red = 255 - 255 * (pct / 100);
	uint8_t green = 255 * (pct / 100);
	uint16_t newColor = (((31 * (red + 4)) / 255) << 11) | (((63 * (green + 2)) / 255) << 5) | ((31 * (0 + 4)) / 255);
	battery.fillColor = newColor;
	battery.setProgress(pct);
	battery.draw();

	float rx = random(840);
	String vol = String(rx / 10, 1);
	voltageText.fillRect(0, 0, TEXTSPR_WIDTH, 30, TFT_BLACK);
	voltageText.drawString(vol + "V", 0, 0);
	voltageText.pushSprite(10, 160);
}

void createPower() {
	power.setPosition(270, 40);
	power.mode = ProgressBar::Mode::VerticalReversed;

	drawPng(269, 5, engine32, sizeof(engine32));

	powerText.setColorDepth(16);
	powerText.createSprite(TEXTSPR_WIDTH, 30);

	powerText.setTextDatum(TL_DATUM);
	powerText.loadFont(AzeretMono16);
	powerText.setTextColor(TFT_GREEN);
}

void drawPower() {
	// tft.setTextColor(TFT_DARKCYAN, TFT_BLACK, true);
	// tft.drawString(String(vescUart.data.ampHoursCharged, 1), 0, 100);
	power.setProgress(random(100));
	power.draw();

	float rv = random(22);
	String vol = String(rv / 10, 1);
	powerText.fillRect(0, 0, TEXTSPR_WIDTH, 30, TFT_BLACK);
	powerText.drawString(vol + "kW", 0, 0);
	powerText.pushSprite(260,160);
}

#define TEMP1X 60
#define TEMP2X 160
#define TEMP1Y 200
#define TEMP2Y 200

void createClock() {
	clkWidget.setPosition(110, 170);
	clkWidget.setBigFont(AzeretMono16);
	clkWidget.setSmallFont(AzeretMono14);
	// clkWidget.draw();
}

void createTemps() {
	drawPng(TEMP1X, TEMP1Y, temperature32, sizeof(temperature32));
	drawPng(TEMP1X + 32, TEMP1Y, cpu32, sizeof(cpu32));

	drawPng(TEMP2X, TEMP2Y, temperature32, sizeof(temperature32));
	drawPng(TEMP2X + 32, TEMP2Y, car_engine32, sizeof(car_engine32));
}

void drawTemps() {
	tft.setTextDatum(TL_DATUM);
	tft.loadFont(AzeretMono22);
	tft.drawString(String(26.7, 0), TEMP1X + 2*35, TEMP1Y + 5);  // vescUart.data.tempMotor
	tft.drawString(String(29.3, 0), TEMP2X + 2 * 35, TEMP2Y + 5);
}

void createSpeedGauge() {
	speedGauge.setFonts(AzeretMono16, AzeretMono26, AzeretMono14);
	speedGauge.setPosition(90, 20);
	speedGauge.setSections(0, 80, 8, 6, 1.5);
	speedGauge.setTopLabel("Speed");
	speedGauge.setBottomLabel("km/h");
	speedGauge.setValue(50);
	speedGauge.draw();
}

void setup() {
	Serial.begin(115200);  // Debug only
	// Turn on backlight
	pinMode(TFT_LED, OUTPUT);
	digitalWrite(TFT_LED, HIGH);
	pinMode(13, OUTPUT);
	WireBus::init();

	tft.begin();
	tft.setRotation(0);
	tft.fillScreen(TFT_BLACK);

	tft.loadFont(AzeretMono16);
	tft.setTextDatum(MC_DATUM);

	createBattery();
	createPower();
	createSpeedGauge();
	createTemps();
	createClock();

	delay(1000);
}

int spp = 0;
bool blink = false;
void loop() {
	uint16_t angle = random(241);  // random speed in power 0 to 240
	float pct = random(100);
	// Plot needle at random angle in power 0 to 240, speed 40ms per increment
	// plotNeedle(angle, 30);
	drawBattery();
	drawPower();

	spp++;
	if (spp > 80) {
		spp = 0;
	}
	speedGauge.setValue(spp);
	speedGauge.draw();
	// Pause at new position
	drawTemps();
	clkWidget.draw();

	blink = !blink;
	digitalWrite(13, 1);
	delay(1);
	digitalWrite(13,0);
	delay(1);
	digitalWrite(13, 1);
	delay(1);
	digitalWrite(13, 0);
	delay(1);
	digitalWrite(13, 1);
	delay(1);
	digitalWrite(13, 0);
	delay(1);
	digitalWrite(13, 1);
	delay(1);
	digitalWrite(13, 0);
	delay(1);
	digitalWrite(13, 1);
	delay(1);
	digitalWrite(13, 0);
	delay(1);
	digitalWrite(13, 1);
	delay(1);
	digitalWrite(13, 0);
	delay(1);

	delay(150);
	WireBus::getTime();
}

// void setup() {
// 	/** Setup Serial port to display data */
// 	Serial.begin(9600);

// 	// /** Setup UART port (Serial1 on Atmega32u4) */
// 	Serial1.begin(115200);
// 	// Serial1.begin(19200);

// 	while (!Serial) {
// 		;
// 	}

// 	// /** Define which ports to use as UART */
// 	UART.setSerialPort(&Serial1);

// 	tft.begin();
// 	tft.setRotation(LANDSCAPE);

// 	pinMode(TFT_LED, OUTPUT);
// 	digitalWrite(TFT_LED, HIGH);

// 	tft.fillScreen(ILI9341_BLACK);
// }

// unsigned long testText() {
// 	// tft.fillScreen(ILI9341_BLACK);
// 	unsigned long start = micros();
// 	tft.setCursor(0, 0);

// 	tft.setTextColor(ILI9341_GREEN);
// 	tft.setTextSize(4);
// 	tft.println("VESC TEST");
// 	tft.setTextSize(1);
// 	tft.println("UBOX + STM32 + IL 2.4 Display");
// 	tft.setTextSize(2);

// 	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
// 	tft.print("Speed: ");
// 	tft.println(UART.data.rpm * 0.001885 * WHEEL_DIAMETER);
// 	tft.print("RPM: ");
// 	tft.println(UART.data.rpm);
// 	tft.print("Voltage: ");
// 	tft.println(UART.data.inpVoltage);
// 	tft.print("Ah: ");
// 	tft.println(UART.data.ampHours);
// 	tft.print("Tacho: ");
// 	tft.println(UART.data.tachometerAbs);
// 	tft.print("Temp Mosfet: ");
// 	tft.println(UART.data.tempMosfet);
// 	tft.print("Temp Motor: ");
// 	tft.println(UART.data.tempMotor);

// 	return micros() - start;
// }

// void loop() {
// 	/** Call the function getVescValues() to acquire data from VESC */
// 	if (UART.getVescValues()) {
// 		// Serial.println(UART.data.rpm);
// 		// Serial.println(UART.data.inpVoltage);
// 		// Serial.println(UART.data.ampHours);
// 		// Serial.println(UART.data.tachometerAbs);
// 		// Serial.println(UART.data.tempMosfet);
// 		// Serial.println(UART.data.tempMotor);
// 		// Serial.println(UART.appData.pitch);
// 		// Serial.println(UART.appData.roll);
// 	}
// 	else {
// 		Serial.println("Failed to get data!");
// 	}

// 	Serial.println(" --");
// 	testText();
// 	blink = !blink;
// 	digitalWrite(PC13, blink ? 1 : 0);
// 	delay(200);
// }