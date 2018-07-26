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

#endif // USB_MIDI_H
