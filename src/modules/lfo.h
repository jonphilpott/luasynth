/**
 * lfo.h — Low-Frequency Oscillator Module
 *
 * An LFO (Low-Frequency Oscillator) is structurally identical to a regular
 * oscillator, but operates in the range 0.01–20 Hz. Its output is used not
 * as an audio signal but as a modulation source — e.g., to sweep a filter
 * cutoff or vibrato a pitch.
 *
 * The LFO's process() output is in [-1, 1] and can be scaled by a `depth`
 * value in the ModInput that connects it to a target parameter.
 *
 * Example (Lua):
 *   local lfo = LFO.new("sine", 0.5)   -- 0.5 Hz sine LFO
 *   local flt = Filter.new("svf")
 *   flt:setInput(osc)
 *   flt:setCutoff(1000)
 *   lfo:connectMod(flt, "cutoff", 800)  -- sweep cutoff ±800 Hz
 */

#ifndef LUASYNTH_LFO_H
#define LUASYNTH_LFO_H

#include "../module.h"
#include "osc.h"  /* reuse OscWaveform */

/* -------------------------------------------------------------------------
 * LFOData — private state for an LFO module.
 * Same as OscData but conceptually limited to low frequencies.
 * ---------------------------------------------------------------------- */
typedef struct {
    OscWaveform waveform;
    double      frequency;   /* Hz, 0.01 – 20 */
    double      depth;       /* output scale, 0.0 – 1.0 */
    double      phase;       /* current phase [0, 1) */
} LFOData;

/* -------------------------------------------------------------------------
 * lfo_create — allocate a new LFO module.
 *
 * @param waveform   the waveform shape
 * @param frequency  LFO rate in Hz [0.01, 20.0]
 * @return           pointer to a new Module, or NULL on OOM
 * ---------------------------------------------------------------------- */
Module *lfo_create(OscWaveform waveform, double frequency);

void lfo_set_frequency(Module *mod, double frequency);
void lfo_set_depth(Module *mod, double depth);
void lfo_set_waveform(Module *mod, OscWaveform waveform);

#endif /* LUASYNTH_LFO_H */
