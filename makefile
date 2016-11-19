# environment variables
ARDUINO_ROOT=~/Learning/iot/arduino/Arduino.app/Contents/Java/hardware/tools/avr
DEVICE_PATH=/dev/cu.usbmodem1411

SKETCH_NAME=main

# you probably shouldn't change these variables
GCC=$(ARDUINO_ROOT)/bin/avr-gcc
OBJCOPY=$(ARDUINO_ROOT)/bin/avr-objcopy
AVRDUDE=$(ARDUINO_ROOT)/bin/avrdude
AVRDUDE_CONF=$(ARDUINO_ROOT)/etc/avrdude.conf

DEPS=circular_buffer.c uart.c

.PHONY: compile upload clean

compile: $(SKETCH_NAME).hex

%.hex: %.x
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.x: $(DEPS) %.c
	$(GCC) -Wall -Os -mmcu=atmega328 -o $@ $^

upload: $(SKETCH_NAME).hex $(AVRDUDE_CONF) $(DEVICE_PATH)
	$(AVRDUDE) -C $(AVRDUDE_CONF) -v -patmega328p -carduino -P $(DEVICE_PATH) -b115200 -D -Uflash:w:$<:i

clean:
	$(RM) *.x *.hex
