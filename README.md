# usbmidi
USB MIDI support for Arduino

# Install instructions

* Download / clone the repository
* Copy or symbolically link (ln on unices, mklink on Windows) the 'usbmidi' folder in your Arduino 'libraries' folder (usually in your sketchbook location, see Preferences, ctrl+comma)

# Examples

## midictrl

![midictrl Wiring Diagram](https://github.com/BlokasLabs/usbmidi/blob/master/arduino/libraries/usbmidi/examples/midictrl/midictrl.png)

This example shows how to create a 4 potentiometer and 2 push button MIDI controller.

[Read more...](https://github.com/BlokasLabs/usbmidi/tree/master/arduino/libraries/usbmidi/examples/midictrl)

## UsbMidiLoopback

This example echoes incoming USB MIDI data back to output, as well as prints it to the Serial Monitor. It demonstrates all that's necessary to initialize, receive and send USB MIDI data.

[See the code here...]( https://github.com/BlokasLabs/usbmidi/blob/master/arduino/libraries/usbmidi/examples/UsbMidiLoopback/UsbMidiLoopback.ino)

# Design Considerations of usbmidi and difference from Arduino's MIDIUSB

Arduino has its own similar library called MIDIUSB, it was being developed relatively at the same time as usbmidi. The biggest difference is that the MIDIUSB library introduced new APIs with difficult interfaces while usbmidi library exposes a familiar interface, similar to the Serial objects, by inheriting and implementing the Stream base class. Therefore, as many existing projects and examples for MIDI are using Serial to send and receive MIDI data through the DIN-5 ports using TX/RX UART pins, the exact same code can be adapted simply by using USBMIDI global object instead of Serial, or generalized by refactoring the code generating and interpreting messages to use Stream pointers or references, and passing in USBMIDI as a reference, that makes it straightforward to extend existing projects with MIDI over USB support.

# Supported Devices

The lirary is using Pluggable USB built-in Arduino libraries, so any processor supporting it should be usable with this library.

Let us know by creating an Issue if you can't get it to work on any platform supporting Pluggable USB!
