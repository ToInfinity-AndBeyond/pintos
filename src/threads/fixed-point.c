#include "fixed-point.h"

real convert_to_real(int n) {
  return n * FIXED_MULTIPLIER;
}

int convert_to_int(real r) {
  if (r > 0) {
    return (r + FIXED_MULTIPLIER / 2) / FIXED_MULTIPLIER;
  } else {
    return (r - FIXED_MULTIPLIER / 2) / FIXED_MULTIPLIER;
  }
}

real add_reals(real x, real y) {
  return x + y;
}

// Performs x - y
real sub_reals(real x, real y) {
  return x - y;
}

real add_real_and_int(real x, int n) {
  return add_reals(x, convert_to_real(n));
}

// Performs x - n
real sub_real_and_int(real x, int n) {
  return sub_reals(x, convert_to_real(n));
}

real multiply_reals(real x, real y) {
  return ((int64_t) x) * y / FIXED_MULTIPLIER;
}

real multiply_real_and_int(real x, int n) {
  return x * n;
}

real divide_reals(real x, real y) {
  return ((int64_t) x) * FIXED_MULTIPLIER / y;
}

real divide_real_and_int(real x, int n) {
  return x / n;
}
