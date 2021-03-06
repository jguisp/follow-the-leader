# environment variables
#ARDUINO_ROOT=~/Learning/iot/arduino/Arduino.app/Contents/Java/hardware/tools/avr
# ARDUINO_ROOT=/Carlos/IOT/arduino-1.6.12/hardware/tools/avr
#DEVICE_PATH=/dev/cu.usbmodem1411
# DEVICE_PATH=/dev/ttyACM0

SKETCH_NAME=main

# you probably shouldn't change these variables
GCC=$(ARDUINO_ROOT)/bin/avr-gcc
OBJCOPY=$(ARDUINO_ROOT)/bin/avr-objcopy
AVRDUDE=$(ARDUINO_ROOT)/bin/avrdude
AVRDUDE_CONF=$(ARDUINO_ROOT)/etc/avrdude.conf

DEPS=main.h circular_buffer/circular_buffer.c uart/uart.c notes/notes.c
# DEPS=simple_usart/uart.o

HEADER_SEARCH_PATH=./
PRINTF_EXT=-Wl,-u,vfprintf -lprintf_flt -lm
SCANF_EXT=-Wl,-u,vfscanf -lscanf_flt -lm

.PHONY: compile upload clean

compile: $(SKETCH_NAME).hex

%.hex: %.x
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.x: $(DEPS) %.c
	$(GCC) -Wall -I $(HEADER_SEARCH_PATH) -Os -mmcu=atmega328 $(PRINTF_EXT)  -o $@ $^

upload: $(SKETCH_NAME).hex $(AVRDUDE_CONF) $(DEVICE_PATH)
	$(AVRDUDE) -C $(AVRDUDE_CONF) -v -patmega328p -carduino -P $(DEVICE_PATH) -b115200 -D -Uflash:w:$<:i

clean:
	$(RM) *.x *.hex
