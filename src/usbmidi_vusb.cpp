#include <arduino.h>

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


uint8_t usbFunctionDescriptor(usbRequest_t * rq)
{
	if (rq->wValue.bytes[1] == USBDESCR_DEVICE)
	{
		usbMsgPtr = (uint8_t*)deviceDescrMIDI;
		return sizeof(deviceDescrMIDI);
	}
	else // Must be config descriptor.
	{
		usbMsgPtr = (uint8_t*)configDescrMIDI;
		return sizeof(configDescrMIDI);
	}
}

static uint8_t sendEmptyFrame;

uint8_t usbFunctionSetup(uint8_t data[8])
{
	usbRequest_t *rq = (void *)data;

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS)
	{
		// Prepare bulk-in endpoint to respond to early termination.
		if ((rq->bmRequestType & USBRQ_DIR_MASK) == USBRQ_DIR_HOST_TO_DEVICE)
			sendEmptyFrame = 1;
	}

	return 0xff;
}

// Called when PC asks to fill MIDI messages sent to PC.
uint8_t usbFunctionRead(uint8_t * data, uint8_t len)
{
	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;
	data[4] = 0;
	data[5] = 0;
	data[6] = 0;

	return 7;
}

uint8_t usbFunctionWrite(uint8_t * data, uint8_t len)
{
	return 1;
}

// Called when receiving MIDI message from PC.
void usbFunctionWriteOut(uint8_t * data, u8 len)
{
#if 0
	uint8_t m[3];
	for (int i = 0; i<len; i += 4)
	{
		midi_event_t event;
		event.m_event = data[i + 0];
		event.m_data[0] = data[i + 1];
		event.m_data[1] = data[i + 2];
		event.m_data[2] = data[i + 3];
		if (UsbToMidi::process(event, m))
		{
			Serial.write(m[0]);
			Serial.write(m[1]);
			Serial.write(m[2]);
		}
	}
#endif

	if (len >= 4)
	{
		//msg[1] = data[1];
		//msg[2] = data[2];
		//msg[3] = data[3];
		//msg[0] = data[0];
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
	//delay(10); // delay 10ms for USB reset.

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

#if 0
int main(void)
{
  int adcOld[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
  uchar key, lastKey = 0;
  uchar keyDidChange = 0;
  uchar midiMsg[8];
  uchar channel = 0;
  int value;
  uchar iii;

  wdt_enable(WDTO_1S);
  hardwareInit();
  odDebugInit();
  usbInit();

  sendEmptyFrame = 0;

  sei();
  
  // only ADC channel 6 and channel 7 are used
  channel = 6;
  for (;;) {    // main event loop
	wdt_reset();
	usbPoll();

	key = keyPressed();
	if (lastKey != key)
	  keyDidChange = 1;

	if (usbInterruptIsReady()) {
	  if (keyDidChange) {
		// DEBUG LED
		PORTC ^= 0x40;
		// use last key and not current key status in order to avoid lost
		   changes in key status.
		// up to two midi events in one midi msg.
		// For description of USB MIDI msg see:
		// http://www.usb.org/developers/devclass_docs/midi10.pdf
		// 4. USB MIDI Event Packets
		iii = 0;
		if (lastKey) {  // release
		  midiMsg[iii++] = 0x08;
		  midiMsg[iii++] = 0x80;
		  midiMsg[iii++] = lastKey;
		  midiMsg[iii++] = 0x00;
		}
		if (key) {  // press
		  midiMsg[iii++] = 0x09;
		  midiMsg[iii++] = 0x90;
		  midiMsg[iii++] = key;
		  midiMsg[iii++] = 0x7f;
		}
		if (8 == iii)
		  sendEmptyFrame = 1;
		else
		  sendEmptyFrame = 0;
		usbSetInterrupt(midiMsg, iii);
		keyDidChange = 0;
		lastKey = key;
	  } else {  // if no key event check analog input
		value = adc(channel); // 0..1023
		// hysteresis
		if (adcOld[channel] - value > 7
			|| adcOld[channel] - value < -7) {
		  // DEBUG LED
		  PORTC ^= 0x80;
		  adcOld[channel] = value;
		  // MIDI CC msg
		  midiMsg[0] = 0x0b;
		  midiMsg[1] = 0xb0;
		  midiMsg[2] = channel + 70;  // cc 70..77 
		  midiMsg[3] = value >> 3;
		  sendEmptyFrame = 0;
		  usbSetInterrupt(midiMsg, 4);
		}
		channel++;
		channel &= 0x07;
		if (0 == channel)
		  channel = 6;
	  }
	}   // usbInterruptIsReady()
  }
  return 0;
}
#endif

USBMIDI_ USBMIDI;

USBMIDI_::USBMIDI_()
{
	midiUsbInit();
}

int USBMIDI_::available()
{
	return 0;
}

int USBMIDI_::read()
{
	return 0;
}

int USBMIDI_::peek()
{
	return 0;
}

void USBMIDI_::flush()
{
}

size_t USBMIDI_::write(uint8_t c)
{
	return 0;
}

void USBMIDI_::poll()
{
	usbPoll();
}

#endif // USBCON
