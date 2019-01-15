#include <usbmidi.h>

void setup() {
	Serial.begin(9600);
}

void loop() {
	//Handle USB communication
	USBMIDI.poll();

	// While there's MIDI USB input available...
	while (USBMIDI.available()) {
		u8 b = USBMIDI.read();

		// Output the byte to the Serial interface.
		Serial.print(b >> 4, HEX);
		Serial.print(b & 0xf, HEX);
		Serial.print('\n');

		// Echo the byte back to the MIDI USB output.
		USBMIDI.write(b);
		USBMIDI.flush();
	}
}
