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

