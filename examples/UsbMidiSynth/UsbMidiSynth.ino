/*
 * Simple USBMIDI synth v0.1
 *
 * Plays MIDI notes using square waves on SNDOUT pin
 * Attach piezo or audio amplifier to that pin and send some notes to it
 * Can only play one note at time
 * Tested on Brmlab Leonardo
 *
 * (c) Tomas 'harvie' Mudrunka 2019
 */

#include <usbmidi.h>

#define SNDOUT 1 //Audio output pin

#define MIDI_NOTE_OFF 128
#define MIDI_NOTE_ON  144
#define MIDI_CONTROL  176
#define MIDI_PITCH_BEND 224

//Convert MIDI note to frequency
double base_a4=440; //set A4=440Hz
double note_to_freq(double n) {
  if( n>=0 && n<=119 ) {
    return base_a4*pow(2,(n-57)/12);
  } else {
    return -1;
  }
}

void setup() {
  Serial.begin(9600);
}

void loop() {
  // While there's MIDI USB input available...
  while (USBMIDI.available()) {

    //Parse MIDI
    u8 command=0, channel=0, pitch=0;
    command = USBMIDI.read();
    pitch   = USBMIDI.read();
    channel = USBMIDI.read();

    //Play tones
    if(command == MIDI_NOTE_ON && pitch > 0) tone(SNDOUT, note_to_freq(pitch));
    if(command == MIDI_NOTE_OFF || pitch == 0) noTone(SNDOUT);

    //Debug
    Serial.print(command);
    Serial.print(' ');
    Serial.print(pitch);
    Serial.print(' ');
    Serial.print(channel);
    Serial.print(' ');
    Serial.print(note_to_freq(pitch));
    Serial.print('\n');
  }
}
