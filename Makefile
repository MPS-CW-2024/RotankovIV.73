MCU = atmega8515
F_CPU = 3686400
TARGET = irrigation

CC = avr-gcc
OBJCOPY = avr-objcopy
SIZE = avr-size
OBJDIR = obj

CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL -Os -Wall
CFLAGS += -ffunction-sections -fdata-sections -std=gnu99
CFLAGS += -Isrc
LDFLAGS = -Wl,--gc-sections

SOURCES = src/main.c \
         src/zone.c \
         src/lcd.c \
         src/uart.c \
         src/dht.c \
		 src/zone_control.c \
		 src/time.c

OBJECTS = $(SOURCES:src/%.c=$(OBJDIR)/%.o)

all: directories $(TARGET).hex size

directories:
	mkdir -p $(OBJDIR)

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).elf: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

size: $(TARGET).elf
	$(SIZE) --format=avr --mcu=$(MCU) $<

clean:
	rm -rf $(OBJDIR)
	rm -f *.hex *.elf

.PHONY: all clean directories size