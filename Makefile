
# The name of your project (used to name the compiled .hex file)
TARGET = orestes

# configurable options
OPTIONS = -DF_CPU=48000000 -DUSB_HID -DLAYOUT_US_ENGLISH

# options needed by many Arduino libraries to configure for Teensy 3.0
OPTIONS += -D__MK20DX128__ -DARDUIO=104

#************************************************************************
# Settings below this point usually do not need to be edited
#************************************************************************

# CPPFLAGS = compiler options for C and C++
CPPFLAGS = -Wall -g -Os -mcpu=cortex-m4 -mthumb -nostdlib -MMD $(OPTIONS) -I.

# compiler options for C++ only
CXXFLAGS = -std=gnu++0x -felide-constructors -fno-exceptions -fno-rtti

# compiler options for C only
CFLAGS = -std=gnu99

# linker options
LDFLAGS = -Os -Wl,--gc-sections -mcpu=cortex-m4 -mthumb -Tmk20dx128.ld

# additional libraries to link
LIBS = -lm

# I'm not sure of the right place to get these; I had to extract it from
# Teensyduino: http://www.pjrc.com/teensy/td_download.html
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

C_FILES := $(wildcard *.c) $(wildcard teensy3/*.c)
CPP_FILES := $(wildcard *.cpp) $(wildcard teensy3/*.cpp)
OBJS := $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

all: build

build: $(TARGET).hex

SRC ?= orestes.fs
inline.cfs:
	ruby -ne 'n ||= 0; puts "run(\"#{$$_.chomp}\");" unless $$_.chomp.empty?' orestes.fs > inlined.cfs


upload: build
	teensy_loader_cli -mmcu=mk20dx128 -w $(TARGET).hex

# $(CC) $(LDFLAGS) -o $@ $(OBJS)
$(TARGET).elf: $(OBJS) teensy3/mk20dx128.ld
	mkdir -p build
	$(CC) -Os -Wl,--gc-sections -mcpu=cortex-m4 -mthumb \
	  -Tteensy3/mk20dx128.ld -o $@ $(OBJS)

%.hex: %.elf
	$(SIZE) $<
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# compiler generated dependency info
-include $(OBJS:.o=.d)

orestes.c: inline.cfs

clean:
	rm -f *.o *.d teensy3/*.o teensy3/*.d $(TARGET).elf $(TARGET).hex local/$(TARGET) inlined.c

local: local/$(TARGET).o
	mkdir -p local
	gcc -std=gnu99 -c $(TARGET).c -o local/$(TARGET).o
	gcc local/$(TARGET).o -o $(TARGET)

test: local
	tests/run ./$(TARGET)
