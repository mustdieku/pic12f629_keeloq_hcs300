#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/keeloq.h"

int main(void) {
    const uint32_t key_lo = 0x89ABCDEFUL;
    const uint32_t key_hi = 0x01234567UL;
    const uint32_t input_a = 0xA1234567UL;
    const uint32_t input_2 = 0x21234567UL;
    const uint32_t input_8 = 0x81234567UL;
    uint32_t encrypted;

    keeloq_set_key(key_lo, key_hi);

    /*
     * Known regression vector for the current KeeLoq implementation.
     */
    encrypted = keeloq_encrypt(input_a);
    assert(encrypted == 0xD4034220UL);
    assert(keeloq_decrypt(encrypted) == input_a);

    /*
     * Verify that different HCS300 button codes produce independent
     * encrypted values while preserving the same serial/discriminator
     * and counter fields.
     */
    {
        uint32_t encrypted_2 = keeloq_encrypt(input_2);
        uint32_t encrypted_8 = keeloq_encrypt(input_8);

        assert(encrypted_2 != encrypted_8);
        assert(encrypted_2 != encrypted);
        assert(encrypted_8 != encrypted);

        assert(keeloq_decrypt(encrypted_2) == input_2);
        assert(keeloq_decrypt(encrypted_8) == input_8);
    }

    printf("KeeLoq tests passed\n");
    return 0;
}
