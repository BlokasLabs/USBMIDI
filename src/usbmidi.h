/* 
 * Copyright (C) 2015-2018 UAB Vilniaus Blokas
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. Implementation for microcontrollers without built-in
 * USB support depends on V-USB library by Objective Development,
 * https://www.obdev.at/vusb/ licensed under GPLv2 license.
 *
 * See the LICENSE file for details.
 */

#ifndef USB_MIDI_H
#define USB_MIDI_H

#include <Stream.h>

class USBMIDI_ : public Stream
{
public:
	USBMIDI_();

	// Stream interface.
	virtual int available();
	virtual int read();
	virtual int peek();
	virtual void flush();

	// Print interface.
	virtual size_t write(uint8_t c);

	// Poll for new USB data. Should be called from loop() to handle incoming MIDI data.
	void poll();
};

extern USBMIDI_ USBMIDI;

/*
 * USBMIDI_DEFINE_VENDOR_NAME and USB_DEFINE_PRODUCT_NAME macros can be used to customize the USB Device strings.
 * Instead of accepting regular double-quote strings, the strings must be provided as single chars in
 * single quotes. For example:
 *
 * USBMIDI_DEFINE_VENDOR_NAME('b', 'l', 'o', 'k', 'a', 's', '.', 'i', 'o');
 *
 * This works only on V-USB based implementation for now.
 *
 * As of writing, if using 1.8.5 Arduino IDE or earlier, this must be placed in a .cpp source file
 * instead of .ino due to a conflict with Arduino sketch preprocessing. Reported issue:
 *
 * https://github.com/arduino/arduino-builder/issues/303
 *
 * The issue is already fixed in 1.9.0-beta Arduino IDE.
 */
#include <avr/pgmspace.h>

#define USBMIDI_DEFINE_STRING(stringId, ...) \
	unsigned char _usbmidi_get_ ## stringId ## _string(const unsigned char *&data) { \
		static const char _TMP[] = { __VA_ARGS__ }; \
		static const PROGMEM int _STRING[] = { \
			(2*(sizeof(_TMP))+2) | (3<<8), \
			__VA_ARGS__ \
		}; \
		data = (const unsigned char *)_STRING; \
		return sizeof(_STRING); \
	}

#define USBMIDI_DEFINE_VENDOR_NAME(...) \
	USBMIDI_DEFINE_STRING(vendor, __VA_ARGS__)

#define USBMIDI_DEFINE_PRODUCT_NAME(...) \
	USBMIDI_DEFINE_STRING(product, __VA_ARGS__)

#endif // USB_MIDI_H
