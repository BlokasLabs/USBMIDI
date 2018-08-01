/* 
 * Copyright (C) 2015-2018 UAB Vilniaus Blokas
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef MIDI_SERIALIZATION_H
#define MIDI_SERIALIZATION_H

#include <stdint.h>

typedef unsigned char uint8_t;

struct midi_event_t
{
	uint8_t m_event;
	uint8_t m_data[3];
};

#ifdef __cplusplus

class MidiToUsb
{
public:
	MidiToUsb();
	explicit MidiToUsb(int cable);

	void reset();

	void setCable(int cable);
	int getCable() const;

	bool process(uint8_t byte, midi_event_t &out);

private:
	int m_cable;

	uint8_t m_status;
	uint8_t m_data[3];

	uint8_t m_counter;
	bool m_sysex;
};

class UsbToMidi
{
public:
	static unsigned process(midi_event_t in, uint8_t out[3]);
};

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

unsigned midi_get_data_length(struct midi_event_t ev);
unsigned usb_to_midi(struct midi_event_t in, uint8_t out[3]);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // MIDI_SERIALIZATION_H
