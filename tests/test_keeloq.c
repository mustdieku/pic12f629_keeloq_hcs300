#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/keeloq.h"

int main(void) {
    const uint32_t key_lo = 0x89ABCDEFUL;
    const uint32_t key_hi = 0x01234567UL;
    const uint32_t input = 0xA1234567UL;
    uint32_t encrypted;

    keeloq_set_key(key_lo, key_hi);
    encrypted = keeloq_encrypt(input);
    assert(encrypted == 0xD4034220UL);

    assert(keeloq_decrypt(encrypted) == input);

    printf("KeeLoq test passed: %08lX -> %08lX\n",
           (unsigned long)input, (unsigned long)encrypted);
    return 0;
}
