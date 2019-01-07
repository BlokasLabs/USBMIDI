/*
 * Simple USBMIDI converter v0.1
 *
 * Forwards MIDI messages from USB to MIDI and back
 * If you short TX and RX pins of arduino, it should act as MIDI loopback
 *
 * (c) Tomas 'harvie' Mudrunka 2019
 */

#include <usbmidi.h>

void setup() {
  Serial1.begin(31250); //MIDI baudrate
}

void loop() {
  //Forward MIDI
  while(USBMIDI.available()) Serial1.write(USBMIDI.read());
  USBMIDI.flush();
  while(Serial1.available()) USBMIDI.write(Serial1.read());
  Serial1.flush();
}
