// We are using 32-bit fixed point binary in 17.14 format
// f = 2^q = 2^14 = 16384

#include <stdint.h>

#define FRACTIONAL_BITS 14
#define FIXED_MULTIPLIER (1 << FRACTIONAL_BITS)

typedef int32_t real;
