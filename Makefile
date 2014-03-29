TARGET=orestes

MCU=atmega32u4

F_CPU=16000000

build:
	avr-gcc -std=gnu99 -Os -D F_CPU=$(F_CPU)UL -mmcu=$(MCU) -c -o $(TARGET).o $(TARGET).c
	avr-gcc -mmcu=$(MCU) $(TARGET).o -o $(TARGET)
	avr-size $(TARGET)
	avr-objcopy -O ihex -R .eeprom $(TARGET) $(TARGET).hex

local:
	gcc -std=gnu99 -c $(TARGET).c -o $(TARGET).o
	gcc $(TARGET).o -o $(TARGET)

simulate: build
	simulavr -d $(MCU) $(TARGET).hex

upload: build
	teensy_loader_cli -mmcu=$(MCU) $(TARGET).hex

test: local
	tests/run ./$(TARGET)

clean:
	rm -f $(TARGET).o $(TARGET).hex $(TARGET)
