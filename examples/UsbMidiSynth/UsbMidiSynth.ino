/*
 * Simple USBMIDI synth v0.2
 *
 * Plays MIDI notes using square waves on SNDOUT pin
 * Attach piezo or audio amplifier to that pin and send some notes to it
 * Can only play one note at time
 * Tested on Brmlab Leonardo
 *
 * (c) Tomas 'harvie' Mudrunka 2019
 */

#include <usbmidi.h>

#define SNDOUT 10 //Audio output pin

#define MIDI_NOTE_OFF   0b10000000
#define MIDI_NOTE_ON    0b10010000
#define MIDI_CONTROL    0b10110000
#define MIDI_PITCH_BEND 0b11100000

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
  //Handle USB communication
  USBMIDI.poll();

  // While there's MIDI USB input available...
  while (USBMIDI.available()) {

    //Parse MIDI
    u8 command=0, channel=0, key=0, pitchbend=0, pblo=0, pbhi=0, velocity=0;

    //Skip to beginning of next message (silently dropping stray data bytes)
    while(!(USBMIDI.peek() & 0b10000000)) USBMIDI.read();

    command = USBMIDI.read();
    channel = (command & 0b00001111)+1;
    command = command & 0b11110000;

    switch(command) {
      case MIDI_NOTE_ON:
      case MIDI_NOTE_OFF:
        if(USBMIDI.peek() & 0b10000000) continue; key      = USBMIDI.read();
        if(USBMIDI.peek() & 0b10000000) continue; velocity = USBMIDI.read();
        break;
      case MIDI_PITCH_BEND:
        if(USBMIDI.peek() & 0b10000000) continue; pblo = USBMIDI.read();
        if(USBMIDI.peek() & 0b10000000) continue; pbhi = USBMIDI.read();
        int pitchbend = (pblo << 7) | pbhi;
        //TODO: apply pitchbend to tone
        break;
    }

    //Play tones
    unsigned int pitch = note_to_freq(key);
    if(command == MIDI_NOTE_ON && velocity > 0 && pitch > 0) tone(SNDOUT, pitch);
    if(command == MIDI_NOTE_OFF || velocity == 0 || pitch == 0) noTone(SNDOUT);

    //Debug
    Serial.print(command);
    Serial.print(" C:");
    Serial.print(channel);
    Serial.print(" K:");
    Serial.print(key);
    Serial.print("\tV:");
    Serial.print(velocity);
    Serial.print("\tP:");
    Serial.print(pitch);
    Serial.print("\tPB:");
    Serial.print(pitchbend, BIN);
    Serial.print(" (");
    Serial.print(pblo, BIN);
    Serial.print(" ");
    Serial.print(pbhi, BIN);
    Serial.print(")\tF:");
    Serial.print(pitch);
    Serial.print('\n');
  }
}
