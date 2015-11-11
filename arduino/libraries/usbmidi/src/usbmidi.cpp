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

static u8 USBMIDI_ENDPOINT_IN;
static u8 USBMIDI_ENDPOINT_OUT;
static u8 USBMIDI_INTERFACE;

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

static int USBMIDI_GetInterface(u8* interfaceNum)
{
	*interfaceNum += 2;

	u8 desc[] =
	{
		D_AUDIO_CONTROL_INTERFACE(USBMIDI_INTERFACE),
		D_AUDIO_CONTROL_INTERFACE_SPC(0x03),
		D_AUDIO_STREAM_INTERFACE(USBMIDI_INTERFACE + 1),
		D_AUDIO_STREAM_INTERFACE_SPC(0x41),
		D_MIDI_IN_JACK(D_JACK_TYPE_EMBEDDED, 0x01),
		D_MIDI_IN_JACK(D_JACK_TYPE_EXTERNAL, 0x02),
		D_MIDI_OUT_JACK(D_JACK_TYPE_EMBEDDED, 0x03, 0x02, 0x01),
		D_MIDI_OUT_JACK(D_JACK_TYPE_EXTERNAL, 0x04, 0x01, 0x01),
		D_MIDI_JACK_EP(D_ENDPOINT_OUT | USBMIDI_ENDPOINT_OUT),
		D_MIDI_JACK_EP_SPC(0x01),
		D_MIDI_JACK_EP(D_ENDPOINT_IN | USBMIDI_ENDPOINT_IN),
		D_MIDI_JACK_EP_SPC(0x03),
	};

	return USB_SendControl(0, desc, sizeof(desc));
}

static int USBMIDI_GetDescriptor(int8_t t)
{
	return 0;
}

static bool USBMIDI_Setup(USBSetup& setup, u8 i)
{
	return false;
}

USBMIDI_ USBMIDI;
MidiToUsb midiToUsb(0);

#ifndef USBMIDI_IN_BUFFER_SIZE
#define USBMIDI_IN_BUFFER_SIZE 64
#endif

static u8 midi_in_fifo[USBMIDI_IN_BUFFER_SIZE];
static u8 midi_in_fifo_head = 0;
static u8 midi_in_fifo_tail = 0;

static inline bool midiInFifoIsEmpty()
{
	return midi_in_fifo_head == midi_in_fifo_tail;
}

static inline bool midiInFifoPeek(u8 *byte)
{
	if (midiInFifoIsEmpty())
		return false;

	*byte = midi_in_fifo[midi_in_fifo_head];
	return true;
}

static inline bool midiInFifoPop(u8 *byte)
{
	if (!midiInFifoPeek(byte))
		return false;

	midi_in_fifo_head = (midi_in_fifo_head + 1) % USBMIDI_IN_BUFFER_SIZE;
	return true;
}

static inline void midiInFifoPush(u8 byte)
{
	u8 nextTail = (midi_in_fifo_tail + 1) % USBMIDI_IN_BUFFER_SIZE;

	if (nextTail == midi_in_fifo_head)
		return;

	midi_in_fifo[midi_in_fifo_tail] = byte;
	midi_in_fifo_tail = nextTail;
}

static inline bool midiInFifoHasSpaceFor(u8 numBytes)
{
	return USBMIDI_IN_BUFFER_SIZE - ((USBMIDI_IN_BUFFER_SIZE + midi_in_fifo_tail - midi_in_fifo_head) % USBMIDI_IN_BUFFER_SIZE) >= numBytes;
}

static void pollUsb()
{
	midi_event_t midiEvent;
	int numReceived;

	while (numReceived = USB_Recv(USBMIDI_ENDPOINT_OUT, &midiEvent, sizeof(midiEvent)))
	{
		// MIDI USB messages are 4 bytes in size.
		if (numReceived != 4)
			return;

		// They get decoded to up to 3 MIDI Serial bytes.
		u8 data[3];

		unsigned count = UsbToMidi::process(midiEvent, data);

		if (midiInFifoHasSpaceFor(count))
		{
			for (unsigned i=0; i<count; ++i)
			{
				midiInFifoPush(data[i]);
			}
		}
	}
}

USBMIDI_::USBMIDI_()
{
	static uint8_t endpointType[2];

	endpointType[0] = EP_TYPE_BULK_OUT;
	endpointType[1] = EP_TYPE_BULK_IN;

	static PUSBCallbacks cb = {
		.setup = &USBMIDI_Setup,
		.getInterface = &USBMIDI_GetInterface,
		.getDescriptor = &USBMIDI_GetDescriptor,
		.numEndpoints = 2,
		.numInterfaces = 2,
		.endpointType = endpointType,
	};

	static PUSBListNode node(&cb);

	USBMIDI_ENDPOINT_OUT = PUSB_AddFunction(&node, &USBMIDI_INTERFACE);
	USBMIDI_ENDPOINT_IN = USBMIDI_ENDPOINT_OUT + 1;
}

int USBMIDI_::available()
{
	pollUsb();
	return !midiInFifoIsEmpty();
}

int USBMIDI_::read()
{
	pollUsb();

	u8 byte = 0;
	midiInFifoPop(&byte);

	return byte;
}

int USBMIDI_::peek()
{
	pollUsb();

	u8 byte = 0;
	midiInFifoPeek(&byte);

	return byte;
}

void USBMIDI_::flush()
{
	USB_Flush(USBMIDI_ENDPOINT_IN);
}

size_t USBMIDI_::write(uint8_t c)
{
	midi_event_t midiEvent;
	if (midiToUsb.process(c, midiEvent))
	{
		USB_Send(USBMIDI_ENDPOINT_IN, &midiEvent, sizeof(midiEvent));
	}

	return 1;
}

void USBMIDI_::poll()
{
	pollUsb();
}

#endif // USBCON
