#ifndef EEPROM_STORE_H
#define EEPROM_STORE_H

#include <stdint.h>

void eeprom_store_init(uint16_t initial_counter);
uint16_t eeprom_store_get(void);
uint8_t eeprom_store_commit(uint16_t counter);

#endif
