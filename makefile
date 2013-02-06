BASENAME=$(shell basename `pwd`)
FIRMWARE=$(BASENAME).hex
BINS=$(FIRMWARE)

MCU=atmega168
FREQUENCY=8000000
BAUDRATE=19200
PORT=/dev/ttyUSB0

CC=avr-gcc
FLAGS=-mmcu=$(MCU) -morder2
CFLAGS=-Os -g -Wall --std=c99 -fgnu89-inline -fpack-struct \
	   -DF_CPU=$(FREQUENCY)UL -DBAUD=$(BAUDRATE)UL
LDFLAGS=-Wl,-u,vfprintf
LIBS=-lprintf_min

AVROBJCOPY=avr-objcopy
AVROBJDUMP=avr-objdump
AVRSIZE=avr-size
AVRDUDE=avrdude
AVRDUDE_OPTS=-p $(MCU) -c arduino -b $(BAUDRATE) -P $(PORT)

all: $(BINS)

sixbright.o: config.h

%.o: %.c
	$(CC) $(FLAGS) $(CFLAGS) -c $^

%.elf: $(patsubst %.c,%.o,$(wildcard *.c))
	$(CC) $(FLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(AVROBJDUMP) -h -S $@ > $*.lst
	$(AVRSIZE) $@

%.hex: %.elf
	$(AVROBJCOPY) -j .text -j .data -O ihex $^ $@

.PHONY: load chipid fuse clean tags

load: $(FIRMWARE)
	$(AVRDUDE) $(AVRDUDE_OPTS) -U flash:w:$^

clean:
	-rm -f $(BINS) *.cod *.lst *.o

tags:
	ctags -R .
