# USBMIDI

USB MIDI library for Arduino.

## Quick start

1. Download / clone the repository into your arduino/libraries folder.
2. Add `#include <usbmidi.h>` at the top of your sketch
3. Add `USBMIDI.poll();` into your `loop() { ... }`
4. Use `USBMIDI` object in the same way as `Serial` (except no need for `Serial.begin(31250)` call) for writing and reading MIDI data.
5. Make sure to read any Input from USBMIDI, even if you are only using USBMIDI for Output. See [midictrl.ino](https://github.com/BlokasLabs/usbmidi/blob/master/examples/midictrl/midictrl.ino#L54) for an example.

## Examples

### midictrl

![midictrl Wiring Diagram](https://github.com/BlokasLabs/usbmidi/blob/master/examples/midictrl/midictrl.png)

This example shows how to create a 4 potentiometer and 2 push button MIDI controller.

[Read more...](https://github.com/BlokasLabs/usbmidi/tree/master/examples/midictrl)

### UsbMidiLoopback

This example echoes incoming USB MIDI data back to output, as well as prints it to the Serial Monitor. It demonstrates all that's necessary to initialize, receive and send USB MIDI data.

[See the code here...](https://github.com/BlokasLabs/usbmidi/blob/master/examples/UsbMidiLoopback/UsbMidiLoopback.ino)

## Design Considerations of USBMIDI and Difference from Arduino's MIDIUSB

Arduino has its own similar library called MIDIUSB, it was being developed relatively at the same time as USBMIDI.
The biggest difference is that the MIDIUSB library introduced new APIs with difficult interfaces while USBMIDI library exposes
a familiar interface, similar to the Serial objects, by inheriting and implementing the Stream base class. Therefore, as many
existing projects and examples for MIDI are using Serial to send and receive MIDI data through the DIN-5 ports using TX/RX UART pins,
the exact same code can be adapted simply by using USBMIDI global object instead of Serial, or generalized by refactoring the code generating
and interpreting messages to use Stream pointers or references, and passing in USBMIDI as a reference, that makes it straightforward to extend
existing projects with MIDI over USB support.

## Supported Devices

The library has two implementations, one is using Pluggable USB built-in Arduino library, for microcontrollers with native USB support (for example ATmega32U4),
and another using V-USB library which implements USB functionality in software, so microcontrollers such as ATmega328P can be used.

### Boards with Predefined Config

You may contribute support for your board by adding in the definitions to [usbboard.h](https://github.com/BlokasLabs/USBMIDI/blob/master/src/usbboard.h) and opening a pull request.

1. [Digispark](http://digistump.com/products/1)

### Custom Boards

To use the software USB implementation, the library must be reconfigured according to your wirings. The best way to do that is to create a custom [Arduino Board specification](https://github.com/arduino/Arduino/wiki/Arduino-IDE-1.5-3rd-party-Hardware-specification) for your project, and provide the following defines:

| Define name                 | Default Value | Optional? |
| --------------------------- | ------------- | --------- |
| -DUSB_CFG_IOPORTNAME        | D             | N         |
| -DUSB_CFG_DMINUS_BIT        | 7             | N         |
| -DUSB_CFG_DPLUS_BIT         | 2             | N         |
| -DUSB_CFG_PULLUP_IOPORTNAME | Undefined     | Y         |
| -DUSB_CFG_PULLUP_BIT        | Undefined     | Y         |

By default V-USB assumes pins with INT0 external interrupt function are used. If USB data lines are connected to other pins, additional variables must be defined. See [Optional MCU Description](https://github.com/BlokasLabs/USBMIDI/blob/master/src/usbconfig.h#L388) in usbconfig.h.

Alternatively you may modify [src/usbconfig.h](src/usbconfig.h) by hand to match your board.

## License

The exact [License](LICENSE) terms depend on which implementation gets used in your project Pluggable USB based implementations use BSD License, V-USB implementation follows V-USB open source license terms,
alternatively commercial license for V-USB is available. See [LICENSE](LICENSE) file for details.
