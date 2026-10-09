#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/keeloq.h"

static uint32_t bytes_to_u32(const uint8_t data[4]) {
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static void u32_to_bytes(uint32_t value, uint8_t data[4]) {
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

int main(void) {
    const uint32_t input_a = 0xA1234567UL;
    const uint32_t input_2 = 0x21234567UL;
    const uint32_t input_8 = 0x81234567UL;
    uint8_t data[4];

    /*
     * Simple Learning regression vector.
     *
     * The 64-bit manufacturer code is compiled into keeloq.c through
     * config.h and is used directly as the KeeLoq key.
     */
    u32_to_bytes(input_a, data);
    keeloq_encrypt(data);

    assert(bytes_to_u32(data) == 0xD4034220UL);

    /*
     * Verify that different HCS300 button codes produce independent
     * encrypted values while preserving the same serial/discriminator
     * and counter fields.
     */
    {
        uint8_t data_2[4];
        uint8_t data_8[4];

        u32_to_bytes(input_2, data_2);
        u32_to_bytes(input_8, data_8);

        keeloq_encrypt(data_2);
        keeloq_encrypt(data_8);

        assert(bytes_to_u32(data_2) == 0x37166330UL);
        assert(bytes_to_u32(data_8) == 0x9F3F659AUL);

        assert(memcmp(data_2, data_8, sizeof(data_2)) != 0);
        assert(memcmp(data_2, data, sizeof(data_2)) != 0);
        assert(memcmp(data_8, data, sizeof(data_8)) != 0);
    }

    /*
     * Regression vector for the actual config.h values:
     *
     * serial       = 0x01234567
     * DISC         = 0x167
     * button       = 0x2
     * first counter after a clean EEPROM initialization = 0x0001
     * OVR          = 0
     * plaintext    = 0x21670001
     * manufacturer key = 0x0123456789ABCDEF
     */
    {
        uint8_t configured[4];
        u32_to_bytes(0x21670001UL, configured);
        keeloq_encrypt(configured);
        assert(bytes_to_u32(configured) == 0x08EE9FC6UL);
    }

    printf("KeeLoq tests passed\n");
    return 0;
}
