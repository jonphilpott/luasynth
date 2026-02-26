/**
 * filter.h — Filter Module (State-Variable Filter + Moog Ladder)
 *
 * Provides two classic synthesiser filter topologies:
 *
 * 1. SVF — Chamberlin State-Variable Filter
 *    A two-integrator loop that simultaneously produces lowpass, bandpass,
 *    and highpass outputs. Output mode is selected by FilterMode.
 *    Very stable; good for smooth cutoff sweeps.
 *
 *    Per-sample update equations (Chamberlin topology):
 *      low  +=  f * band
 *      high  =  in - low - q * band
 *      band +=  f * high
 *    where f = 2·sin(π·cutoff/sample_rate), q = 1/resonance.
 *
 * 2. MOOG — Moog Ladder Filter
 *    4-stage lowpass filter with resonance feedback. Models the transistor
 *    ladder circuit found in Moog synthesisers. The transistor non-linearity
 *    (soft saturation) is approximated with fast_tanhf().
 *
 *    Per-sample:
 *      feedback = resonance * stage[3]
 *      x = tanh(in - feedback)
 *      for each stage: stage[k] = stage[k] + f * (tanh(x) - tanh(stage[k]))
 *      out = stage[3]
 *
 * Both filter types accept:
 *   - An audio input module (the signal to filter)
 *   - cutoff    — Hz, 20–20000
 *   - resonance — 0.0–4.0 (SVF) or 0.0–1.0 (Moog, 1.0 = self-oscillation)
 *
 * Modulation:
 *   Accepts mod_inputs[] with param="cutoff" for LFO filter sweeps.
 *
 * Example (Lua):
 *   local flt = Filter.new("svf")
 *   flt:setInput(osc)
 *   flt:setCutoff(800)
 *   flt:setResonance(0.7)
 *   flt:setMode("lowpass")
 */

#ifndef LUASYNTH_FILTER_H
#define LUASYNTH_FILTER_H

#include "../module.h"

/* -------------------------------------------------------------------------
 * FilterType — which topology to use.
 * ---------------------------------------------------------------------- */
typedef enum {
    FILTER_SVF,
    FILTER_MOOG
} FilterType;

/* -------------------------------------------------------------------------
 * FilterMode — which output tap of the SVF to use.
 * (Moog ladder is always lowpass.)
 * ---------------------------------------------------------------------- */
typedef enum {
    FILTER_LOWPASS,
    FILTER_BANDPASS,
    FILTER_HIGHPASS
} FilterMode;

/* -------------------------------------------------------------------------
 * FilterData — private state for a filter module.
 * ---------------------------------------------------------------------- */
typedef struct {
    FilterType  type;
    FilterMode  mode;
    float       cutoff;      /* cutoff frequency in Hz */
    float       resonance;   /* resonance [0, 4] SVF; [0, 1] Moog */

    /* SVF state variables */
    float       svf_low;
    float       svf_band;

    /* Moog ladder state */
    float       moog_stage[4];
    float       moog_in;
} FilterData;

/* -------------------------------------------------------------------------
 * filter_create — allocate a filter module.
 *
 * @param type  FILTER_SVF or FILTER_MOOG
 * @return      pointer to a new Module, or NULL on OOM
 * ---------------------------------------------------------------------- */
Module *filter_create(FilterType type);

/* -------------------------------------------------------------------------
 * filter_set_cutoff — update the cutoff frequency.
 * Caller must hold SDL audio lock.
 *
 * @param mod     the filter module
 * @param cutoff  frequency in Hz [20, 20000]
 * ---------------------------------------------------------------------- */
void filter_set_cutoff(Module *mod, float cutoff);

/* -------------------------------------------------------------------------
 * filter_set_resonance — update the resonance.
 *
 * @param mod        the filter module
 * @param resonance  [0.0, 4.0] for SVF; [0.0, 1.0] for Moog
 * ---------------------------------------------------------------------- */
void filter_set_resonance(Module *mod, float resonance);

/* -------------------------------------------------------------------------
 * filter_set_mode — set the SVF output mode.
 *
 * @param mod   the filter module
 * @param mode  FILTER_LOWPASS, FILTER_BANDPASS, or FILTER_HIGHPASS
 * ---------------------------------------------------------------------- */
void filter_set_mode(Module *mod, FilterMode mode);

/* -------------------------------------------------------------------------
 * filter_type_from_string / filter_mode_from_string — parse names.
 * ---------------------------------------------------------------------- */
FilterType filter_type_from_string(const char *s);
FilterMode filter_mode_from_string(const char *s);

#endif /* LUASYNTH_FILTER_H */
