#include "Buzzer.h"

uint8_t channel = 5;

Buzzer::Buzzer(int pin) {
	this->pin = pin;
	pinMode(pin, OUTPUT);
	ledcSetup(channel, 0, 16);
}

void Buzzer::startBeep(unsigned int frequency) {
	// if (ledcRead(channel)) {
	// 	log_e("Tone channel %d is already in use", channel);
	// 	return;
	// }

	ledcAttachPin(pin, channel);
	ledcWriteTone(channel, frequency);
}

void Buzzer::stopBeep() {
	ledcDetachPin(pin);
	// ledcWrite(channel, 0);
}