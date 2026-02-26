/**
 * osc.c — Oscillator Module implementation
 *
 * See osc.h for design notes.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "osc.h"
#include "../util/math_util.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* -------------------------------------------------------------------------
 * osc_process — the audio callback for the oscillator.
 *
 * Step 1: For each frame, check if any mod_inputs affect frequency.
 * Step 2: Compute the phase increment for this sample.
 * Step 3: Generate the waveform sample from the current phase.
 * Step 4: Advance and wrap the phase accumulator.
 * Step 5: Mix with upstream audio inputs (additive).
 * ---------------------------------------------------------------------- */
static int osc_process(Module *self, float *out_buf, int num_frames, int sample_rate) {
    OscData *d = (OscData *)self->data;

    /* Step 1–4: Generate our own signal */
    for (int i = 0; i < num_frames; i++) {
        /* Step 1: Apply frequency modulation from mod_inputs */
        double freq = d->frequency;
        for (int m = 0; m < self->num_mod_inputs; m++) {
            ModInput *mi = &self->mod_inputs[m];
            if (strcmp(mi->param, "frequency") == 0 && mi->source && mi->source->process) {
                /* Get one frame of LFO output into a tiny temporary buffer */
                float lfo_sample[2];
                mi->source->process(mi->source, lfo_sample, 1, sample_rate);
                /* lfo_sample[0] is the left channel value [-1, 1] */
                freq += (double)(lfo_sample[0] * mi->depth);
            }
        }
        if (freq < 0.0) freq = 0.0;

        /* Step 2: Phase increment = frequency / sample_rate
         * Dividing by sample_rate converts "cycles per second" into
         * "fraction of a full cycle per sample". */
        double inc = freq / (double)sample_rate;

        /* Step 3: Compute waveform sample from phase ∈ [0, 1) */
        float sample = 0.0f;
        switch (d->waveform) {
            case OSC_SINE:
                /* sin() expects radians; phase*2π maps [0,1) → [0,2π) */
                sample = (float)sin(d->phase * 2.0 * M_PI);
                break;

            case OSC_SQUARE:
                /* +1 for the first half of the cycle, -1 for the second.
                 * The discontinuity at 0.5 causes strong aliasing at HF. */
                sample = (d->phase < 0.5) ? 1.0f : -1.0f;
                break;

            case OSC_SAW:
                /* Linear ramp from -1 → +1 over [0,1), then jump to -1.
                 * Equivalent: sample = 2*phase - 1 */
                sample = (float)(2.0 * d->phase - 1.0);
                break;

            case OSC_TRIANGLE:
                /* Rise from -1 → +1 over [0, 0.5), then fall from +1 → -1
                 * over [0.5, 1). Peak-to-peak = 2; we scale back to [-1,1]. */
                if (d->phase < 0.5) {
                    sample = (float)(4.0 * d->phase - 1.0);
                } else {
                    sample = (float)(3.0 - 4.0 * d->phase);
                }
                break;
        }

        sample *= (float)d->amplitude;

        /* Step 5: Write stereo frame (both channels identical = mono source) */
        out_buf[i * 2 + 0] = sample;  /* left  */
        out_buf[i * 2 + 1] = sample;  /* right */

        /* Step 4: Advance and wrap the phase accumulator */
        d->phase += inc;
        if (d->phase >= 1.0) d->phase -= 1.0;
    }

    /* Mix in upstream audio inputs (e.g., if another oscillator is connected
     * as an audio input to this one — unusual but supported) */
    for (int in = 0; in < self->num_inputs; in++) {
        Module *src = self->inputs[in];
        if (!src || !src->process) continue;
        float *tmp = self->scratch;
        src->process(src, tmp, num_frames, sample_rate);
        for (int i = 0; i < num_frames * 2; i++) {
            out_buf[i] += tmp[i];
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * osc_destroy
 * ---------------------------------------------------------------------- */
static void osc_destroy(Module *self) {
    free(self->data);
    self->data = NULL;
}

/* -------------------------------------------------------------------------
 * osc_create
 * ---------------------------------------------------------------------- */
Module *osc_create(OscWaveform waveform, double frequency) {
    Module *mod = module_alloc(MODULE_OSC);
    if (!mod) return NULL;

    OscData *d = (OscData *)calloc(1, sizeof(OscData));
    if (!d) { module_free(mod); return NULL; }

    d->waveform  = waveform;
    d->frequency = frequency;
    d->amplitude = 1.0;
    d->phase     = 0.0;

    mod->data    = d;
    mod->name    = "osc";
    mod->process = osc_process;
    mod->destroy = osc_destroy;

    return mod;
}

/* -------------------------------------------------------------------------
 * Parameter setters
 * ---------------------------------------------------------------------- */
void osc_set_frequency(Module *mod, double frequency) {
    ((OscData *)mod->data)->frequency = frequency;
}

void osc_set_amplitude(Module *mod, double amplitude) {
    ((OscData *)mod->data)->amplitude = amplitude;
}

void osc_set_waveform(Module *mod, OscWaveform waveform) {
    ((OscData *)mod->data)->waveform = waveform;
}

OscWaveform osc_waveform_from_string(const char *s) {
    if (strcmp(s, "square")   == 0) return OSC_SQUARE;
    if (strcmp(s, "saw")      == 0) return OSC_SAW;
    if (strcmp(s, "triangle") == 0) return OSC_TRIANGLE;
    return OSC_SINE; /* default */
}
