#include "keeloq.h"

#define KEELOQ_NLF 0x3A5C742EUL

/*
 * Precomputed encoder key.
 *
 * Manufacturer:
 *     0x01234567_89ABCDEF
 *
 * Serial:
 *     0x01234567
 *
 * KeeLoq normal learning:
 *     K1 = decrypt(serial | 0x20000000) = 0x89074278
 *     K2 = decrypt(serial | 0x60000000) = 0x0516FBE9
 */
#define KEY_LO 0x89074278UL
#define KEY_HI 0x0516FBE9UL

/*
 * Return one bit from a 32-bit value.
 *
 * Keeping this as a small function allows XC8 to reuse the implementation
 * instead of duplicating the shift/mask sequence throughout the encoder.
 */
static uint8_t get_bit(uint32_t value, uint8_t bit)
{
    return (uint8_t)((value >> bit) & 1U);
}

/*
 * KeeLoq nonlinear feedback function.
 */
static uint8_t nlf(uint8_t index)
{
    return (uint8_t)((KEELOQ_NLF >> index) & 1U);
}

/*
 * Return one bit of the 64-bit encoder key.
 *
 * The key itself is stored in program constants, not RAM.
 */
static uint8_t key_bit(uint8_t bit)
{
    if(bit < 32U)
        return get_bit(KEY_LO, bit);

    return get_bit(KEY_HI, (uint8_t)(bit - 32U));
}

/*
 * Standard 528-round KeeLoq encryption.
 *
 * The input/output format is the standard 32-bit KeeLoq block.
 */
uint32_t keeloq_encrypt(uint32_t data)
{
    uint32_t x = data;
    uint16_t round;
    uint8_t index;
    uint8_t feedback;

    for(round = 0; round < 528U; ++round) {
        index =
            get_bit(x, 1U) |
            (uint8_t)(get_bit(x, 9U)  << 1) |
            (uint8_t)(get_bit(x, 20U) << 2) |
            (uint8_t)(get_bit(x, 26U) << 3) |
            (uint8_t)(get_bit(x, 31U) << 4);

        feedback =
            get_bit(x, 0U) ^
            get_bit(x, 16U) ^
            key_bit((uint8_t)(round & 63U)) ^
            nlf(index);

        x = (x >> 1) | ((uint32_t)feedback << 31);
    }

    return x;
}