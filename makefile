BASENAME=$(shell basename `pwd`)
FIRMWARE=$(BASENAME).hex
BINS=$(FIRMWARE)

MCU=atmega168
FREQUENCY=8000000
BAUDRATE=19200
PORT=/dev/ttyUSB0

AVRCC=avr-gcc
AVRCC_FLAGS=-Os -g -Wall -mmcu=$(MCU) --std=c99 -morder2 -fgnu89-inline \
            -fpack-struct -DF_CPU=$(FREQUENCY)UL -DBAUD=$(BAUDRATE)UL
AVROBJCOPY=avr-objcopy
AVROBJDUMP=avr-objdump
AVRSIZE=avr-size
AVRDUDE=avrdude
AVRDUDE_OPTS=-p $(MCU) -c arduino -b $(BAUDRATE) -P $(PORT)

CC=gcc
CFLAGS=-Wall -O3 -g
LDFLAGS=-lm

all: $(BINS)

%.o: %.c
	$(AVRCC) $(AVRCC_FLAGS) -c $^

%.elf: $(patsubst %.c,%.o,$(wildcard *.c))
	$(AVRCC) $(AVRCC_FLAGS) -o $@ $^ -lm
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
