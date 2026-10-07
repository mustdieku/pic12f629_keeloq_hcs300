#include "keeloq.h"

#define KEELOQ_NLF 0x3A5C742EUL

/* Normal-learning key for the configured manufacturer/serial pair. */
#define KEY_LO 0x89074278UL
#define KEY_HI 0x0516FBE9UL

/*
 * KeeLoq uses:
 *
 *   g5(x) = x[1] + 2*x[9] + 4*x[20] + 8*x[26] + 16*x[31]
 *
 * The previous implementation used variable 32-bit shifts for every bit.
 * On the PIC12F629 this generates relatively large helper code.
 *
 * Keeping the state as four bytes makes the individual bits directly
 * addressable and avoids variable-width 32-bit shifts.
 */
static uint8_t nlf(uint8_t index)
{
    return (uint8_t)((KEELOQ_NLF >> index) & 1U);
}

static uint8_t key_bit(uint8_t bit)
{
    if(bit < 32U)
        return (uint8_t)((KEY_LO >> bit) & 1U);

    return (uint8_t)((KEY_HI >> (bit - 32U)) & 1U);
}

static uint8_t g5(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
    /*
     * State byte layout:
     *
     *   b0 = bits  0..7
     *   b1 = bits  8..15
     *   b2 = bits 16..23
     *   b3 = bits 24..31
     */
    return (uint8_t)(
        ((b0 >> 1) & 1U) |
        (((b1 >> 1) & 1U) << 1) |
        (((b2 >> 4) & 1U) << 2) |
        (((b3 >> 2) & 1U) << 3) |
        (((b3 >> 7) & 1U) << 4));
}

/*
 * Standard 528-round KeeLoq encryption.
 *
 * This version deliberately uses four bytes instead of uint32_t shifts.
 * It is considerably cheaper for the PIC12F629 instruction set.
 *
 * The byte order is little-endian:
 *
 *   b0 = bits  0..7
 *   b1 = bits  8..15
 *   b2 = bits 16..23
 *   b3 = bits 24..31
 */
uint32_t keeloq_encrypt(uint32_t data)
{
    uint8_t b0 = (uint8_t)data;
    uint8_t b1 = (uint8_t)(data >> 8);
    uint8_t b2 = (uint8_t)(data >> 16);
    uint8_t b3 = (uint8_t)(data >> 24);
    uint16_t round;
    uint8_t index;
    uint8_t feedback;

    for(round = 0; round < 528U; ++round) {
        index = g5(b0, b1, b2, b3);

        feedback =
            (uint8_t)(
                (b0 & 1U) ^
                ((b2 & 1U) << 0) ^
                key_bit((uint8_t)(round & 63U)) ^
                nlf(index));

        b0 = (uint8_t)((b0 >> 1) | (b1 << 7));
        b1 = (uint8_t)((b1 >> 1) | (b2 << 7));
        b2 = (uint8_t)((b2 >> 1) | (b3 << 7));
        b3 = (uint8_t)((b3 >> 1) | (feedback << 7));
    }

    return
        (uint32_t)b0 |
        ((uint32_t)b1 << 8) |
        ((uint32_t)b2 << 16) |
        ((uint32_t)b3 << 24);
}
