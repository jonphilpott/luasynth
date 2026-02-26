/**
 * osc.h — Oscillator Module
 *
 * Generates periodic audio waveforms using a phase accumulator.
 *
 * Waveforms available:
 *   OSC_SINE     — smooth sinusoid: sin(2π·phase)
 *   OSC_SQUARE   — ±1 pulse at 50% duty cycle
 *   OSC_SAW      — sawtooth: ramp from -1 to +1
 *   OSC_TRIANGLE — symmetric triangle wave
 *
 * All waveforms are naive (no PolyBLEP anti-aliasing). At higher frequencies
 * you will hear aliasing artefacts — this is intentional for the learning
 * example; adding PolyBLEP is a natural extension exercise.
 *
 * Phase accumulator model:
 *   phase ∈ [0, 1); advances by (frequency / sample_rate) each sample.
 *   When phase ≥ 1 it wraps back: phase -= 1.0.
 *   This produces exactly `frequency` cycles per second regardless of the
 *   sample rate.
 *
 * Modulation:
 *   The oscillator respects mod_inputs[] on its parent Module. If an LFO is
 *   connected with param="frequency", the LFO's output is added (scaled by
 *   depth) to the base frequency each sample.
 *
 * Example (Lua):
 *   local osc = Osc.new("sine", 440.0)
 *   osc:setFrequency(880.0)
 *   Output.set(osc)
 */

#ifndef LUASYNTH_OSC_H
#define LUASYNTH_OSC_H

#include "../module.h"

/* -------------------------------------------------------------------------
 * OscWaveform — selects the waveform shape.
 * ---------------------------------------------------------------------- */
typedef enum {
    OSC_SINE,
    OSC_SQUARE,
    OSC_SAW,
    OSC_TRIANGLE
} OscWaveform;

/* -------------------------------------------------------------------------
 * OscData — private data for an oscillator module.
 * ---------------------------------------------------------------------- */
typedef struct {
    OscWaveform  waveform;
    double       frequency;   /* Hz */
    double       amplitude;   /* 0.0 – 1.0 linear gain */
    double       phase;       /* current phase [0, 1) */
} OscData;

/* -------------------------------------------------------------------------
 * osc_create — allocate and initialise an oscillator module.
 *
 * @param waveform   the waveform shape (OSC_SINE, OSC_SAW, etc.)
 * @param frequency  initial frequency in Hz (e.g. 440.0)
 * @return           pointer to a new Module, or NULL on OOM
 * ---------------------------------------------------------------------- */
Module *osc_create(OscWaveform waveform, double frequency);

/* -------------------------------------------------------------------------
 * osc_set_frequency — update the oscillator frequency (thread-safe via caller
 * holding SDL audio lock before calling this).
 *
 * @param mod        the oscillator module
 * @param frequency  new frequency in Hz
 * ---------------------------------------------------------------------- */
void osc_set_frequency(Module *mod, double frequency);

/* -------------------------------------------------------------------------
 * osc_set_amplitude — update the oscillator amplitude.
 *
 * @param mod        the oscillator module
 * @param amplitude  linear gain [0.0, 1.0]
 * ---------------------------------------------------------------------- */
void osc_set_amplitude(Module *mod, double amplitude);

/* -------------------------------------------------------------------------
 * osc_set_waveform — change the waveform.
 *
 * @param mod       the oscillator module
 * @param waveform  new waveform type
 * ---------------------------------------------------------------------- */
void osc_set_waveform(Module *mod, OscWaveform waveform);

/* -------------------------------------------------------------------------
 * osc_waveform_from_string — parse a waveform name.
 *
 * @param s   string: "sine", "square", "saw", or "triangle"
 * @return    the matching OscWaveform, or OSC_SINE as default
 * ---------------------------------------------------------------------- */
OscWaveform osc_waveform_from_string(const char *s);

#endif /* LUASYNTH_OSC_H */
