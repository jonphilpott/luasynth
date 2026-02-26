/**
 * lfo.c — Low-Frequency Oscillator implementation
 */

#include <stdlib.h>
#include <math.h>
#include "lfo.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int lfo_process(Module *self, float *out_buf, int num_frames, int sample_rate) {
    LFOData *d = (LFOData *)self->data;

    for (int i = 0; i < num_frames; i++) {
        double inc = d->frequency / (double)sample_rate;

        float sample = 0.0f;
        switch (d->waveform) {
            case OSC_SINE:
                sample = (float)sin(d->phase * 2.0 * M_PI);
                break;
            case OSC_SQUARE:
                sample = (d->phase < 0.5) ? 1.0f : -1.0f;
                break;
            case OSC_SAW:
                sample = (float)(2.0 * d->phase - 1.0);
                break;
            case OSC_TRIANGLE:
                if (d->phase < 0.5) {
                    sample = (float)(4.0 * d->phase - 1.0);
                } else {
                    sample = (float)(3.0 - 4.0 * d->phase);
                }
                break;
        }

        sample *= (float)d->depth;

        out_buf[i * 2 + 0] = sample;
        out_buf[i * 2 + 1] = sample;

        d->phase += inc;
        if (d->phase >= 1.0) d->phase -= 1.0;
    }
    return 0;
}

static void lfo_destroy(Module *self) {
    free(self->data);
    self->data = NULL;
}

Module *lfo_create(OscWaveform waveform, double frequency) {
    Module *mod = module_alloc(MODULE_LFO);
    if (!mod) return NULL;

    LFOData *d = (LFOData *)calloc(1, sizeof(LFOData));
    if (!d) { module_free(mod); return NULL; }

    d->waveform  = waveform;
    d->frequency = frequency;
    d->depth     = 1.0;
    d->phase     = 0.0;

    mod->data    = d;
    mod->name    = "lfo";
    mod->process = lfo_process;
    mod->destroy = lfo_destroy;
    return mod;
}

void lfo_set_frequency(Module *mod, double frequency) {
    ((LFOData *)mod->data)->frequency = frequency;
}

void lfo_set_depth(Module *mod, double depth) {
    ((LFOData *)mod->data)->depth = depth;
}

void lfo_set_waveform(Module *mod, OscWaveform waveform) {
    ((LFOData *)mod->data)->waveform = waveform;
}
