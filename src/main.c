#include <xc.h>
#include <stdint.h>
#include "config.h"
#include "keeloq.h"
#include "eeprom_store.h"

/* PIC12F629 configuration: internal oscillator, watchdog off, code protection off. */
#pragma config FOSC = INTRCIO
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config MCLRE = OFF
#pragma config BOREN = ON
#pragma config CP = OFF
#pragma config CPD = OFF

#define BUTTON1_MASK (1U << 1) /* GP1 / pin 6 */
#define BUTTON2_MASK (1U << 0) /* GP0 / pin 7 */
#define DATA_MASK    (1U << 4) /* GP4 / pin 3 */
#define LED_MASK     (1U << 5) /* GP5 / pin 2 */


static void delay_te(uint8_t n) {
    while(n--) __delay_us(TE_US);
}

static void data_bit(uint8_t bit) {
    /* HCS300 PWM: 0 = TE high + 2TE low; 1 = 2TE high + TE low. */
    GPIO |= DATA_MASK;
    delay_te(bit ? 2U : 1U);
    GPIO &= (uint8_t)~DATA_MASK;
    delay_te(bit ? 1U : 2U);
}

static void send_word(uint8_t button_code, uint16_t counter, uint8_t repeat) {
    uint32_t hop;
    uint8_t i;
    uint8_t fixed_byte;

    /* Standard HCS300 normal-learning encrypted input. */
    hop = keeloq_encrypt(
        ((uint32_t)(button_code & 0x0FU) << 28) |
        ((uint32_t)(OVR_BITS & 0x03U) << 26) |
        ((SERIAL_NUMBER & 0x03FFUL) << 16) |
        counter);

    /* 23 TE 50% duty-cycle preamble. */
    for(i = 0; i < 23U; ++i) {
        GPIO |= DATA_MASK;
        __delay_us(TE_US);
        GPIO &= (uint8_t)~DATA_MASK;
        __delay_us(TE_US);
    }

    /* Standard 10 TE header: low level following the preamble. */
    GPIO &= (uint8_t)~DATA_MASK;
    delay_te(10U);

    /* Encrypted portion: 32 bits, LSB first. */
    for(i = 0; i < 32U; ++i) data_bit((uint8_t)((hop >> i) & 1UL));

    /*
     * Fixed portion, 34 bits. HCS300 transmits the serial number MSB first,
     * followed by S3,S0,S1,S2,VLOW,RPT. This is the ordering shown in DS21137F.
     */
    for(i = 0; i < 28U; ++i) data_bit((uint8_t)((SERIAL_NUMBER >> (27U - i)) & 1UL));

    fixed_byte = (uint8_t)((button_code >> 3) & 1U); /* S3 */
    data_bit(fixed_byte);
    fixed_byte = (uint8_t)(button_code & 1U);       /* S0 */
    data_bit(fixed_byte);
    fixed_byte = (uint8_t)((button_code >> 1) & 1U); /* S1 */
    data_bit(fixed_byte);
    fixed_byte = (uint8_t)((button_code >> 2) & 1U); /* S2 */
    data_bit(fixed_byte);
    data_bit(VLOW_BIT);
    data_bit(repeat ? 1U : 0U); /* HCS300 RPT status: 0 for first word, 1 for repeats. */

    /* Guard time before another code word. */
    GPIO &= (uint8_t)~DATA_MASK;
    delay_te(39U);
}

static uint8_t read_buttons(void) {
    uint8_t v = 0;
    if(GPIO & BUTTON1_MASK) v |= BUTTON1_CODE;
    if(GPIO & BUTTON2_MASK) v |= BUTTON2_CODE;
    return v;
}

static uint8_t debounce_buttons(uint8_t candidate) {
    uint8_t stable = candidate;
    uint8_t i;
    for(i = 0; i < DEBOUNCE_MS; ++i) {
        __delay_ms(1);
        if(read_buttons() != candidate) return 0xFFU;
    }
    return stable;
}

static void io_init(void) {
    GPIO = 0;
    TRISIO = (uint8_t)(BUTTON1_MASK | BUTTON2_MASK); /* GP0/GP1 inputs, rest outputs. */
    CMCON = 0x07;                                    /* Comparator off. */
    WPU = 0x00;                                      /* External pull resistors expected. */
    OPTION_REGbits.nGPPU = 1;                        /* Disable weak pull-ups. */
}

void main(void) {
    uint8_t state = 0;
    uint8_t candidate;
    uint8_t debounced;
    uint8_t button_code;
    uint8_t repeat;
    uint16_t counter;

    /* Derive the encoder key once from the manufacturer key. */
    keeloq_normal_learning(SERIAL_NUMBER, MANUFACTURER_CODE_LO,
                           MANUFACTURER_CODE_HI);

    io_init();
    eeprom_store_init(INITIAL_COUNTER);
    counter = eeprom_store_get();
    repeat = 0;

    for(;;) {
        candidate = read_buttons();
        if(candidate == state) {
            if(state != 0) {
                GPIO |= LED_MASK;
                send_word(state, counter, 1U);
                repeat = 1U;
                /* LED remains on between words while a button is held. */
            } else {
                GPIO &= (uint8_t)~LED_MASK;
                __delay_ms(5);
            }
            continue;
        }

        debounced = debounce_buttons(candidate);
        if(debounced == 0xFFU) continue;
        candidate = debounced;
        if(candidate == state) continue;

        if(candidate == 0) {
            state = 0;
            GPIO &= (uint8_t)~LED_MASK;
            continue;
        }

        /* A new pressed state consumes exactly one counter value. */
        counter = (uint16_t)(counter + 1U);
        if(!eeprom_store_commit(counter)) {
            /* Do not transmit if the new synchronization value was not committed. */
            GPIO &= (uint8_t)~LED_MASK;
            state = 0;
            continue;
        }
        state = candidate;
        button_code = state;
        GPIO |= LED_MASK;
        send_word(button_code, counter, 0U);
        repeat = 1U;
    }
}
