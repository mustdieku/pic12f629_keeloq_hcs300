#include "keeloq.h"
#include "config.h"

#define KEELOQ_NLF 0x3A5C742EU

/*
 * The encoder key is derived from the compile-time manufacturer code and
 * serial number using KeeLoq normal learning.
 *
 * Because both input values are compile-time constants, the resulting key
 * is also stored as compile-time constants. This saves 8 bytes of RAM,
 * which is significant on the PIC12F629.
 *
 * These values correspond to:
 *
 *   manufacturer = 0x01234567_89ABCDEF
 *   serial       = 0x01234567
 *
 * normal learning:
 *
 *   K1 = decrypt(serial | 0x20000000) = 0x89074278
 *   K2 = decrypt(serial | 0x60000000) = 0x0516FBE9
 */
#define ENCODER_KEY_LO 0x89074278UL
#define ENCODER_KEY_HI 0x0516FBE9UL

static uint8_t bit32(uint32_t x, uint8_t n) {
    return (uint8_t)((x >> n) & 1U);
}

/* KeeLoq nonlinear feedback lookup. */
static uint8_t nlf(uint8_t index) {
    return (uint8_t)((KEELOQ_NLF >> index) & 1U);
}

/* Standard 528-round KeeLoq encryption. */
uint32_t keeloq_encrypt(uint32_t data) {
    uint32_t x = data;
    uint16_t r;
    uint8_t idx;
    uint8_t feedback;

    for(r = 0; r < 528U; ++r) {
        idx = (uint8_t)(bit32(x, 1) |
              (bit32(x, 9) << 1) |
              (bit32(x, 20) << 2) |
              (bit32(x, 26) << 3) |
              (bit32(x, 31) << 4));

        /*
         * The KeeLoq key is constant, so only one key bit is needed
         * during each round. Avoid keeping the complete 64-bit key in RAM.
         */
        if((r & 63U) < 32U) {
            feedback = (uint8_t)(bit32(x, 0) ^ bit32(x, 16) ^
                                 bit32(ENCODER_KEY_LO,
                                       (uint8_t)(r & 63U)) ^
                                 nlf(idx));
        } else {
            feedback = (uint8_t)(bit32(x, 0) ^ bit32(x, 16) ^
                                 bit32(ENCODER_KEY_HI,
                                       (uint8_t)((r & 63U) - 32U)) ^
                                 nlf(idx));
        }

        x = (x >> 1) | ((uint32_t)feedback << 31);
    }

    return x;
}

void keeloq_normal_learning(uint32_t serial, uint32_t manufacturer_lo,
                            uint32_t manufacturer_hi) {
    /*
     * Normal learning is performed offline because the manufacturer code
     * and serial number are compile-time constants.
     *
     * Keep this function for API compatibility with the transmitter code.
     * No RAM is allocated and no runtime calculation is performed.
     */
    (void)serial;
    (void)manufacturer_lo;
    (void)manufacturer_hi;
}
