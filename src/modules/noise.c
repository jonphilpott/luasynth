/**
 * noise.c — White Noise Source implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include "noise.h"

static int noise_process(Module *self, float *out_buf, int num_frames, int sample_rate) {
    (void)sample_rate;
    NoiseData *d = (NoiseData *)self->data;

    for (int i = 0; i < num_frames; i++) {
        /* rand() returns [0, RAND_MAX]; map to [-1, 1] */
        float sample = ((float)rand() / (float)(RAND_MAX / 2)) - 1.0f;
        sample *= d->amplitude;
        out_buf[i * 2 + 0] = sample;
        out_buf[i * 2 + 1] = sample;
    }
    return 0;
}

static void noise_destroy(Module *self) {
    free(self->data);
    self->data = NULL;
}

Module *noise_create(void) {
    Module *mod = module_alloc(MODULE_NOISE);
    if (!mod) return NULL;

    NoiseData *d = (NoiseData *)calloc(1, sizeof(NoiseData));
    if (!d) { module_free(mod); return NULL; }

    d->amplitude = 1.0f;
    mod->data    = d;
    mod->name    = "noise";
    mod->process = noise_process;
    mod->destroy = noise_destroy;
    return mod;
}

void noise_set_amplitude(Module *mod, float amplitude) {
    ((NoiseData *)mod->data)->amplitude = amplitude;
}
