#ifndef KEELOQ_H
#define KEELOQ_H

#include <stdint.h>

/*
 * The PIC12F629 has very little RAM. Keep the 64-bit KeeLoq key in the
 * KeeLoq module instead of passing an 8-byte structure through the call stack.
 */
void keeloq_set_key(uint32_t lo, uint32_t hi);
uint32_t keeloq_encrypt(uint32_t data);
uint32_t keeloq_decrypt(uint32_t data);

/* Derive the encoder key using KeeLoq normal learning. */
void keeloq_normal_learning(uint32_t serial, uint32_t manufacturer_lo,
                            uint32_t manufacturer_hi);

#endif
