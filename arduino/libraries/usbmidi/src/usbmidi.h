/* 
 * Copyright (C) 2015 Blokas Labs
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef USB_MIDI_H
#define USB_MIDI_H

#include <Arduino.h>
#include <Stream.h>

#if defined(USBCON)

// Requires Arduino IDE version 1.6.7.
// todo: add compile-time check?

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

	// Send data using MIDI USB protocol.
	void sendUSB(u8 midiUsbEvent, u8 data1, u8 data2, u8 data3);
};

extern USBMIDI_ USBMIDI;

#endif // USBCON

#endif // USB_MIDI_H
