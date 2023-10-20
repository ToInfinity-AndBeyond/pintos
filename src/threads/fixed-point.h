// We are using 32-bit fixed point binary in 17.14 format
// f = 2^q = 2^14 = 16384
#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

#include <stdint.h>

#define FRACTIONAL_BITS 14
#define FIXED_MULTIPLIER (1 << FRACTIONAL_BITS)

typedef int32_t real;

inline real convert_to_real(int n)
{
  return n * FIXED_MULTIPLIER;
}

inline int convert_to_int_towards_zero(real r)
{
  return r / FIXED_MULTIPLIER;
}

inline int convert_to_int_round(real r)
{
  if (r > 0)
    return (r + FIXED_MULTIPLIER / 2) / FIXED_MULTIPLIER;
  else
    return (r - FIXED_MULTIPLIER / 2) / FIXED_MULTIPLIER;
}

inline real add_reals(real x, real y)
{
  return x + y;
}

// Performs x - y
inline real sub_reals(real x, real y)
{
  return x - y;
}

inline real add_real_and_int(real x, int n)
{
  return add_reals(x, convert_to_real(n));
}

// Performs x - n
inline real sub_real_and_int(real x, int n)
{
  return sub_reals(x, convert_to_real(n));
}

inline real sub_int_and_real(int n, real x) {
  return sub_reals(convert_to_real(n), x);
}

inline real multiply_reals(real x, real y)
{
  return ((int64_t) x) * y / FIXED_MULTIPLIER;
}

inline real multiply_real_and_int(real x, int n)
{
  return x * n;
}

inline real divide_reals(real x, real y)
{
  return ((int64_t) x) * FIXED_MULTIPLIER / y;
}

inline real divide_real_and_int(real x, int n)
{
  return x / n;
}

#endif
