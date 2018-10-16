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

// This USBMIDI implementation is based on V-USB-MIDI project: http://cryptomys.de/horo/V-USB-MIDI/index.html

#include <avr/io.h>
#include <avr/pgmspace.h>

#ifndef USBCON

#include "usbdrv.h"

#include "fifo.h"
#include "midi_serialization.h"
#include "usbmidi.h"

// USB device descriptor
static const PROGMEM char deviceDescrMIDI[] =
{
	18,                     // sizeof(usbDescriptorDevice): length of descriptor in bytes
	USBDESCR_DEVICE,        // descriptor type
	0x10, 0x01,             // USB version supported
	0,                      // device class: defined at interface level
	0,                      // subclass
	0,                      // protocol
	8,                      // max packet size
	USB_CFG_VENDOR_ID,      // 2 bytes
	USB_CFG_DEVICE_ID,      // 2 bytes
	USB_CFG_DEVICE_VERSION, // 2 bytes
	1,                      // manufacturer string index
	2,                      // product string index
	0,                      // serial number string index
	1,                      // number of configurations
};

// B.2 Configuration Descriptor
// USB configuration descriptor
static const PROGMEM char configDescrMIDI[] =
{
	9,                      // sizeof(usbDescrConfig): length of descriptor in bytes
	USBDESCR_CONFIG,        // descriptor type
	101, 0,                 // total length of data returned (including inlined descriptors)
	2,                      // number of interfaces in this configuration
	1,                      // index of this configuration
	0,                      // configuration name string index
#if USB_CFG_IS_SELF_POWERED
	USBATTR_SELFPOWER,      // attributes
#else
	USBATTR_BUSPOWER,       // attributes
#endif
	USB_CFG_MAX_BUS_POWER / 2,  // max USB current in 2mA units

	// B.3 AudioControl Interface Descriptors
	// The AudioControl interface describes the device structure (audio function topology) 
	// and is used to manipulate the Audio Controls. This device has no audio function 
	// incorporated. However, the AudioControl interface is mandatory and therefore both 
	// the standard AC interface descriptor and the classspecific AC interface descriptor 
	// must be present. The class-specific AC interface descriptor only contains the header 
	// descriptor.

	// B.3.1 Standard AC Interface Descriptor
	// The AudioControl interface has no dedicated endpoints associated with it. It uses the 
	// default pipe (endpoint 0) for all communication purposes. Class-specific AudioControl 
	// Requests are sent using the default pipe. There is no Status Interrupt endpoint provided.

	// AC interface descriptor follows inline:
	9,                      // sizeof(usbDescrInterface): length of descriptor in bytes
	USBDESCR_INTERFACE,     // descriptor type
	0,                      // index of this interface
	0,                      // alternate setting for this interface
	0,                      // endpoints excl 0: number of endpoint descriptors to follow
	1,                      //
	1,                      //
	0,                      //
	0,                      // string index for interface

	// B.3.2 Class-specific AC Interface Descriptor
	// The Class-specific AC interface descriptor is always headed by a Header descriptor 
	// that contains general information about the AudioControl interface. It contains all 
	// the pointers needed to describe the Audio Interface Collection, associated with the 
	// described audio function. Only the Header descriptor is present in this device 
	// because it does not contain any audio functionality as such.
	// AC Class-Specific descriptor
	9,                      // sizeof(usbDescrCDC_HeaderFn): length of descriptor in bytes
	36,                     // descriptor type
	1,                      // header functional descriptor
	0x0, 0x01,              // bcdADC
	9, 0,                   // wTotalLength
	1,                      //
	1,                      //

	// B.4 MIDIStreaming Interface Descriptors
	// B.4.1 Standard MS Interface Descriptor
	// interface descriptor follows inline:
	9,                      // length of descriptor in bytes
	USBDESCR_INTERFACE,     // descriptor type
	1,                      // index of this interface
	0,                      // alternate setting for this interface
	2,                      // endpoints excl 0: number of endpoint descriptors to follow
	1,                      // AUDIO
	3,                      // MS
	0,                      // unused
	0,                      // string index for interface

	// B.4.2 Class-specific MS Interface Descriptor
	// MS Class-Specific descriptor
	7,                      // length of descriptor in bytes
	36,                     // descriptor type
	1,                      // header functional descriptor
	0x0, 0x01,              // bcdADC
	65, 0,                  // wTotalLength

	// B.4.3 MIDI IN Jack Descriptor
	6,                      // bLength
	36,                     // descriptor type
	2,                      // MIDI_IN_JACK desc subtype
	1,                      // EMBEDDED bJackType
	1,                      // bJackID
	0,                      // iJack

	6,                      // bLength
	36,                     // descriptor type
	2,                      // MIDI_IN_JACK desc subtype
	2,                      // EXTERNAL bJackType
	2,                      // bJackID
	0,                      // iJack

	//B.4.4 MIDI OUT Jack Descriptor
	9,                      // length of descriptor in bytes
	36,                     // descriptor type
	3,                      // MIDI_OUT_JACK descriptor
	1,                      // EMBEDDED bJackType
	3,                      // bJackID
	1,                      // No of input pins
	2,                      // BaSourceID
	1,                      // BaSourcePin
	0,                      // iJack

	9,                      // bLength of descriptor in bytes
	36,                     // bDescriptorType
	3,                      // MIDI_OUT_JACK bDescriptorSubtype
	2,                      // EXTERNAL bJackType
	4,                      // bJackID
	1,                      // bNrInputPins
	1,                      // baSourceID (0)
	1,                      // baSourcePin (0)
	0,                      // iJack

	// B.5 Bulk OUT Endpoint Descriptors
	//B.5.1 Standard Bulk OUT Endpoint Descriptor
	9,                      // bLength
	USBDESCR_ENDPOINT,      // bDescriptorType = endpoint
	0x1,                    // bEndpointAddress OUT endpoint number 1
	3,                      // bmAttributes: 2:Bulk, 3:Interrupt endpoint
	8, 0,                   // wMaxPacketSize
	10,                     // bInterval in ms
	0,                      // bRefresh
	0,                      // bSyncAddress

	// B.5.2 Class-specific MS Bulk OUT Endpoint Descriptor
	5,                      // bLength of descriptor in bytes
	37,                     // bDescriptorType
	1,                      // bDescriptorSubtype
	1,                      // bNumEmbMIDIJack 
	1,                      // baAssocJackID (0)

	//B.6 Bulk IN Endpoint Descriptors
	//B.6.1 Standard Bulk IN Endpoint Descriptor
	9,                      // bLength
	USBDESCR_ENDPOINT,      // bDescriptorType = endpoint
	0x81,                   // bEndpointAddress IN endpoint number 1
	3,                      // bmAttributes: 2: Bulk, 3: Interrupt endpoint
	8, 0,                   // wMaxPacketSize
	10,                     // bIntervall in ms
	0,                      // bRefresh
	0,                      // bSyncAddress

	// B.6.2 Class-specific MS Bulk IN Endpoint Descriptor
	5,                      // bLength of descriptor in bytes
	37,                     // bDescriptorType
	1,                      // bDescriptorSubtype
	1,                      // bNumEmbMIDIJack (0)
	3,                      // baAssocJackID (0)
};

static TFifo<uint8_t, uint8_t, 64> g_midiInput;
static TFifo<uint8_t, uint8_t, 64> g_midiOutput;
static MidiToUsb g_serializer(0);

__attribute__((weak)) USBMIDI_DEFINE_VENDOR_NAME(USB_CFG_VENDOR_NAME);
__attribute__((weak)) USBMIDI_DEFINE_PRODUCT_NAME(USB_CFG_DEVICE_NAME);

usbMsgLen_t usbFunctionDescriptor(usbRequest_t * rq)
{
	if (rq->wValue.bytes[1] == USBDESCR_DEVICE)
	{
		usbMsgPtr = (usbMsgPtr_t)deviceDescrMIDI;
		return sizeof(deviceDescrMIDI);
	}
	else if (rq->wValue.bytes[1] == USBDESCR_CONFIG)
	{
		usbMsgPtr = (usbMsgPtr_t)configDescrMIDI;
		return sizeof(configDescrMIDI);
	}
	else if (rq->wValue.bytes[1] == USBDESCR_STRING)
	{
		if (rq->wValue.bytes[0] == 1)
		{
			const uint8_t *data;
			usbMsgLen_t n = _usbmidi_get_vendor_string(data);
			usbMsgPtr = (usbMsgPtr_t)data;
			return n;
		}
		else if (rq->wValue.bytes[0] == 2)
		{
			const uint8_t *data;
			usbMsgLen_t n = _usbmidi_get_product_string(data);
			usbMsgPtr = (usbMsgPtr_t)data;
			return n;
		}
	}

	return 0;
}

uint8_t usbFunctionSetup(uint8_t data[8])
{
	return 0;
}

// Called when receiving MIDI message from PC.
void usbFunctionWriteOut(uint8_t * data, uint8_t len)
{
	uint8_t m[3];
	for (uint8_t i=0; i<len; i+=4)
	{
		midi_event_t event;
		event.m_event   = data[i+0];
		event.m_data[0] = data[i+1];
		event.m_data[1] = data[i+2];
		event.m_data[2] = data[i+3];
		uint8_t n = UsbToMidi::process(event, m);
		for (uint8_t j=0; j<n; ++j)
		{
			g_midiInput.push(m[j]);
		}
	}
}

static void midiUsbInit(void)
{
	// Activate pull-ups except on USB lines.
	USB_CFG_IOPORT = (uint8_t)~((1 << USB_CFG_DMINUS_BIT) | (1 << USB_CFG_DPLUS_BIT));

	// All pins input except USB (-> USB reset).
#ifdef USB_CFG_PULLUP_IOPORT // Use usbDeviceConnect()/usbDeviceDisconnect() if available.
	USBDDR = 0;              // We do RESET by deactivating pullup.
	usbDeviceDisconnect();
#else
	USBDDR = (1 << USB_CFG_DMINUS_BIT) | (1 << USB_CFG_DPLUS_BIT);
#endif

	// USB Reset by device only required on Watchdog Reset.
	uint8_t j = 0;
	while (--j) {   /* USB Reset by device only required on Watchdog Reset */
		uint8_t i = 0;
		while (--i);  /* delay >10ms for USB reset */
	}

#ifdef USB_CFG_PULLUP_IOPORT
	usbDeviceConnect();
#else
	USBDDR = 0; // Remove USB reset condition.
#endif

	usbInit();
}

USBMIDI_ USBMIDI;

USBMIDI_::USBMIDI_()
{
	midiUsbInit();
}

int USBMIDI_::available()
{
	return g_midiInput.size();
}

int USBMIDI_::read()
{
	uint8_t byte;
	if (g_midiInput.pop(byte))
	{
		return byte;
	}
	else return -1;
}

int USBMIDI_::peek()
{
	uint8_t byte;
	if (g_midiInput.peek(byte))
	{
		return byte;
	}
	else return -1;
}

void USBMIDI_::flush()
{
	while (!g_midiOutput.empty())
	{
		poll();
	}
}

size_t USBMIDI_::write(uint8_t c)
{
	if (g_midiOutput.full())
		flush();

	g_midiOutput.push(c);
	return sizeof(c);
}

static uint8_t fillBuffer(uint8_t buffer[8])
{
	midi_event_t ev;
	uint8_t byte;
	uint8_t n=0;
	for (uint8_t i=0; !g_midiOutput.empty() && i<2; ++i)
	{
		while (g_midiOutput.pop(byte))
		{
			if (g_serializer.process(byte, ev))
			{
				memcpy(&buffer[i * sizeof(ev)], &ev, sizeof(ev));
				++n;
				break;
			}
		}
	}

	return n * sizeof(ev);
}

void USBMIDI_::poll()
{
	if (!g_midiOutput.empty() && usbInterruptIsReady())
	{
		uint8_t buffer[8];
		uint8_t n = fillBuffer(buffer);

		if (n)
			usbSetInterrupt(buffer, n);
	}
	usbPoll();
}

#endif // USBCON
