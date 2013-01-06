SixBright
=========

SixBright is replacement firmware for the
[HexBright](http://www.hexbright.com/) flashlight.

SixBright's design goals are:

  - Feature parity with HexBright factory firmware.
  - Pure AVR C using avr-gcc and avr-libc (no Arduino dependencies).
  - Higher quality firmware with clear code, precision PWM control, using a
    watchdog, power management, and modern software engineering techniques.
  - Simple serial communication for monitoring programs (ex: temperature and
    charge status).
  - Cross-platform host support with first-class Linux support.


Requirnments
------------

 - avr-gcc
 - avr-libc
 - avrdude
 - GNU make

These can be installed on an Ubunutu Linux system with:

$ sudo apt-get install build-essential gcc-avr avr-libc avrdude


Building
--------

To build the firmware, run make:

$ make


Programming
-----------

"sixbright.hex" is the firmware file to load onto your HexBright. Since the
HexBright uses an Arduino compatible bootloader, any Ardunio programming method
should work. The included makefile has "load" rule to do this with avrdude:

$ make load

This rule expects the HexBright to be on /dev/ttyUSB0 by default. This can be
changed with the PORT make variable, for example:

$ make PORT=/dev/ttyS0 load

Note: On Ubuntu, and many other Linux distributions, regular users must be a
part of the "dialout" group in order to write to a serial port. Add your user
to this group or run "make load" as root.


Serial Protocol
---------------

SixBright will send periodic updates via serial when USB is attached. This
section describes the format of these messages.

Messages are plain ASCII text. There is one message per line. The End-of-Line
is marked by CRLF (\r\n).

### Temperture ###

Format:  'T' \[SPACE\] \{temperature\}

Example: "T 55"

"temperature" is a positive integer from 0 to 255. It is the raw conversion
of an 8-bit ADC. The MCP9700's outputs a voltage of:

    Vout = 10mV * C + 500mV

The ADC conversion is:

    ADC = Vout * 256 / 3.3V

The resulting formula for converting a raw ADC value to Celcius is:

    C = ((ADC * 3.3 / 256) - 0.5) / 0.01

Note: the MCP9700 is a cheap sensor usable over the range of 0C to 70C +- 4C.
Do not expect precision temperature reports.

### Battery Status ###

Format: 'FULL' or 'CHARGE'

SixBright will report FULL when the HexBright is charged. If it is charging,
then it will report CHARGE.

