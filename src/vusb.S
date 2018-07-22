#include <avr/io.h>

#undef __SFR_OFFSET // Workaround for 'warning: "__SFR_OFFSET" redefined'

#ifndef USBCON
#	include "../vusb/usbdrv/usbdrvasm.S"
#endif
