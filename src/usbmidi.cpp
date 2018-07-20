/* 
 * Copyright (C) 2015 Blokas Labs
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include "usbmidi.h"
#include <PluggableUSB.h>

#if defined(USBCON)

#include "midi_serialization.h"

#define D_AUDIO_CONTROL_INTERFACE(interfaceNumber) \
	0x09, 0x04, interfaceNumber, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00

#define D_AUDIO_CONTROL_INTERFACE_SPC(interfaceNumber) \
	0x09, 0x24, 0x01, 0x00, 0x01, 0x09, 0x00, 0x01, interfaceNumber

#define D_AUDIO_STREAM_INTERFACE(interfaceNumber) \
	0x09, 0x04, interfaceNumber, 0x00, 0x02, 0x01, 0x03, 0x00, 0x00

#define D_AUDIO_STREAM_INTERFACE_SPC(totalLength) \
	0x07, 0x24, 0x01, 0x00, 0x01, totalLength & 0xff, (totalLength & 0xff00) >> 8

#define D_JACK_TYPE_EMBEDDED 0x01
#define D_JACK_TYPE_EXTERNAL 0x02

#define D_MIDI_IN_JACK(jackType, jackId) \
	0x06, 0x24, 0x02, jackType, jackId, 0x00

#define D_MIDI_OUT_JACK(jackType, jackId, sourceJack, sourcePin) \
	0x09, 0x24, 0x03, jackType, jackId, 0x01, sourceJack, sourcePin, 0x00

#define D_ENDPOINT_OUT 0x00
#define D_ENDPOINT_IN 0x80

#define D_MIDI_JACK_EP(endpointAddress) \
	0x09, 0x05, endpointAddress, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00

#define D_MIDI_JACK_EP_SPC(associatedJackId) \
	0x05, 0x25, 0x01, 0x01, associatedJackId

#ifndef USBMIDI_IN_BUFFER_SIZE
#define USBMIDI_IN_BUFFER_SIZE 64
#endif

class Fifo
{
public:
	Fifo();

	bool isEmpty() const;
	bool peek(u8 *byte) const;
	bool pop(u8 *byte);
	void push(u8 byte);
	bool hasSpaceFor(u8 numBytes) const;

private:
	uint8_t m_fifo[USBMIDI_IN_BUFFER_SIZE];
	uint8_t m_fifoHead;
	uint8_t m_fifoTail;
};

Fifo::Fifo()
	:m_fifoHead(0)
	,m_fifoTail(0)
{
}

bool Fifo::isEmpty() const
{
	return m_fifoHead == m_fifoTail;
}

bool Fifo::peek(u8 *byte) const
{
	if (isEmpty())
		return false;

	*byte = m_fifo[m_fifoHead];
	return true;
}

bool Fifo::pop(u8 *byte)
{
	if (!peek(byte))
		return false;

	m_fifoHead = (m_fifoHead + 1) % USBMIDI_IN_BUFFER_SIZE;
	return true;
}

void Fifo::push(u8 byte)
{
	u8 nextTail = (m_fifoTail + 1) % USBMIDI_IN_BUFFER_SIZE;

	if (nextTail == m_fifoHead)
		return;

	m_fifo[m_fifoTail] = byte;
	m_fifoTail = nextTail;
}

bool Fifo::hasSpaceFor(u8 numBytes) const
{
	return USBMIDI_IN_BUFFER_SIZE - ((USBMIDI_IN_BUFFER_SIZE + m_fifoTail - m_fifoHead) % USBMIDI_IN_BUFFER_SIZE) >= numBytes;
}

class UsbMidiModule : public PluggableUSBModule
{
public:
	static void install();

	inline static int available() { return getInstance()._available(); }
	inline static int read() { return getInstance()._read(); }
	inline static int peek() { return getInstance()._peek(); }
	inline static void flush() { return getInstance()._flush(); }

	inline static size_t write(uint8_t c) { return getInstance()._write(c); }

	inline static void poll() { return getInstance()._poll(); }

	inline static void sendUSB(u8 midiUsbEvent, u8 data1, u8 data2, u8 data3) { getInstance()._sendUSB(midiUsbEvent, data1, data2, data3); }

protected:
	virtual bool setup(USBSetup& setup);
	virtual int getInterface(uint8_t* interfaceCount);
	virtual int getDescriptor(USBSetup& setup);

private:
	UsbMidiModule();

	static UsbMidiModule &getInstance();

	int _available();
	int _read();
	int _peek();
	void _flush();
	size_t _write(uint8_t c);
	void _poll();
	void _sendUSB(u8 midiUsbEvent, u8 data1, u8 data2, u8 data3);

	uint8_t getInEndpointId() const;
	uint8_t getOutEndpointId() const;

	inline uint8_t getInterfaceId() const;

	static uint8_t s_endpointTypes[2];
	MidiToUsb m_midiToUsb;

	Fifo m_midiInFifo;
};

uint8_t UsbMidiModule::s_endpointTypes[2] =
{
	EP_TYPE_BULK_OUT,
	EP_TYPE_BULK_IN,
};

void UsbMidiModule::install()
{
	PluggableUSB().plug(&getInstance());
}

bool UsbMidiModule::setup(USBSetup &setup)
{
	return false;
}

int UsbMidiModule::getInterface(uint8_t *interfaceCount)
{
	*interfaceCount += 2;

	u8 desc[] =
	{
		D_AUDIO_CONTROL_INTERFACE(pluggedInterface),
		D_AUDIO_CONTROL_INTERFACE_SPC(0x03),
		D_AUDIO_STREAM_INTERFACE(pluggedInterface + 1),
		D_AUDIO_STREAM_INTERFACE_SPC(0x41),
		D_MIDI_IN_JACK(D_JACK_TYPE_EMBEDDED, 0x01),
		D_MIDI_IN_JACK(D_JACK_TYPE_EXTERNAL, 0x02),
		D_MIDI_OUT_JACK(D_JACK_TYPE_EMBEDDED, 0x03, 0x02, 0x01),
		D_MIDI_OUT_JACK(D_JACK_TYPE_EXTERNAL, 0x04, 0x01, 0x01),
		D_MIDI_JACK_EP(D_ENDPOINT_OUT | getOutEndpointId()),
		D_MIDI_JACK_EP_SPC(0x01),
		D_MIDI_JACK_EP(D_ENDPOINT_IN | getInEndpointId()),
		D_MIDI_JACK_EP_SPC(0x03),
	};

	return USB_SendControl(0, desc, sizeof(desc));
}

int UsbMidiModule::getDescriptor(USBSetup &setup)
{
	return 0;
}

UsbMidiModule::UsbMidiModule()
	:PluggableUSBModule(2, 2, s_endpointTypes)
	,m_midiToUsb(0)
{
}

UsbMidiModule& UsbMidiModule::getInstance()
{
	static UsbMidiModule instance;
	return instance;
}

int UsbMidiModule::_available()
{
	_poll();
	return !m_midiInFifo.isEmpty();
}

int UsbMidiModule::_read()
{
	_poll();

	u8 byte = 0;
	m_midiInFifo.pop(&byte);

	return byte;
}

int UsbMidiModule::_peek()
{
	_poll();

	u8 byte = 0;
	m_midiInFifo.peek(&byte);

	return byte;
}

void UsbMidiModule::_flush()
{
	USB_Flush(getInEndpointId());
}

size_t UsbMidiModule::_write(uint8_t c)
{
	midi_event_t midiEvent;
	if (m_midiToUsb.process(c, midiEvent))
	{
		USB_Send(getInEndpointId(), &midiEvent, sizeof(midiEvent));
	}

	return 1;
}

void UsbMidiModule::_poll()
{
	midi_event_t midiEvent;
	int numReceived;

	while (numReceived = USB_Recv(getOutEndpointId(), &midiEvent, sizeof(midiEvent)))
	{
		// MIDI USB messages are 4 bytes in size.
		if (numReceived != 4)
			return;

		// They get decoded to up to 3 MIDI Serial bytes.
		u8 data[3];

		unsigned count = UsbToMidi::process(midiEvent, data);

		if (m_midiInFifo.hasSpaceFor(count))
		{
			for (unsigned i=0; i<count; ++i)
			{
				m_midiInFifo.push(data[i]);
			}
		}
	}
}

void UsbMidiModule::_sendUSB(u8 midiUsbEvent, u8 data1, u8 data2, u8 data3)
{
	u8 packet[4];
	packet[0] = midiUsbEvent;
	packet[1] = data1;
	packet[2] = data2;
	packet[3] = data3;

	USB_Send(getInEndpointId(), packet, sizeof(packet));
}

uint8_t UsbMidiModule::getInEndpointId() const
{
	return pluggedEndpoint + 1;
}

uint8_t UsbMidiModule::getOutEndpointId() const
{
	return pluggedEndpoint;
}

uint8_t UsbMidiModule::getInterfaceId() const
{
	return pluggedInterface;
}

USBMIDI_ USBMIDI;

USBMIDI_::USBMIDI_()
{
	UsbMidiModule::install();
}

int USBMIDI_::available()
{
	return UsbMidiModule::available();
}

int USBMIDI_::read()
{
	return UsbMidiModule::read();
}

int USBMIDI_::peek()
{
	return UsbMidiModule::peek();
}

void USBMIDI_::flush()
{
	UsbMidiModule::flush();
}

size_t USBMIDI_::write(uint8_t c)
{
	return UsbMidiModule::write(c);
}

void USBMIDI_::poll()
{
	UsbMidiModule::poll();
}

#endif // USBCON
