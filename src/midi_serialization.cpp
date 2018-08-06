/* 
 * Copyright (C) 2015-2018 UAB Vilniaus Blokas
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include "midi_serialization.h"
#include "midi_messages.h"

extern "C" unsigned midi_get_data_length(midi_event_t ev)
{
	// http://www.usb.org/developers/docs/devclass_docs/midi10.pdf, page 16
	switch (ev.m_event & 0x0f)
	{
	case 0x5:
	case 0xF:
		// 1 byte message
		return 1;
	case 0x2:
	case 0x6:
	case 0xC:
	case 0xD:
		// 2 byte message
		return 2;
	case 0x3:
	case 0x4:
	case 0x7:
	case 0x8:
	case 0x9:
	case 0xA:
	case 0xB:
	case 0xE:
		// 3 byte message
		return 3;
	default: // Unhandled 0x0 and 0x1 ("reserved for future")
		return 0;
	}
}

MidiToUsb::MidiToUsb()
	:m_cable(0)
	,m_status(0)
	,m_counter(0)
	,m_sysex(false)
{
}

MidiToUsb::MidiToUsb(int cable)
	:m_cable((cable & 0x0f) << 4)
	,m_status(0)
	,m_counter(0)
	,m_sysex(false)
{
	for (unsigned i=0; i<sizeof(m_data); ++i)
	{
		m_data[i] = 0;
	}
}

void MidiToUsb::reset()
{
	m_status = 0;
	m_counter = 0;
	m_sysex = false;
}

void MidiToUsb::setCable(int cable)
{
	m_cable = (cable & 0x0f) << 4;
}

int MidiToUsb::getCable() const
{
	return m_cable >> 4;
}

bool MidiToUsb::process(uint8_t byte, midi_event_t &out)
{
	if (byte & 0x80) // Status byte received.
	{
		if (midi_is_real_time(byte))
		{
			out.m_event = m_cable | 0x0f;
			out.m_data[0] = byte;
			out.m_data[1] = 0;
			out.m_data[2] = 0;
			//m_counter = 0;
			return true;
		}
		else if (midi_is_single_byte_system_common(byte))
		{
			out.m_event = m_cable | 0x05;
			out.m_data[0] = byte;
			out.m_data[1] = 0;
			out.m_data[2] = 0;
			m_counter = 0;
			return true;
		}
		else if (midi_is_sysex_start(byte))
		{
			m_sysex = true;
			m_data[0] = byte;
			m_counter = 1;
			return false;
		}
		else if (midi_is_sysex_end(byte))
		{
			m_data[m_counter++] = byte;
			out.m_event = m_cable | (0x04 + m_counter);
			unsigned i=0;
			for (; i<m_counter; ++i)
				out.m_data[i] = m_data[i];
			for (; i<sizeof(m_data); ++i)
				out.m_data[i] = 0x00;
			m_sysex = false;
			m_counter = 0;
			return true;
		}
		else
		{
			m_status = byte;
			m_counter = 0;
			return false;
		}
	}
	else // Data byte received.
	{
		m_data[m_counter++] = byte;

		if (m_sysex)
		{
			if (m_counter == 3)
			{
				out.m_event = m_cable | 0x04;
				out.m_data[0] = m_data[0];
				out.m_data[1] = m_data[1];
				out.m_data[2] = m_data[2];
				m_counter = 0;
				return true;
			}
			return false;
		}

		switch (m_status & 0xf0)
		{
		case 0xf0:
			switch (m_status & 0x0f)
			{
			case 0x1: // MTC
			case 0x3: // Song Select
				out.m_event = m_cable | 0x02;
				out.m_data[0] = m_status;
				out.m_data[1] = m_data[0];
				out.m_data[2] = 0x00;
				m_counter = 0;
				return true;
			case 0x2: // Song Position Pointer
				if (m_counter == 2)
				{
					out.m_event = m_cable | 0x03;
					out.m_data[0] = m_status;
					out.m_data[1] = m_data[0];
					out.m_data[2] = m_data[1];
					m_counter = 0;
					return true;
				}
				break;
			}
			break;
		case 0x80:
		case 0x90:
		case 0xA0:
		case 0xB0:
		case 0xE0:
			if (m_counter == 2)
			{
				out.m_event = m_cable | (m_status >> 4);
				out.m_data[0] = m_status;
				out.m_data[1] = m_data[0];
				out.m_data[2] = m_data[1];
				m_counter = 0;
				return true;
			}
			break;
		case 0xC0:
		case 0xD0:
			if (m_counter == 1)
			{
				out.m_event = m_cable | (m_status >> 4);
				out.m_data[0] = m_status;
				out.m_data[1] = m_data[0];
				out.m_data[2] = 0x00;
				m_counter = 0;
				return true;
			}
			break;
		case 0x00:
			m_counter = 0;
			return false;
		}
	}

	return false;
}

unsigned UsbToMidi::process(midi_event_t in, uint8_t out[3])
{
	switch (in.m_event & 0x0f)
	{
	case 0x5:
	case 0xF:
		// 1 byte message
		out[0] = in.m_data[0];
		return 1;
	case 0x2:
	case 0x6:
	case 0xC:
	case 0xD:
		// 2 byte message
		out[0] = in.m_data[0];
		out[1] = in.m_data[1];
		return 2;
	case 0x3:
	case 0x4:
	case 0x7:
	case 0x8:
	case 0x9:
	case 0xA:
	case 0xB:
	case 0xE:
		// 3 byte message
		out[0] = in.m_data[0];
		out[1] = in.m_data[1];
		out[2] = in.m_data[2];
		return 3;
	default: // unhandled 0x0 and 0x1 ("reserved for future")
		return 0;
	}
}

extern "C" unsigned usb_to_midi(struct midi_event_t in, uint8_t out[3])
{
	return UsbToMidi::process(in, out);
}
