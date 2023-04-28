#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>

#include "AzeretMono.h"
#include "BluetoothSerial.h"
#include "Buzzer.h"
#include "Icons.h"
#include "VescUart.h"
#include "WireBus.h"
#include "widget/Clock.h"
#include "widget/Gauge.h"
#include "widget/ProgressBar.h"

// APP SETTINGS START
#define WHEEL_DIAMETER 0.5 // in meters
#define MOTOR_PULLEY 1.0
#define WHEEL_PULLEY 1.0
#define MOTOR_POLE_PAIRS 7.0
#define MAX_BATTERY_VOLTAGE 84.0
#define MIN_BATTERY_VOLTAGE 50.0
#define MAX_MOTOR_CURRENT 35.0

// Display positions
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

float tachometer = 0;
float rpm = 0;
float distance = 0;
double velocity = 0;
float batPercentage = 0;
float motorTemp = 0;
float mosfetTemp = 0;
float powerWatt = 0;
float powerPercent = 0;
float voltage = 0;

AsyncWebServer server(80);

// Main classes
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
TFT_eSprite* mosfetTempText = nullptr;
TFT_eSprite* motorTempText = nullptr;
BluetoothSerial BT;
bool clearRequested = false;

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

void drawAllImages() {
	drawImage(VOLTAGE_X + 1, 5, 32, 32, battery32);
	drawImage(POWER_X, 5, 32, 32, engine32);
	drawImage(TEMP_MCU_X + 32, TEMP_MCU_Y, 32, 32, cpu32);
	drawImage(TEMP_MOSFET_X + 32, TEMP_MOSFET_Y, 32, 32, car_engine32);
}

void createBattery() {
	battery = new ProgressBar(&tft, 2, 30, 100, TFT_WHITE, TFT_BROWN);
	battery->setPosition(VOLTAGE_X, 40);
	battery->mode = ProgressBar::Mode::VerticalReversed;

	voltageText = new TFT_eSprite(&tft);
	voltageText->setColorDepth(16);
	voltageText->createSprite(TEXTSPR_WIDTH, 20);
	voltageText->setTextDatum(TL_DATUM);
	voltageText->loadFont(AzeretMono16);
	voltageText->setTextColor(TFT_GREEN);
}

void drawBattery() {
	uint8_t red = 255 - 255 * (batPercentage / 100);
	uint8_t green = 255 * (batPercentage / 100);
	uint16_t newColor = (((31 * (red + 4)) / 255) << 11) | (((63 * (green + 2)) / 255) << 5) | ((31 * (0 + 4)) / 255);
	battery->fillColor = newColor;
	battery->setProgress(batPercentage);
	battery->draw();

	voltageText->fillRect(0, 0, TEXTSPR_WIDTH, 30, TFT_BLACK);
	voltageText->drawString(String(voltage, 1) + "V", 0, 0);
	voltageText->pushSprite(10, 160);
}

void createPower() {
	power = new ProgressBar(&tft, 2, 30, 100, TFT_WHITE, TFT_BROWN);
	power->setPosition(POWER_X, 40);
	power->mode = ProgressBar::Mode::VerticalReversed;

	powerText = new TFT_eSprite(&tft);
	powerText->setColorDepth(16);
	powerText->createSprite(TEXTSPR_WIDTH, 20);

	powerText->setTextDatum(TL_DATUM);
	powerText->loadFont(AzeretMono16);
	powerText->setTextColor(TFT_GREEN);
}

void drawPower() {
	power->setProgress(powerPercent);
	power->draw();

	String vol = String(abs(powerWatt) / 1000, 1);
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
	mosfetTempText = new TFT_eSprite(&tft);
	mosfetTempText->setColorDepth(16);
	mosfetTempText->createSprite(TEXTSPR_WIDTH, 20);

	mosfetTempText->setTextDatum(TL_DATUM);
	mosfetTempText->loadFont(AzeretMono22);
	mosfetTempText->setTextColor(TFT_WHITE);

	motorTempText = new TFT_eSprite(&tft);
	motorTempText->setColorDepth(16);
	motorTempText->createSprite(TEXTSPR_WIDTH, 20);

	motorTempText->setTextDatum(TL_DATUM);
	motorTempText->loadFont(AzeretMono22);
	motorTempText->setTextColor(TFT_WHITE);
}

void drawTemps() {
	mosfetTempText->fillRect(0, 0, TEXTSPR_WIDTH, 30, TFT_BLACK);
	mosfetTempText->drawString(String(mosfetTemp, 0), 0, 0);
	mosfetTempText->pushSprite(TEMP_MOSFET_X + 2 * 35, TEMP_MOSFET_Y + 5);

	motorTempText->fillRect(0, 0, TEXTSPR_WIDTH, 30, TFT_BLACK);
	motorTempText->drawString(String(motorTemp, 0), 0, 0);
	motorTempText->pushSprite(TEMP_MCU_X + 2 * 35, TEMP_MCU_Y + 5);

	// tft.drawString(String(motorTemp, 0), TEMP_MCU_X + 2 * 35, TEMP_MCU_Y + 5);	//
	// tft.drawString(String(mosfetTemp, 0), TEMP_MOSFET_X + 2 * 35, TEMP_MOSFET_Y + 5);
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

void drawSpeedGauge() {
	speedGauge->setValue(abs(velocity));
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
	// TODO: fix that
	float total = mode == Demo ? random(10000) / 10 : distance;
	float session = mode == Demo ? random(10000) / 10 : distance;

	odometerText->fillRect(0, 0, 260, 40, TFT_BLACK);
	odometerText->drawString("T: " + String(total, 1) + "km  S: " + String(session, 1) + "km", 0, 20);
	odometerText->pushSprite(20, 260);
}

void switchToMode(AppMode m, bool force = false) {
	if (mode == m && !force) {
		return;
	}
	mode = m;

	clearRequested = true;
	// clearScreen();
}

void UpdateTFT(void* pvParameters) {
	while (true) {
		if (clearRequested) {
			clearScreen();
			clearRequested = false;
			drawAllImages();
		}

		drawBattery();
		drawPower();
		drawSpeedGauge();
		drawTemps();
		drawOdometer();
		clkWidget->draw();

		vTaskDelay(pdMS_TO_TICKS(250));
	}
}

void ReadVesc(void* params) {
	while (true) {
		if (vescUart->getVescValues()) {
			// Serial.println(vescUart->data.rpm);
			// Serial.println(vescUart->data.inpVoltage);
			// Serial.println(vescUart->data.ampHours);
			// Serial.println(vescUart->data.tachometerAbs);
			// Serial.println(vescUart->data.tempMosfet);
			// Serial.println(vescUart->data.tempMotor);
			// Serial.println(vescUart->appData.pitch);
			// Serial.println(vescUart->appData.roll);

			motorTemp = vescUart->data.tempMotor;
			random(100);
			mosfetTemp = vescUart->data.tempMosfet;

			voltage = vescUart->data.inpVoltage;
			powerPercent = abs(vescUart->data.avgMotorCurrent / MAX_MOTOR_CURRENT);
			powerWatt = vescUart->data.avgMotorCurrent * voltage;

			// Code borrowed from
			// https://github.com/TomStanton/VESC_LCD_EBIKE/blob/master/VESC6_LCD_EBIKE.ino

			// The '42' is the number of motor poles multiplied by 3
			tachometer = (vescUart->data.tachometerAbs) / MOTOR_POLE_PAIRS * 3;
			rpm = (vescUart->data.rpm) / MOTOR_POLE_PAIRS;
			// Motor tacho x Pi x (1 / meters in a mile or km) x Wheel diameter x (motor pulley / wheelpulley)
			distance = tachometer * 3.1415 * (1.0 / 1000.0) * WHEEL_DIAMETER  * (MOTOR_PULLEY / WHEEL_PULLEY);
			// Motor RPM x Pi x (seconds in a minute / meters in a kilometer) x Wheel diameter x (motor pulley /
			// wheelpulley)
			velocity = rpm * 3.1415 * WHEEL_DIAMETER * (60.0 / 1000.0) * (MOTOR_PULLEY / WHEEL_PULLEY);
			// ((Battery voltage - minimum voltage) / number of cells) x 100
			batPercentage =
				100 * (vescUart->data.inpVoltage - MIN_BATTERY_VOLTAGE) / (MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE);

			Serial.println(rpm);
			Serial.println(rpm * 3.1415 * WHEEL_DIAMETER);
			Serial.println((60.0 / 1000.0) * (MOTOR_PULLEY / WHEEL_PULLEY));
			Serial.println(velocity);
			switchToMode(Live);
		}
		else {
			// Serial.println("Failed to get data!");
			velocity = random(80);
			voltage = random(MAX_BATTERY_VOLTAGE * 10) / 10;
			batPercentage = random(100);
			powerWatt = random(2200);
			powerPercent = random(100);
			motorTemp = random(500) / 10;
			mosfetTemp = random(500) / 10;

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

void handleUpload(
	AsyncWebServerRequest* request,
	String filename,
	size_t index,
	uint8_t* data,
	size_t len,
	bool final
) {
	if (!index) {
		Serial.printf("UploadStart: %s\n", filename.c_str());
	}

	for (size_t i = 0; i < len; i++) {
		Serial.write(data[i]);
	}

	if (final) {
		Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index + len);
	}
}

void handleUpdateTime(
	AsyncWebServerRequest* request,
	String filename,
	size_t index,
	uint8_t* data,
	size_t len,
	bool final
) {
	if (!index) {
		Serial.printf("UploadStart: %s\n", filename.c_str());
	}

	for (size_t i = 0; i < len; i++) {
		Serial.write(data[i]);
	}

	if (final) {
		Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index + len);
	}
}

const char* ssid = "ESP32-Access-Point";
const char* password = "12345678";
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

	WiFi.mode(WIFI_AP);
	WiFi.softAP(ssid, password);

	IPAddress IP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(IP);

	server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(200, "text/html", "<h1>Hello</h1>"); });

	server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(
			200,
			"text/html",
			"<form method='post' enctype='multipart/form-data' action='/upfile'><input type='file' name='f'><button "
			"type='submit'>Up</button></form>"
		);
	});

	server.on("/time", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(
			200,
			"text/html",
			"<form method='post' action='/update-time'><input id='x' type='text' name='d'>"
			"<input id='dow' type='text' name='dow'>"
			"<button type='submit'>Up</button></form><script>"
			"const d = new Date();"
			"const dfu = date => new Date(date.getTime() + date.getTimezoneOffset() * -60 * "
			"1000).toISOString().slice(0, 19);"
			"document.getElementById('x').value =dfu(d);"
			"document.getElementById('dow').value = (d.getDay() + 6) % 7 + 1;"
			"</script>"
		);
	});

	server.on(
		"/upfile", HTTP_POST, [](AsyncWebServerRequest* request) { request->send(200); }, handleUpload
	);

	server.on("/update-time", HTTP_POST, [](AsyncWebServerRequest* request) {
		if (request->hasParam("d")) {
			auto d = request->getParam("d");
			auto val = d->value();

			auto dow = request->getParam("dow");
			auto dowVal = dow->value();

			Serial.println(val);
			Serial.println(dowVal);

			auto year = val.substring(0, 3);
			auto month = val.substring(5, 7);
			auto day = val.substring(8, 10);
			auto hour = val.substring(11, 13);
			auto minute = val.substring(14, 16);
			auto second = val.substring(17, 19);

			Serial.println(year);
			Serial.println(month);
			Serial.println(day);
			Serial.println(hour);
			Serial.println(minute);
			Serial.println(second);
		}
		request->send(200);
	});

	server.begin();

	delay(500);

	switchToMode(Demo, true);

	createBattery();
	createPower();
	createSpeedGauge();
	createTemps();
	createClock();
	createOdometer();

	xTaskCreatePinnedToCore(UpdateTFT, "UpdateTFT", 4096, NULL, 1, NULL, 0);
	xTaskCreatePinnedToCore(ReadVesc, "ReadVesc", 4096, NULL, 5, NULL, ARDUINO_RUNNING_CORE);
	xTaskCreatePinnedToCore(ReadTime, "ReadTime", 4096, NULL, 2, NULL, ARDUINO_RUNNING_CORE);

	Serial.print("Starting up app");
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

	// if (Serial.available()) {
	// 	BT.write(Serial.read());
	// }
	// if (BT.available()) {
	// 	Serial.write(BT.read());
	// }

	// vTaskDelay(pdMS_TO_TICKS(20));
}