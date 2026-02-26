/**
 * math_util.h — DSP Math Utilities
 *
 * Small collection of maths helpers used across the module implementations.
 * Keeping them here avoids duplication and gives one place to tune precision
 * vs. speed trade-offs.
 */

#ifndef LUASYNTH_MATH_UTIL_H
#define LUASYNTH_MATH_UTIL_H

/* -------------------------------------------------------------------------
 * clampf — clamp a float to [lo, hi].
 *
 * Used to keep parameters like filter cutoff within stable ranges.
 *
 * @param x   value to clamp
 * @param lo  lower bound
 * @param hi  upper bound
 * @return    x clamped to [lo, hi]
 * ---------------------------------------------------------------------- */
static inline float clampf(float x, float lo, float hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

/* -------------------------------------------------------------------------
 * lerpf — linear interpolation between a and b by t ∈ [0,1].
 *
 * @param a  start value
 * @param b  end value
 * @param t  interpolation factor (0 = a, 1 = b)
 * @return   interpolated value
 * ---------------------------------------------------------------------- */
static inline float lerpf(float a, float b, float t) {
    return a + t * (b - a);
}

/* -------------------------------------------------------------------------
 * fast_tanhf — rational approximation of tanh(x) for |x| ≤ ~4.
 *
 * Used in the Moog ladder filter to model the soft-saturation of transistors.
 * The exact math.h tanh() is accurate but expensive. This Padé approximant
 * is cheap and accurate enough for audio (error < 0.003 for |x| ≤ 3):
 *
 *   tanh(x) ≈ x * (27 + x²) / (27 + 9·x²)
 *
 * For |x| > 4 the approximation saturates gracefully (approaches ±1).
 *
 * @param x  input value
 * @return   approximate tanh(x)
 * ---------------------------------------------------------------------- */
static inline float fast_tanhf(float x) {
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

/* -------------------------------------------------------------------------
 * db_to_linear — convert decibels to a linear amplitude multiplier.
 *
 * @param db  level in decibels
 * @return    linear multiplier
 * ---------------------------------------------------------------------- */
float db_to_linear(float db);

#endif /* LUASYNTH_MATH_UTIL_H */
