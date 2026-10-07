#include <xc.h>
#include <stdint.h>
#include "config.h"
#include "keeloq.h"
#include "eeprom_store.h"

/*
 * PIC12F629 configuration:
 * - internal 4 MHz oscillator
 * - watchdog disabled
 * - power-up timer enabled
 * - MCLR disabled
 * - brown-out reset enabled
 * - code protection disabled
 */
#pragma config FOSC = INTRCIO
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config MCLRE = OFF
#pragma config BOREN = ON
#pragma config CP = OFF
#pragma config CPD = OFF

#define BUTTON1_MASK (1U << 1) /* GP1, pin 6 */
#define BUTTON2_MASK (1U << 0) /* GP0, pin 7 */
#define DATA_MASK    (1U << 4) /* GP4, pin 3 */
#define LED_MASK     (1U << 5) /* GP5, pin 2 */

/* Delay by an integer number of HCS300 time elements. */
static void delay_te(uint8_t n) {
    while(n--) __delay_us(TE_US);
}

static void data_bit(uint8_t bit) {
    GPIO |= DATA_MASK;
    delay_te(bit ? 2U : 1U);
    GPIO &= (uint8_t)~DATA_MASK;
    delay_te(bit ? 1U : 2U);
}

static void data_byte_lsb(uint8_t value) {
    uint8_t i;
    for(i = 0; i < 8U; ++i) {
        data_bit(value & 1U);
        value >>= 1;
    }
}

/*
 * Transmit one complete HCS300 code word.
 *
 * The counter is supplied by the caller and is never modified here.
 * Therefore repeated transmissions of a held button use exactly the
 * same rolling-code counter.
 */
static void send_word(uint8_t button_code, uint16_t counter, uint8_t repeat) {
    uint32_t hop;
    uint8_t i;

    /*
     * Encrypted HCS300 payload:
     *
     *   BUTTON[3:0] | OVR[1:0] | DISC[9:0] | COUNTER[15:0]
     */
    hop = keeloq_encrypt(
        ((uint32_t)(button_code & 0x0FU) << 28) |
        ((uint32_t)(OVR_BITS & 0x03U) << 26) |
        (DISC_VALUE << 16) |
        counter);

    /* HCS300 preamble: 23 TE of alternating high/low. */
    for(i = 0; i != 23U; ++i) {
        GPIO |= DATA_MASK;
        __delay_us(TE_US);
        GPIO &= (uint8_t)~DATA_MASK;
        __delay_us(TE_US);
    }

    /* Header: 10 TE low. */
    GPIO &= (uint8_t)~DATA_MASK;
    delay_te(10U);

    /*
     * Encrypted portion: 32 bits, LSB first.
     *
     * The cipher result is represented little-endian by keeloq_encrypt(),
     * so four byte transmissions are equivalent to the previous bit loop.
     */
    data_byte_lsb((uint8_t)hop);
    data_byte_lsb((uint8_t)(hop >> 8));
    data_byte_lsb((uint8_t)(hop >> 16));
    data_byte_lsb((uint8_t)(hop >> 24));

    /*
     * Fixed portion, 34 bits.
     *
     * HCS300 transmits:
     *
     *   SERIAL[27:0]
     *   S3 S0 S1 S2 VLOW RPT
     *
     * The serial number is transmitted MSB first.
     */
    data_bit((uint8_t)((SERIAL_NUMBER >> 27U) & 1U));
    data_bit((uint8_t)((SERIAL_NUMBER >> 26U) & 1U));
    data_bit((uint8_t)((SERIAL_NUMBER >> 25U) & 1U));
    data_bit((uint8_t)((SERIAL_NUMBER >> 24U) & 1U));

    data_byte_lsb((uint8_t)(SERIAL_NUMBER >> 16));
    data_byte_lsb((uint8_t)(SERIAL_NUMBER >> 8));
    data_byte_lsb((uint8_t)SERIAL_NUMBER);

    data_bit((uint8_t)((button_code >> 3) & 1U)); /* S3 */
    data_bit((uint8_t)(button_code & 1U));         /* S0 */
    data_bit((uint8_t)((button_code >> 1) & 1U)); /* S1 */
    data_bit((uint8_t)((button_code >> 2) & 1U)); /* S2 */
    data_bit(VLOW_BIT);
    data_bit(repeat ? 1U : 0U); /* RPT */

    /* Inter-word guard interval. */
    GPIO &= (uint8_t)~DATA_MASK;
    delay_te(39U);
}

/*
 * Convert the two physical button inputs into the HCS300 button code.
 *
 * The returned value is:
 *   0x0 - no button
 *   0x2 - button 1
 *   0x8 - button 2
 *   0xA - both buttons
 */
static uint8_t read_buttons(void) {
    uint8_t v = 0;

    if(GPIO & BUTTON1_MASK) v |= BUTTON1_CODE;
    if(GPIO & BUTTON2_MASK) v |= BUTTON2_CODE;

    return v;
}

/*
 * Simple debounce.
 *
 * The candidate state must remain unchanged for DEBOUNCE_MS consecutive
 * 1 ms samples. 0xFF is returned when the candidate changed during the
 * debounce interval.
 */
static uint8_t debounce_buttons(uint8_t candidate) {
    uint8_t i;

    for(i = 0; i < DEBOUNCE_MS; ++i) {
        __delay_ms(1);
        if(read_buttons() != candidate) return 0xFFU;
    }

    return candidate;
}

static void io_init(void) {
    GPIO = 0;
    TRISIO = (uint8_t)(BUTTON1_MASK | BUTTON2_MASK);

    /* Disable the comparator so GP0/GP1 are digital inputs. */
    CMCON = 0x07;

    /* Buttons use external pull resistors. */
    WPU = 0x00;
    OPTION_REGbits.nGPPU = 1;
}

void main(void) {
    static uint8_t state;
    static uint8_t candidate;
    static uint8_t debounced;
    static uint16_t counter;

    io_init();
    eeprom_store_init(INITIAL_COUNTER);
    counter = eeprom_store_get();

    for(;;) {
        candidate = read_buttons();

        if(candidate == state) {
            if(state != 0) {
                /*
                 * Button state has not changed.
                 *
                 * The same counter is deliberately reused. Only the RPT
                 * status bit changes for subsequent code words.
                 */
                GPIO |= LED_MASK;
                send_word(state, counter, 1U);
            } else {
                GPIO &= (uint8_t)~LED_MASK;
                __delay_ms(5);
            }

            continue;
        }

        /* Ignore short button transitions caused by mechanical bounce. */
        debounced = debounce_buttons(candidate);
        if(debounced == 0xFFU) continue;

        candidate = debounced;
        if(candidate == state) continue;

        if(candidate == 0) {
            /*
             * Release does not consume a counter value.
             */
            state = 0;
            GPIO &= (uint8_t)~LED_MASK;
            continue;
        }

        /*
         * A transition to a new non-zero button state consumes exactly
         * one rolling-code counter value.
         *
         * This covers:
         *
         *   0 -> 2
         *   0 -> 8
         *   2 -> A
         *   8 -> A
         *   A -> 2
         *   A -> 8
         *
         * A release never increments the counter.
         */
        counter = (uint16_t)(counter + 1U);

        /*
         * Commit the new counter before transmitting it. If the EEPROM
         * write cannot be verified, do not transmit a code that may be
         * lost across a power failure.
         */
        if(!eeprom_store_commit(counter)) {
            GPIO &= (uint8_t)~LED_MASK;
            state = 0;
            continue;
        }

        state = candidate;

        GPIO |= LED_MASK;

        /* First code word after a state transition: RPT = 0. */
        send_word(state, counter, 0U);
    }
}
