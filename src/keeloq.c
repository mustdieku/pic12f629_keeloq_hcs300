#include "keeloq.h"
#include "config.h"

#define KEELOQ_NLF 0x3A5C742EU

/*
 * Only the learned 64-bit key is kept in RAM. The manufacturer key is a
 * compile-time constant, so it does not need a second 64-bit RAM object.
 */
static uint32_t key_lo;
static uint32_t key_hi;

static uint8_t bit32(uint32_t x, uint8_t n) {
    return (uint8_t)((x >> n) & 1U);
}

/* KeeLoq NLF lookup. The argument is the five-bit input index. */
static uint8_t nlf(uint8_t index) {
    return (uint8_t)((KEELOQ_NLF >> index) & 1U);
}

static uint8_t key_bit(uint8_t n) {
    if(n < 32U) return bit32(key_lo, n);
    return bit32(key_hi, (uint8_t)(n - 32U));
}

void keeloq_set_key(uint32_t lo, uint32_t hi) {
    key_lo = lo;
    key_hi = hi;
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
        feedback = (uint8_t)(bit32(x, 0) ^ bit32(x, 16) ^
                             key_bit((uint8_t)(r & 63U)) ^ nlf(idx));
        x = (x >> 1) | ((uint32_t)feedback << 31);
    }

    return x;
}

uint32_t keeloq_decrypt(uint32_t data) {
    uint32_t x = data;
    uint16_t r;
    uint8_t idx;
    uint8_t feedback;

    for(r = 0; r < 528U; ++r) {
        idx = (uint8_t)(bit32(x, 0) |
              (bit32(x, 8) << 1) |
              (bit32(x, 19) << 2) |
              (bit32(x, 25) << 3) |
              (bit32(x, 30) << 4));
        feedback = (uint8_t)(bit32(x, 31) ^ bit32(x, 15) ^
                             key_bit((uint8_t)((15U - r) & 63U)) ^ nlf(idx));
        x = (x << 1) | feedback;
    }

    return x;
}

/*
 * Decrypt one learning block with the compile-time manufacturer key.
 *
 * This is deliberately separate from keeloq_decrypt(): passing a second
 * 64-bit key through the PIC12F629 call stack costs more RAM than the device
 * can spare. The manufacturer key is already available as constants.
 */
static uint32_t decrypt_manufacturer(uint32_t data) {
    uint32_t x = data;
    uint16_t r;
    uint8_t idx;
    uint8_t feedback;

    for(r = 0; r < 528U; ++r) {
        idx = (uint8_t)(bit32(x, 0) |
              (bit32(x, 8) << 1) |
              (bit32(x, 19) << 2) |
              (bit32(x, 25) << 3) |
              (bit32(x, 30) << 4));

        {
            uint8_t k = (uint8_t)((15U - r) & 63U);
            uint8_t kb;

            if(k < 32U)
                kb = (uint8_t)((MANUFACTURER_CODE_LO >> k) & 1U);
            else
                kb = (uint8_t)((MANUFACTURER_CODE_HI >> (k - 32U)) & 1U);

            feedback = (uint8_t)(bit32(x, 31) ^ bit32(x, 15) ^
                                 kb ^ nlf(idx));
        }

        x = (x << 1) | feedback;
    }

    return x;
}

/*
 * KeeLoq normal-learning derivation used by the HCS300 family.
 + * Only the final learned key is retained in RAM.
*/
void keeloq_normal_learning(uint32_t serial, uint32_t manufacturer_lo,
                            uint32_t manufacturer_hi) {
    uint32_t s = serial & 0x0FFFFFFFUL;

    /* Arguments are retained for API compatibility; the source constants
     * are used directly so no second 64-bit key is allocated in RAM. */
    (void)manufacturer_lo;
    (void)manufacturer_hi;

    key_lo = decrypt_manufacturer(s | 0x20000000UL);
    key_hi = decrypt_manufacturer(s | 0x60000000UL);
}
