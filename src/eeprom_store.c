#include "eeprom_store.h"
#include <xc.h>
#include "config.h"

#define EEPROM_SLOTS 16U
#define RECORD_SIZE  5U
#define EEPROM_MAGIC 0xA5U

/* Runtime state is kept in static storage to minimize stack usage. */
static uint8_t current_seq;
static uint16_t current_counter;

static uint8_t ee_read(uint8_t address) {
    EEADR = address;
    EECON1bits.RD = 1;
    return EEDATA;
}

static void ee_write(uint8_t address, uint8_t value) {
    uint8_t gie = INTCONbits.GIE;

    while(EECON1bits.WR) { }

    EEADR = address;
    EEDATA = value;
    EECON1bits.WREN = 1;

    INTCONbits.GIE = 0;
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    EECON1bits.WREN = 0;

    INTCONbits.GIE = gie;
    while(EECON1bits.WR) { }
}

static uint8_t checksum(uint8_t seq, uint16_t counter) {
    uint8_t x = (uint8_t)(0x5AU ^ seq ^ (uint8_t)counter ^
                          (uint8_t)(counter >> 8));
    return (uint8_t)((x << 3) | (x >> 5));
}

/*
 * Validate one EEPROM record.
 *
 * MAGIC is written last. Therefore an interrupted record cannot become
 * a valid record.
 */
static uint8_t slot_valid(uint8_t slot) {
    uint8_t base = (uint8_t)(slot * RECORD_SIZE);
    uint8_t seq;
    uint8_t lo;
    uint8_t hi;
    uint8_t chk;

    if(ee_read(base) != EEPROM_MAGIC) return 0;

    seq = ee_read((uint8_t)(base + 1U));

    lo = ee_read((uint8_t)(base + 2U));
    hi = ee_read((uint8_t)(base + 3U));

    chk = ee_read((uint8_t)(base + 4U));

    if(checksum(seq, (uint16_t)lo | ((uint16_t)hi << 8)) != chk) return 0;

    return 1;
}

/*
 * Compare 8-bit sequence numbers with wrap-around.
 *
 * This remains valid as long as the distance between two valid sequence
 * numbers is less than 128.
 */
static uint8_t seq_newer(uint8_t a, uint8_t b) {
    return (uint8_t)((int8_t)(a - b) > 0);
}

void eeprom_store_init(uint16_t initial_counter) {
    uint8_t i;
    uint8_t seq;
    uint8_t best_seq = 0;
    uint8_t best_slot = 0;
    uint8_t found = 0;
    uint8_t lo;
    uint8_t hi;

    for(i = 0; i < EEPROM_SLOTS; ++i) {
        seq = ee_read((uint8_t)(i * RECORD_SIZE + 1U));
        if(slot_valid(i) && (!found || seq_newer(seq, best_seq))) {
            found = 1;
            best_seq = seq;
            best_slot = i;
        }
    }

    if(found) {
        current_seq = best_seq;
        lo = ee_read((uint8_t)(best_slot * RECORD_SIZE + 2U));
        hi = ee_read((uint8_t)(best_slot * RECORD_SIZE + 3U));
        current_counter = (uint16_t)lo | ((uint16_t)hi << 8);
    } else {
        current_seq = 0;
        current_counter = initial_counter;
        eeprom_store_commit(current_counter);
    }
}

uint16_t eeprom_store_get(void) {
    return current_counter;
}

uint8_t eeprom_store_commit(uint16_t counter) {
    uint8_t next_slot = (uint8_t)((current_seq + 1U) & 0x0FU);
    uint8_t next_seq = (uint8_t)(current_seq + 1U);
    uint8_t base = (uint8_t)(next_slot * RECORD_SIZE);
    uint8_t chk = checksum(next_seq, counter);

    /*
     * Invalidate the slot first.
     *
     * MAGIC is restored only after the complete record has been written.
     */
    ee_write(base, 0x00U);
    ee_write((uint8_t)(base + 1U), next_seq);
    ee_write((uint8_t)(base + 2U), (uint8_t)counter);
    ee_write((uint8_t)(base + 3U), (uint8_t)(counter >> 8));
    ee_write((uint8_t)(base + 4U), chk);

    ee_write(base, EEPROM_MAGIC);

    if(!slot_valid(next_slot))
        return 0;

    current_seq = next_seq;
    current_counter = counter;
    return 1;
}
