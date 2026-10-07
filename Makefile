XC8 ?= xc8-cc
BUILD := build
SRC := src/main.c src/keeloq.c src/eeprom_store.c
CFLAGS := -mcpu=12F629 -O1 -I./src

all: $(BUILD)/hcs300_pic12f629.hex

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/hcs300_pic12f629.hex: $(SRC) src/config.h src/keeloq.h src/eeprom_store.h | $(BUILD)
	$(XC8) $(CFLAGS) $(SRC) -o $(BUILD)/hcs300_pic12f629.elf

clean:
	rm -rf $(BUILD)

.PHONY: all clean
