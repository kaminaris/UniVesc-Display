#include <Arduino.h>
#include <HardwareSerial.h>
#include <PNGdec.h>
#include <SPI.h>
#include <TFT_eSPI.h>  // Graphics library

#include "AzeretMono.h"
#include "Buzzer.h"
#include "Icons.h"
#include "NotoSansBold15.h"
#include "NotoSansBold36.h"
#include "VescUart.h"
#include "WireBus.h"
#include "widget/Clock.h"
#include "widget/Gauge.h"
#include "widget/ProgressBar.h"

// APP SETTINGS START
#define WHEEL_DIAMETER 500
#define MOTOR_POLE_PAIRS 7
#define MAX_BATTERY_VOLTAGE 84
#define MAX_MOTOR_CURRENT 35
#define VOLTAGE_X 5
#define POWER_X 205

#define TEMP_MCU_X 10
#define TEMP_MCU_Y 220

#define TEMP_MOSFET_X 120
#define TEMP_MOSFET_Y 220

#define CLOCK_X 50
#define CLOCK_Y 180

// APP SETTINGS FINISH
#define TFT_LED 21	// Backlight
#define BUZZER_PIN 13
#define MAX_IMAGE_WIDTH 32
#define TEXTSPR_WIDTH 70
#define RXD2 16
#define TXD2 17

enum AppMode { Demo, Live };

// Local variables
AppMode mode = Demo;
PNG png;
int32_t imgX;
int32_t imgY;

// Main classes
VescUart vescUart;
TFT_eSPI tft = TFT_eSPI();
Buzzer buzzer = Buzzer(BUZZER_PIN);

// Display widgets
ProgressBar power = ProgressBar(&tft, 2, 30, 100, TFT_WHITE, TFT_BROWN);
ProgressBar battery = ProgressBar(&tft, 2, 30, 100, TFT_WHITE, TFT_BROWN);
Gauge speedGauge = Gauge(&tft, 140, 1, TFT_WHITE);
Clock clkWidget = Clock(&tft, 120, 30);
auto voltageText = TFT_eSprite(&tft);
auto powerText = TFT_eSprite(&tft);
auto odometerText = TFT_eSprite(&tft);

void clearScreen() {
	tft.fillScreen(TFT_BLACK);

	tft.loadFont(AzeretMono16);
	tft.setTextDatum(MC_DATUM);
}

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
		png.close();  // not needed for memory->memory decode
	}
	else {
		Serial.println("Failed to open image");
		Serial.println(rc);
		Serial.println(sizeof(img));
	}
}

void createBattery() {
	battery.setPosition(VOLTAGE_X, 40);
	battery.mode = ProgressBar::Mode::VerticalReversed;

	drawPng(VOLTAGE_X + 1, 5, battery32, sizeof(battery32));

	voltageText.setColorDepth(16);
	voltageText.createSprite(TEXTSPR_WIDTH, 20);
	voltageText.setTextDatum(TL_DATUM);
	voltageText.loadFont(AzeretMono16);
	voltageText.setTextColor(TFT_GREEN);
}

void drawBattery() {
	float pct = mode == Demo ? random(100) : vescUart.data.wattHours / vescUart.data.wattHoursCharged;
	float currentVoltage = mode == Demo ? random(MAX_BATTERY_VOLTAGE * 10) / 10 : vescUart.data.inpVoltage;

	uint8_t red = 255 - 255 * (pct / 100);
	uint8_t green = 255 * (pct / 100);
	uint16_t newColor = (((31 * (red + 4)) / 255) << 11) | (((63 * (green + 2)) / 255) << 5) | ((31 * (0 + 4)) / 255);
	battery.fillColor = newColor;
	battery.setProgress(pct);
	battery.draw();

	voltageText.fillRect(0, 0, TEXTSPR_WIDTH, 30, TFT_BLACK);
	voltageText.drawString(String(currentVoltage, 1) + "V", 0, 0);
	voltageText.pushSprite(10, 160);
}

void createPower() {
	power.setPosition(POWER_X, 40);
	power.mode = ProgressBar::Mode::VerticalReversed;

	drawPng(POWER_X, 5, engine32, sizeof(engine32));

	powerText.setColorDepth(16);
	powerText.createSprite(TEXTSPR_WIDTH, 20);

	powerText.setTextDatum(TL_DATUM);
	powerText.loadFont(AzeretMono16);
	powerText.setTextColor(TFT_GREEN);
}

void drawPower() {
	float percent = mode == Demo ? random(100) : vescUart.data.avgMotorCurrent / MAX_MOTOR_CURRENT;
	float rv = mode == Demo ? random(2200) : vescUart.data.avgMotorCurrent * vescUart.data.inpVoltage;

	power.setProgress(percent);
	power.draw();

	String vol = String(rv / 1000, 1);
	powerText.fillRect(0, 0, TEXTSPR_WIDTH, 30, TFT_BLACK);
	powerText.drawString(vol + "kW", 0, 0);
	powerText.pushSprite(POWER_X - 25, 160);
}

void createClock() {
	clkWidget.setPosition(CLOCK_X, CLOCK_Y);
	clkWidget.setBigFont(AzeretMono22);
	clkWidget.setSmallFont(AzeretMono14);
	// clkWidget.draw();
}

void createTemps() {
	drawPng(TEMP_MCU_X, TEMP_MCU_Y, temperature32, sizeof(temperature32));
	drawPng(TEMP_MCU_X + 32, TEMP_MCU_Y, cpu32, sizeof(cpu32));

	drawPng(TEMP_MOSFET_X, TEMP_MOSFET_Y, temperature32, sizeof(temperature32));
	drawPng(TEMP_MOSFET_X + 32, TEMP_MOSFET_Y, car_engine32, sizeof(car_engine32));
}

void drawTemps() {
	float motorTemp = mode == Demo ? 26.7 : vescUart.data.tempMotor;
	float mosfetTemp = mode == Demo ? 29.3 : vescUart.data.tempMosfet;

	tft.setTextDatum(TL_DATUM);
	tft.loadFont(AzeretMono22);
	tft.drawString(String(motorTemp, 0), TEMP_MCU_X + 2 * 35, TEMP_MCU_Y + 5);	//
	tft.drawString(String(mosfetTemp, 0), TEMP_MOSFET_X + 2 * 35, TEMP_MOSFET_Y + 5);
}

void createSpeedGauge() {
	speedGauge.setFonts(AzeretMono16, AzeretMono26, AzeretMono14);
	speedGauge.setPosition(50, 20);
	speedGauge.setSections(0, 80, 8, 6, 1.5);
	// speedGauge.setTopLabel("Speed");
	speedGauge.setBottomLabel("km/h");
	speedGauge.setValue(50);
	speedGauge.draw();
}

int spp = 0;
void drawSpeedGauge() {
	spp++;
	if (spp > 80) {
		spp = 0;
	}
	speedGauge.setValue(spp);
	speedGauge.draw();
}

void createOdometer() {
	odometerText.setColorDepth(16);
	odometerText.createSprite(260, 40);

	odometerText.setTextDatum(ML_DATUM);
	odometerText.loadFont(AzeretMono16);
	odometerText.setTextColor(TFT_WHITE);
}

void drawOdometer() {
	float tachAbs = vescUart.data.tachometerAbs / (MOTOR_POLE_PAIRS * 3);
	float tach = vescUart.data.tachometer / (MOTOR_POLE_PAIRS * 3);
	// Motor RPM x Pi x (1 / meters in a mile or km) x Wheel diameter x (motor pulley / wheelpulley);
	float total = mode == Demo ? random(10000) / 10 : tachAbs * PI * (1 / 1000) * WHEEL_DIAMETER / 1000;
	float session = mode == Demo ? random(10000) / 10 : tach * PI * (1 / 1000) * WHEEL_DIAMETER / 1000;

	odometerText.fillRect(0, 0, 260, 40, TFT_BLACK);
	odometerText.drawString("T: " + String(total, 1) + "km  S: " + String(session, 1) + "km", 0, 20);
	odometerText.pushSprite(20, 260);
}

void switchToMode(AppMode m, bool force = false) {
	if (mode == m && !force) {
		return;
	}

	clearScreen();

	createBattery();
	createPower();
	createSpeedGauge();
	createTemps();
	createClock();
	createOdometer();
}

void setup() {
	// Debug only
	Serial.begin(115200);
	// Communication with vesc
	Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
	vescUart.setSerialPort(&Serial2);

	// Turn on backlight
	pinMode(TFT_LED, OUTPUT);
	digitalWrite(TFT_LED, HIGH);
	WireBus::init();

	tft.begin();
	tft.setRotation(0);
	switchToMode(Demo, true);

	delay(1000);
}

uint32_t lastRefresh = 0;
uint32_t lastBeep = 0;
bool beeping = false;

void loop() {
	uint32_t m = millis();

	if (m - lastRefresh >= 150) {
		WireBus::getTime();

		if (vescUart.getVescValues()) {
			Serial.println(vescUart.data.rpm);
			Serial.println(vescUart.data.inpVoltage);
			Serial.println(vescUart.data.ampHours);
			Serial.println(vescUart.data.tachometerAbs);
			Serial.println(vescUart.data.tempMosfet);
			Serial.println(vescUart.data.tempMotor);
			Serial.println(vescUart.appData.pitch);
			Serial.println(vescUart.appData.roll);
			switchToMode(Live);
		}
		else {
			// Serial.println("Failed to get data!");
			switchToMode(Demo);
		}

		drawBattery();
		drawPower();
		drawSpeedGauge();
		drawTemps();
		drawOdometer();
		clkWidget.draw();

		
		lastRefresh = m;
	}

	// if (m - lastBeep >= 1000) {
	// 	beeping = !beeping;
	// 	if (beeping) {
	// 		buzzer.stopBeep();
	// 	} else {
	// 		buzzer.startBeep(NOTE_C6);
	// 	}
	// 	lastBeep = m;
	// }
}