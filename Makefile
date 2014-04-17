TARGET=orestes

MCU=atmega32u4

F_CPU=16000000

build: inline
	avr-gcc -std=gnu99 -Os -D F_CPU=$(F_CPU)UL -mmcu=$(MCU) -c -o $(TARGET).o $(TARGET).c
	avr-gcc -std=gnu99 -Os -D F_CPU=$(F_CPU)UL -mmcu=$(MCU) -c -o usb_keyboard.o usb_keyboard.c
	avr-gcc -std=gnu99 -Os -D F_CPU=$(F_CPU)UL -mmcu=$(MCU) -c -o orestes_board.o orestes_board.c
	avr-gcc -mmcu=$(MCU) usb_keyboard.o $(TARGET).o orestes_board.o -o $(TARGET)
	avr-size $(TARGET)
	avr-objcopy -O ihex -R .eeprom $(TARGET) $(TARGET).hex

SRC ?= orestes.fs
inline:
	ruby -ne 'puts "run(\"#{$$_.chomp}\");" unless $$_.chomp.empty?' $(SRC) > inlined.c

local:
	gcc -std=gnu99 -c $(TARGET).c -o $(TARGET).o
	gcc $(TARGET).o -o $(TARGET)

simulate: local
	cat $(TARGET).fs | ./$(TARGET)

upload: build
	teensy_loader_cli -w -mmcu=$(MCU) $(TARGET).hex

test: local
	tests/run ./$(TARGET)

repl: local
	./$(TARGET)

clean:
	rm -f $(TARGET).o $(TARGET).hex $(TARGET)
