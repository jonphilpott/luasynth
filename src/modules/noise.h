/**
 * noise.h — White Noise Source Module
 *
 * Generates white noise: random samples drawn uniformly from [-1, 1].
 * White noise contains equal power at all frequencies — useful for
 * percussion synthesis, wind effects, and as a filter input.
 *
 * Implementation:
 *   Uses the C standard rand() seeded from SDL_GetTicks(). For serious work
 *   you'd use a faster PRNG (e.g. xorshift32), but rand() is clear enough
 *   for this educational project.
 *
 * Example (Lua):
 *   local n = Noise.new()
 *   n:setAmplitude(0.3)
 *   local flt = Filter.new("svf")
 *   flt:setInput(n)
 *   Output.set(flt)
 */

#ifndef LUASYNTH_NOISE_H
#define LUASYNTH_NOISE_H

#include "../module.h"

/* -------------------------------------------------------------------------
 * NoiseData — private data for the noise module.
 * ---------------------------------------------------------------------- */
typedef struct {
    float amplitude;  /* output gain, 0.0 – 1.0 */
} NoiseData;

/* -------------------------------------------------------------------------
 * noise_create — allocate a new white noise module.
 *
 * @return  pointer to a new Module, or NULL on OOM
 * ---------------------------------------------------------------------- */
Module *noise_create(void);

/* -------------------------------------------------------------------------
 * noise_set_amplitude — update the output gain.
 *
 * @param mod        the noise module
 * @param amplitude  linear gain [0.0, 1.0]
 * ---------------------------------------------------------------------- */
void noise_set_amplitude(Module *mod, float amplitude);

#endif /* LUASYNTH_NOISE_H */
