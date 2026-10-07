#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/*
 * Device identity.
 *
 * These values are fixed for the transmitter and must match the receiver
 * configuration / learned encoder key.
 */
#define MANUFACTURER_CODE_LO UINT32_C(0x89ABCDEF)
#define MANUFACTURER_CODE_HI UINT32_C(0x01234567)
#define SERIAL_NUMBER       UINT32_C(0x01234567)
#define INITIAL_COUNTER   UINT16_C(0x0000)

/* HCS300 nominal timing. */
#define _XTAL_FREQ 4000000UL
#define TE_US 400U
#define DEBOUNCE_MS 10U

/*
 * HCS300 button codes.
 *
 * GP1 -> 0x2
 * GP0 -> 0x8
 * both -> 0xA
 */
#define BUTTON1_CODE 0x2U
#define BUTTON2_CODE 0x8U

/*
 * DISC is normally derived from the ten least significant serial-number
 * bits for HCS300-compatible transmitters.
 */
#define DISC_VALUE (SERIAL_NUMBER & UINT32_C(0x03FF))

/* HCS300 status bits. */
#define VLOW_BIT 0U
#define OVR_BITS 0U

#endif
