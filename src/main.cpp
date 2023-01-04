
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>  // Graphics library

#include "NotoSansBold15.h"
#include "NotoSansBold36.h"
#include "VescUart.h"
#include "widget/ProgressBar.h"
#include "widget/Gauge.h"
#include "widget/Clock.h"

#define WHEEL_DIAMETER 51.8
#define TFT_LED 21	// Backlight

/** Initiate VescUart class */
VescUart vescUart;
#define NEEDLE_LENGTH 25		  // Visible length
#define NEEDLE_WIDTH 5			  // Width of needle - make it an odd number
#define SECTIONS_LENGTH 15		  // Visible length
#define SECTIONS_WIDTH 1		  // Width of needle - make it an odd number
#define SECTIONS_RADIUS 80		  // Radius at tip
#define NEEDLE_RADIUS 70		  // Radius at tip
#define NEEDLE_COLOR1 TFT_MAROON  // Needle periphery colour
#define NEEDLE_COLOR2 TFT_RED	  // Needle centre colour
#define DIAL_CENTRE_X 160
#define DIAL_CENTRE_Y 100

// Font attached to this sketch
#define AA_FONT_LARGE NotoSansBold36
#define AA_FONT_MID NotoSansBold15

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite needle = TFT_eSprite(&tft);	   // Sprite object for needle
TFT_eSprite sections = TFT_eSprite(&tft);  // Sprite object for needle
TFT_eSprite spr = TFT_eSprite(&tft);	   // Sprite for meter reading
TFT_eSprite battery = TFT_eSprite(&tft);   // Sprite for meter reading
TFT_eSprite power = TFT_eSprite(&tft);	   // Sprite for meter reading

ProgressBar range = ProgressBar(&tft, 2, 30, 100, TFT_WHITE, TFT_BROWN);
Gauge gaugex = Gauge(&tft, 160, 1, TFT_WHITE);
Clock clkWidget = Clock(&tft, 160, 40);

uint16_t* tft_buffer;
bool buffer_loaded = false;
uint16_t spr_width = 0;
uint16_t bg_color = 0;

// =======================================================================================
// Create the needle Sprite
// =======================================================================================
// void createNeedle(void) {
// 	needle.setColorDepth(16);
// 	needle.createSprite(NEEDLE_WIDTH, NEEDLE_LENGTH);  // create the needle Sprite

// 	needle.fillSprite(TFT_BLACK);  // Fill with black

// 	// Define needle pivot point relative to top left corner of Sprite
// 	uint16_t piv_x = NEEDLE_WIDTH / 2;	// pivot x in Sprite (middle)
// 	uint16_t piv_y = NEEDLE_RADIUS;		// pivot y in Sprite
// 	needle.setPivot(piv_x, piv_y);		// Set pivot point in this Sprite

// 	// Draw the red needle in the Sprite
// 	needle.fillRect(0, 0, NEEDLE_WIDTH, NEEDLE_LENGTH, TFT_MAROON);
// 	needle.fillRect(1, 1, NEEDLE_WIDTH - 2, NEEDLE_LENGTH - 2, TFT_RED);

// 	// Bounding box parameters to be populated
// 	int16_t min_x;
// 	int16_t min_y;
// 	int16_t max_x;
// 	int16_t max_y;

// 	// Work out the worst case area that must be grabbed from the TFT,
// 	// this is at a 45 degree rotation
// 	needle.getRotatedBounds(45, &min_x, &min_y, &max_x, &max_y);

// 	// Calculate the size and allocate the buffer for the grabbed TFT area
// 	tft_buffer = (uint16_t*)malloc(((max_x - min_x) + 2) * ((max_y - min_y) + 2) * 2);
// }

int angleToSpeed(int angle) {
	return (int)(angle / 3);
}

// void drawSections() {
// 	sections.setColorDepth(16);
// 	sections.createSprite(SECTIONS_WIDTH, SECTIONS_LENGTH);	 // create the needle Sprite

// 	sections.fillSprite(TFT_BLACK);	 // Fill with black

// 	// Define needle pivot point relative to top left corner of Sprite
// 	uint16_t piv_x = SECTIONS_WIDTH / 2;  // pivot x in Sprite (middle)
// 	uint16_t piv_y = SECTIONS_RADIUS;	  // pivot y in Sprite
// 	sections.setPivot(piv_x, piv_y);	  // Set pivot point in this Sprite

// 	// Draw the red needle in the Sprite
// 	sections.fillRect(0, 0, SECTIONS_WIDTH, SECTIONS_LENGTH, TFT_WHITE);
// 	// sections.fillRect(1, 1, SECTIONS_WIDTH - 2, SECTIONS_LENGTH - 2, TFT_WHITE);

// 	for (int i = 0; i <= 240; i += 30) {
// 		int angle = i - 120;
// 		sections.pushRotated(angle, TFT_BLACK);
// 		// x = radius * Math.sin(Math.PI * 2 * angle / 360);

// 		// y = radius * Math.cos(Math.PI * 2 * angle / 360);
// 		tft.drawNumber(
// 			80 - (long)(i / 3),
// 			DIAL_CENTRE_X + (SECTIONS_RADIUS + 10) * sin(PI * 2 * (angle + 180) / 360),
// 			DIAL_CENTRE_Y + (SECTIONS_RADIUS + 10) * cos(PI * 2 * (angle + 180) / 360)
// 		);
// 	}
// }

void drawTemps() {
	String stringOne = String(vescUart.data.tempMotor, 0);
	String stringTwo = String(vescUart.data.tempMosfet, 0);
	stringOne.concat("°C");
	stringTwo.concat("°C");
	// tft.drawString("Tm: ", 30, 200);
	// tft.drawString("Te: ", 30, 230);
	tft.drawString("Tm: " + stringOne, 30, 200);
	tft.drawString("Te: " + stringTwo, 30, 230);
}

void createBattery() {
	battery.setColorDepth(16);
	battery.createSprite(50, 140);

	battery.fillRect(0, 20, 30, 80, TFT_WHITE);
	battery.fillRect(2, 22, 30 - 4, 80 - 4, TFT_BLACK);

	battery.loadFont(AA_FONT_MID);
	battery.setTextColor(TFT_WHITE, TFT_BLACK, true);
	battery.drawString("00%", 0, 0);
	battery.setTextColor(TFT_DARKCYAN, TFT_BLACK, true);
	battery.drawString("150km", 0, 100);
	battery.setTextColor(TFT_OLIVE, TFT_BLACK, true);
	battery.drawString("84.1V", 0, 120);

	battery.pushSprite(15, 15);
}

void drawBattery() {
	float pct = (vescUart.data.ampHoursCharged / vescUart.data.ampHours) * 100;
	if (pct > 100) {
		pct = 100;
	}
	if (pct < 0 || isnan(pct)) {
		pct = 0;
	}
	// Serial.println(pct);
	String p = String(pct, 0);
	battery.setTextColor(TFT_WHITE, TFT_BLACK, true);
	battery.drawString(p + "%", 0, 0);
	int height = (80 - 8) * (pct / 100);
	battery.fillRect(2, 22, 30 - 4, 80 - 4, TFT_BLACK);
	battery.fillRect(4, 24 + (80 - 8 - height), 30 - 8, height, TFT_GREEN);

	battery.setTextColor(TFT_OLIVE, TFT_BLACK, true);
	battery.drawString(String(vescUart.data.inpVoltage, 1) + "V", 0, 120);

	battery.pushSprite(15, 15);
}

void drawRange() {
	tft.setTextColor(TFT_DARKCYAN, TFT_BLACK, true);
	tft.drawString(String(vescUart.data.ampHoursCharged, 1), 0, 100);
}

// =======================================================================================
// Move the needle to a new position
// =======================================================================================
// void plotNeedle(int16_t angle, uint16_t ms_delay) {
// 	static int16_t old_angle = -120;  // Starts at -120 degrees

// 	// Bounding box parameters
// 	static int16_t min_x;
// 	static int16_t min_y;
// 	static int16_t max_x;
// 	static int16_t max_y;

// 	if (angle < 0) angle = 0;  // Limit angle to emulate needle end stops
// 	if (angle > 240) angle = 240;

// 	angle -= 120;  // Starts at -120 degrees

// 	// Move the needle until new angle reached
// 	while (angle != old_angle || !buffer_loaded) {
// 		if (old_angle < angle)
// 			old_angle++;
// 		else
// 			old_angle--;

// 		// Only plot needle at even values to improve plotting performance
// 		if ((old_angle & 1) == 0) {
// 			if (buffer_loaded) {
// 				// Paste back the original needle free image area
// 				tft.pushRect(min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y, tft_buffer);
// 			}

// 			if (needle.getRotatedBounds(old_angle, &min_x, &min_y, &max_x, &max_y)) {
// 				// Grab a copy of the area before needle is drawn
// 				tft.readRect(min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y, tft_buffer);
// 				buffer_loaded = true;
// 			}

// 			// Draw the needle in the new position, black in needle image is transparent
// 			needle.pushRotated(old_angle, TFT_BLACK);

// 			// Wait before next update
// 			delay(ms_delay);
// 		}

// 		// Update the number at the centre of the dial
// 		spr.setTextColor(TFT_WHITE, bg_color, true);
// 		spr.drawNumber((old_angle + 120) / 3, spr_width / 2, spr.fontHeight() / 2);
// 		spr.pushSprite(DIAL_CENTRE_X - spr_width / 2, DIAL_CENTRE_Y - spr.fontHeight() / 2);

// 		// Slow needle down slightly as it approaches the new position
// 		if (abs(old_angle - angle) < 10) ms_delay += ms_delay / 5;
// 	}
// }

// =======================================================================================
// Setup
// =======================================================================================
void setup() {
	Serial.begin(115200);  // Debug only
	pinMode(TFT_LED, OUTPUT);
	digitalWrite(TFT_LED, HIGH);
	pinMode(13, OUTPUT);

	tft.begin();
	tft.setRotation(3);
	tft.fillScreen(TFT_BLACK);

	// // Draw the dial
	// tft.fillCircle(DIAL_CENTRE_X, DIAL_CENTRE_Y, NEEDLE_RADIUS - NEEDLE_LENGTH, TFT_WHITE);
	// tft.fillCircle(DIAL_CENTRE_X, DIAL_CENTRE_Y, NEEDLE_RADIUS - NEEDLE_LENGTH - 3, TFT_BLACK);

	// // Load the font and create the Sprite for reporting the value
	// spr.loadFont(AA_FONT_LARGE);
	// spr_width = spr.textWidth("77");  // 7 is widest numeral in this font
	// spr.createSprite(spr_width, spr.fontHeight());

	// bg_color = tft.readPixel(DIAL_CENTRE_X, DIAL_CENTRE_Y);	 // Get colour from dial centre
	// spr.fillSprite(bg_color);
	// spr.setTextColor(TFT_WHITE, bg_color, true);
	// spr.setTextDatum(MC_DATUM);
	// spr.setTextPadding(spr_width);
	// spr.drawNumber(0, spr_width / 2, spr.fontHeight() / 2);
	// spr.pushSprite(DIAL_CENTRE_X - spr_width / 2, DIAL_CENTRE_Y - spr.fontHeight() / 2);

	tft.loadFont(AA_FONT_MID);
	// tft.setTextColor(TFT_WHITE, bg_color);
	tft.setTextDatum(MC_DATUM);
	// tft.drawString("km/h", DIAL_CENTRE_X, DIAL_CENTRE_Y + 32, 3);

	// // Define where the needle pivot point is on the TFT before
	// // creating the needle so boundary calculation is correct
	tft.setPivot(DIAL_CENTRE_X, DIAL_CENTRE_Y);

	// // Create the needle Sprite
	// createNeedle();
	createBattery();

	// drawSections();
	drawTemps();

	// Reset needle position to 0
	// plotNeedle(0, 0);
	range.setPosition(260, 20);
	range.mode = ProgressBar::Mode::VerticalReversed;
	range.draw();

	gaugex.setSmallFont(AA_FONT_MID);
	gaugex.setBigFont(AA_FONT_LARGE);
	gaugex.setPosition(80, 10);
	gaugex.setSections(0, 80, 8, 6, 1.5);
	gaugex.setTopLabel("Speed");
	gaugex.setBottomLabel("km/h");
	gaugex.setValue(50);
	gaugex.draw();

	clkWidget.setPosition(80, 180);
	clkWidget.setBigFont(AA_FONT_LARGE);
	clkWidget.draw();

	delay(2000);
}

int spp = 0;
bool blink = false;
void loop() {
	uint16_t angle = random(241);  // random speed in range 0 to 240
	float pct = random(100);
	// Plot needle at random angle in range 0 to 240, speed 40ms per increment
	// plotNeedle(angle, 30);
	drawBattery();
	range.setProgress(pct);
	range.draw();
	spp++;
	if (spp > 80) {
		spp = 0;
	}
	gaugex.setValue(spp);
	gaugex.draw();
	clkWidget.draw();
	// Pause at new position

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
	clkWidget.getTime();
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