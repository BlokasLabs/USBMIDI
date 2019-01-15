#include <midi_serialization.h>
#include <usbmidi.h>

// See midictrl.png in the example folder for the wiring diagram,
// as well as README.md.

void sendCC(uint8_t channel, uint8_t control, uint8_t value) {
	USBMIDI.write(0xB0 | (channel & 0xf));
	USBMIDI.write(control & 0x7f);
	USBMIDI.write(value & 0x7f);
}

void sendNote(uint8_t channel, uint8_t note, uint8_t velocity) {
	USBMIDI.write((velocity != 0 ? 0x90 : 0x80) | (channel & 0xf));
	USBMIDI.write(note & 0x7f);
	USBMIDI.write(velocity &0x7f);
}

const int ANALOG_PIN_COUNT = 4;
const int BUTTON_PIN_COUNT = 2;

// Change the order of the pins to change the ctrl or note order.
int analogPins[ANALOG_PIN_COUNT] = { A3, A2, A1, A0 };
int buttonPins[BUTTON_PIN_COUNT] = { 9, 8 };

int ccValues[ANALOG_PIN_COUNT];
int buttonDown[BUTTON_PIN_COUNT];

int readCC(int pin) {
	// Convert from 10bit value to 7bit.
	return analogRead(pin) >> 3;
}

int isButtonDown(int pin) {
	return digitalRead(pin) == 0;
}

void setup() {
	// Initialize initial values.
	for (int i=0; i<ANALOG_PIN_COUNT; ++i) {
		pinMode(analogPins[i], INPUT);
		ccValues[i] = readCC(analogPins[i]);
	}

	for (int i=0; i<BUTTON_PIN_COUNT; ++i) {
		pinMode(buttonPins[i], INPUT);
		digitalWrite(buttonPins[i], HIGH);
		buttonDown[i] = isButtonDown(buttonPins[i]);
	}
}

void loop() {
	//Handle USB communication
	USBMIDI.poll();

	while (USBMIDI.available()) {
		// We must read entire available data, so in case we receive incoming
		// MIDI data, the host wouldn't get stuck.
		u8 b = USBMIDI.read();
	}

	for (int i=0; i<ANALOG_PIN_COUNT; ++i) {
		int value = readCC(analogPins[i]);

		// Send CC only if th has changed.
		if (ccValues[i] != value) {
			sendCC(0, i, value);
			ccValues[i] = value;
		}
	}

	for (int i=0; i<BUTTON_PIN_COUNT; ++i) {
		int down = isButtonDown(buttonPins[i]);

		if (down != buttonDown[i]) {
			sendNote(0, 64 + i, down ? 127 : 0);
			buttonDown[i] = down;
		}
	}

	// Flush the output.
	USBMIDI.flush();
}
