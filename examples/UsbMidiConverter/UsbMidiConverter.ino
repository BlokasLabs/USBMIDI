/*
 * Simple USBMIDI converter v0.1
 *
 * Forwards MIDI messages from USB to MIDI and back
 * If you short TX and RX pins of arduino, it should act as MIDI loopback
 *
 * Note that Serial1 is used by Arduino Leonardo
 * Serial port on your board might have different name (eg.: Serial, Serial2, ...)
 *
 * (c) Tomas 'harvie' Mudrunka 2019
 */

#include <usbmidi.h>

void setup() {
  Serial1.begin(31250); //MIDI baudrate
}

void loop() {
  //Handle USB communication
  USBMIDI.poll();

  //Forward MIDI
  while(USBMIDI.available()) Serial1.write(USBMIDI.read());
  Serial1.flush();
  while(Serial1.available()) USBMIDI.write(Serial1.read());
  USBMIDI.flush();
}
