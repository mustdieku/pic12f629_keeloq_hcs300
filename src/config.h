#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* Device identity. Replace these constants with the values used by the receiver. */
#define MANUFACTURER_CODE_LO UINT32_C(0x89ABCDEF)
#define MANUFACTURER_CODE_HI UINT32_C(0x01234567)
#define SERIAL_NUMBER     UINT32_C(0x01234567) /* only bits 0..27 are transmitted */
#define INITIAL_COUNTER   UINT16_C(0x0000)

/* HCS300-compatible PWM timing. */
#define _XTAL_FREQ 4000000UL
#define TE_US 400U
#define DEBOUNCE_MS 10U
#define INTER_WORD_GUARD_MS 16U

/* Button mapping. GP1=button 1, GP0=button 2. */
#define BUTTON1_CODE 0x2U
#define BUTTON2_CODE 0x8U

/* HCS300 status bits used by this implementation. */
#define VLOW_BIT 0U
#define OVR_BITS 0U

#endif
