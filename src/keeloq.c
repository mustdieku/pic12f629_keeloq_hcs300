#include "keeloq.h"
#include "config.h"

#define KEELOQ_NLF 0x3A5C742EUL

/*
 * Simple learning uses the 64-bit manufacturer code directly as the
 * KeeLoq encryption key.
 */
static uint8_t nlf(uint8_t index)
{
    return (uint8_t)((KEELOQ_NLF >> index) & 1U);
}

static uint8_t key_bit(uint8_t bit)
{
    if(bit < 32U)
        return (uint8_t)((MANUFACTURER_CODE_LO >> bit) & 1U);

    return (uint8_t)((MANUFACTURER_CODE_HI >> (bit - 32U)) & 1U);
}

/*
 * Standard 528-round KeeLoq encryption, in place.
 *
 * data[0] contains bits 0..7 and data[3] contains bits 24..31.
 *
 * In-place operation is important for the PIC12F629: the previous
 * uint32_t return value required additional compiler-generated RAM
 * temporaries during the call.
 */
void keeloq_encrypt(uint8_t data[4])
{
    uint16_t round;
    uint8_t index;
    uint8_t feedback;

    for(round = 0; round < 528U; ++round) {
        index = (uint8_t)(
            ((data[0] >> 1) & 1U) |
            (((data[1] >> 1) & 1U) << 1) |
            (((data[2] >> 4) & 1U) << 2) |
            (((data[3] >> 2) & 1U) << 3) |
            (((data[3] >> 7) & 1U) << 4));

        feedback = (uint8_t)(
            (data[0] & 1U) ^
            (data[2] & 1U) ^
            key_bit((uint8_t)(round & 63U)) ^
            nlf(index));

        data[0] = (uint8_t)((data[0] >> 1) | (data[1] << 7));
        data[1] = (uint8_t)((data[1] >> 1) | (data[2] << 7));
        data[2] = (uint8_t)((data[2] >> 1) | (data[3] << 7));
        data[3] = (uint8_t)((data[3] >> 1) | (feedback << 7));
    }
}
