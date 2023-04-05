#include <Arduino.h>
#include <HardwareSerial.h>
#include <PNGdec.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#include "AzeretMono.h"
#include "BluetoothSerial.h"
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
int32_t imgX;
int32_t imgY;

// Main classes
PNG* png = nullptr;
VescUart* vescUart = nullptr;
TFT_eSPI tft = TFT_eSPI();
Buzzer* buzzer = nullptr;

// Display widgets
ProgressBar* power = nullptr;
ProgressBar* battery = nullptr;
Gauge* speedGauge = nullptr;
Clock* clkWidget = nullptr;
TFT_eSprite* voltageText = nullptr;
TFT_eSprite* powerText = nullptr;
TFT_eSprite* odometerText = nullptr;
BluetoothSerial BT;

void clearScreen() {
	tft.fillScreen(TFT_BLACK);

	tft.loadFont(AzeretMono16);
	tft.setTextDatum(MC_DATUM);
}

void drawImage(int32_t ix, int32_t iy, uint32_t width, uint32_t height, uint16_t* img) {
	tft.startWrite();
	tft.pushImage(ix, iy, width, height, img);
	tft.endWrite();
}

void createBattery() {
	battery = new ProgressBar(&tft, 2, 30, 100, TFT_WHITE, TFT_BROWN);
	battery->setPosition(VOLTAGE_X, 40);
	battery->mode = ProgressBar::Mode::VerticalReversed;

	drawImage(VOLTAGE_X + 1, 5, 32, 32, battery32);
	voltageText = new TFT_eSprite(&tft);
	voltageText->setColorDepth(16);
	voltageText->createSprite(TEXTSPR_WIDTH, 20);
	voltageText->setTextDatum(TL_DATUM);
	voltageText->loadFont(AzeretMono16);
	voltageText->setTextColor(TFT_GREEN);
}

void drawBattery() {
	float pct = mode == Demo ? random(100) : vescUart->data.wattHours / vescUart->data.wattHoursCharged;
	float currentVoltage = mode == Demo ? random(MAX_BATTERY_VOLTAGE * 10) / 10 : vescUart->data.inpVoltage;

	uint8_t red = 255 - 255 * (pct / 100);
	uint8_t green = 255 * (pct / 100);
	uint16_t newColor = (((31 * (red + 4)) / 255) << 11) | (((63 * (green + 2)) / 255) << 5) | ((31 * (0 + 4)) / 255);
	battery->fillColor = newColor;
	battery->setProgress(pct);
	battery->draw();

	voltageText->fillRect(0, 0, TEXTSPR_WIDTH, 30, TFT_BLACK);
	voltageText->drawString(String(currentVoltage, 1) + "V", 0, 0);
	voltageText->pushSprite(10, 160);
}

void createPower() {
	power = new ProgressBar(&tft, 2, 30, 100, TFT_WHITE, TFT_BROWN);
	power->setPosition(POWER_X, 40);
	power->mode = ProgressBar::Mode::VerticalReversed;

	drawImage(POWER_X, 5, 32, 32, engine32);

	powerText = new TFT_eSprite(&tft);
	powerText->setColorDepth(16);
	powerText->createSprite(TEXTSPR_WIDTH, 20);

	powerText->setTextDatum(TL_DATUM);
	powerText->loadFont(AzeretMono16);
	powerText->setTextColor(TFT_GREEN);
}

void drawPower() {
	float percent = mode == Demo ? random(100) : vescUart->data.avgMotorCurrent / MAX_MOTOR_CURRENT;
	float rv = mode == Demo ? random(2200) : vescUart->data.avgMotorCurrent * vescUart->data.inpVoltage;

	power->setProgress(percent);
	power->draw();

	String vol = String(rv / 1000, 1);
	powerText->fillRect(0, 0, TEXTSPR_WIDTH, 30, TFT_BLACK);
	powerText->drawString(vol + "kW", 0, 0);
	powerText->pushSprite(POWER_X - 25, 160);
}

void createClock() {
	clkWidget = new Clock(&tft, 120, 30);
	clkWidget->setPosition(CLOCK_X, CLOCK_Y);
	clkWidget->setBigFont(AzeretMono22);
	clkWidget->setSmallFont(AzeretMono14);
	// clkWidget.draw();
}

void createTemps() {
	drawImage(TEMP_MCU_X, TEMP_MCU_Y, 32, 32, temperature32);
	drawImage(TEMP_MCU_X + 32, TEMP_MCU_Y, 32, 32, cpu32);

	drawImage(TEMP_MOSFET_X, TEMP_MOSFET_Y, 32, 32, temperature32);
	drawImage(TEMP_MOSFET_X + 32, TEMP_MOSFET_Y, 32, 32, car_engine32);
}

void drawTemps() {
	float motorTemp = mode == Demo ? 26.7 : vescUart->data.tempMotor;
	float mosfetTemp = mode == Demo ? 29.3 : vescUart->data.tempMosfet;

	tft.setTextDatum(TL_DATUM);
	tft.loadFont(AzeretMono22);
	tft.drawString(String(motorTemp, 0), TEMP_MCU_X + 2 * 35, TEMP_MCU_Y + 5);	//
	tft.drawString(String(mosfetTemp, 0), TEMP_MOSFET_X + 2 * 35, TEMP_MOSFET_Y + 5);
}

void createSpeedGauge() {
	speedGauge = new Gauge(&tft, 140, 1, TFT_WHITE);
	speedGauge->setFonts(AzeretMono16, AzeretMono26, AzeretMono14);
	speedGauge->setPosition(50, 20);
	speedGauge->setSections(0, 80, 8, 6, 1.5);
	// speedGauge.setTopLabel("Speed");
	speedGauge->setBottomLabel("km/h");
	speedGauge->setValue(50);
	speedGauge->draw();
}

int spp = 0;
void drawSpeedGauge() {
	spp++;
	if (spp > 80) {
		spp = 0;
	}
	speedGauge->setValue(spp);
	speedGauge->draw();
}

void createOdometer() {
	odometerText = new TFT_eSprite(&tft);
	odometerText->setColorDepth(16);
	odometerText->createSprite(260, 40);

	odometerText->setTextDatum(ML_DATUM);
	odometerText->loadFont(AzeretMono16);
	odometerText->setTextColor(TFT_WHITE);
}

void drawOdometer() {
	float tachAbs = vescUart->data.tachometerAbs / (MOTOR_POLE_PAIRS * 3);
	float tach = vescUart->data.tachometer / (MOTOR_POLE_PAIRS * 3);
	// Motor RPM x Pi x (1 / meters in a mile or km) x Wheel diameter x (motor pulley / wheelpulley);
	float total = mode == Demo ? random(10000) / 10 : tachAbs * PI * (1 / 1000) * WHEEL_DIAMETER / 1000;
	float session = mode == Demo ? random(10000) / 10 : tach * PI * (1 / 1000) * WHEEL_DIAMETER / 1000;

	odometerText->fillRect(0, 0, 260, 40, TFT_BLACK);
	odometerText->drawString("T: " + String(total, 1) + "km  S: " + String(session, 1) + "km", 0, 20);
	odometerText->pushSprite(20, 260);
}

void switchToMode(AppMode m, bool force = false) {
	if (mode == m && !force) {
		return;
	}

	clearScreen();
}

void UpdateTFT(void* pvParameters) {
	while (true) {
		drawBattery();
		drawPower();
		drawSpeedGauge();
		drawTemps();
		drawOdometer();
		clkWidget->draw();

		vTaskDelay(pdMS_TO_TICKS(250));
	}
}

void ReadBTUart(void* pvParams) {
	while (true) {
		if (Serial.available()) {
			BT.write(Serial.read());
		}
		if (BT.available()) {
			Serial.write(BT.read());
		}

		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

void ReadVesc(void* params) {
	while (true) {
		if (vescUart->getVescValues()) {
			Serial.println(vescUart->data.rpm);
			Serial.println(vescUart->data.inpVoltage);
			Serial.println(vescUart->data.ampHours);
			Serial.println(vescUart->data.tachometerAbs);
			Serial.println(vescUart->data.tempMosfet);
			Serial.println(vescUart->data.tempMotor);
			Serial.println(vescUart->appData.pitch);
			Serial.println(vescUart->appData.roll);
			switchToMode(Live);
		}
		else {
			// Serial.println("Failed to get data!");
			switchToMode(Demo);
		}

		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

void ReadTime(void* p) {
	while (true) {
		WireBus::getTime();
		vTaskDelay(pdMS_TO_TICKS(400));
	}
}

void BTPing(void* p) {
	while (true) {
		BT.println("Ping");
		Serial.println("ping");
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

void setup() {
	// Debug only
	Serial.begin(115200);
	// Communication with vesc
	Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
	vescUart = new VescUart();
	vescUart->setSerialPort(&Serial2);

	buzzer = new Buzzer(BUZZER_PIN);
	// Turn on backlight
	pinMode(TFT_LED, OUTPUT);
	digitalWrite(TFT_LED, HIGH);
	WireBus::init();

	tft.begin();
	tft.setRotation(0);
	BT.begin("UniVesc Ctrl");

	delay(500);

	switchToMode(Demo, true);

	createBattery();
	createPower();
	createSpeedGauge();
	createTemps();
	createClock();
	createOdometer();


	xTaskCreatePinnedToCore(UpdateTFT, "UpdateTFT", 4096, NULL, 3, NULL, ARDUINO_RUNNING_CORE);
	xTaskCreatePinnedToCore(ReadVesc, "ReadVesc", 4096, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
	xTaskCreatePinnedToCore(ReadTime, "ReadTime", 4096, NULL, 10, NULL, ARDUINO_RUNNING_CORE);
	xTaskCreatePinnedToCore(ReadBTUart, "ReadBTUart", 4096, NULL, 5, NULL, ARDUINO_RUNNING_CORE);
	xTaskCreatePinnedToCore(BTPing, "BTPing", 4096, NULL, 10, NULL, ARDUINO_RUNNING_CORE);
}

void loop() {
	// uint32_t m = millis();

	// if (m - lastRefresh >= 250) {
	// 	Serial.println("UPD");
	// 	WireBus::getTime();

	// 	// if (Serial.available()) {
	// 	// 	SerialBT.write(Serial.read());
	// 	// }
	// 	if (SerialBT.available()) {
	// 		Serial.write(SerialBT.read());
	// 	}
	// 	lastRefresh = m;
	// }

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