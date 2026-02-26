/**
 * delay.c — Delay Line Module implementation
 *
 * See delay.h for design notes.
 */

#include <stdlib.h>
#include <string.h>
#include "delay.h"
#include "../util/math_util.h"

/* -------------------------------------------------------------------------
 * delay_process
 * ---------------------------------------------------------------------- */
static int delay_process(Module *self, float *out_buf, int num_frames, int sample_rate) {
    DelayData *d = (DelayData *)self->data;

    /* Pull audio from ALL upstream inputs, mixing them additively into scratch.
     * Same pattern as filter_process — control modules (clock) may be wired
     * here as a side-channel so their process() advances each audio frame.
     * out_buf is used as a per-input temporary (safe: not yet written). */
    float *src = self->scratch;
    memset(src, 0, sizeof(float) * (size_t)(num_frames * 2));
    for (int in_idx = 0; in_idx < self->num_inputs; in_idx++) {
        Module *inp = self->inputs[in_idx];
        if (!inp || !inp->process) continue;
        inp->process(inp, out_buf, num_frames, sample_rate);
        for (int j = 0; j < num_frames * 2; j++) src[j] += out_buf[j];
    }

    /* Compute current delay in frames */
    int delay_frames = (int)(d->delay_time * (float)sample_rate);
    delay_frames = delay_frames < 1 ? 1 : (delay_frames >= d->buf_len ? d->buf_len - 1 : delay_frames);

    for (int i = 0; i < num_frames; i++) {
        float inL = src[i * 2 + 0];
        float inR = src[i * 2 + 1];

        /* Calculate the read position: look back `delay_frames` frames
         * using modulo arithmetic to wrap around the circular buffer. */
        int read_pos = (d->write_head - delay_frames + d->buf_len) % d->buf_len;

        float delayedL = d->buffer[read_pos * 2 + 0];
        float delayedR = d->buffer[read_pos * 2 + 1];

        /* Write to buffer: input + feedback from delayed output */
        d->buffer[d->write_head * 2 + 0] = inL + delayedL * d->feedback;
        d->buffer[d->write_head * 2 + 1] = inR + delayedR * d->feedback;

        /* Mix dry and wet signals */
        out_buf[i * 2 + 0] = inL * d->dry + delayedL * d->wet;
        out_buf[i * 2 + 1] = inR * d->dry + delayedR * d->wet;

        /* Advance the write head, wrapping at the end of the buffer */
        d->write_head = (d->write_head + 1) % d->buf_len;
    }

    return 0;
}

static void delay_destroy(Module *self) {
    DelayData *d = (DelayData *)self->data;
    if (d) {
        free(d->buffer);
        free(d);
        self->data = NULL;
    }
}

Module *delay_create(float delay_time, float feedback, int sample_rate) {
    Module *mod = module_alloc(MODULE_DELAY);
    if (!mod) return NULL;

    DelayData *d = (DelayData *)calloc(1, sizeof(DelayData));
    if (!d) { module_free(mod); return NULL; }

    /* Pre-allocate the maximum possible delay buffer */
    int buf_len = (int)(DELAY_MAX_TIME_SECONDS * (float)sample_rate) + 1;
    d->buffer = (float *)calloc((size_t)(buf_len * 2), sizeof(float));
    if (!d->buffer) { free(d); module_free(mod); return NULL; }

    d->buf_len    = buf_len;
    d->write_head = 0;
    d->delay_time = clampf(delay_time, 0.001f, DELAY_MAX_TIME_SECONDS);
    d->feedback   = clampf(feedback, 0.0f, 0.99f);
    d->wet        = 0.5f;
    d->dry        = 1.0f;

    mod->data    = d;
    mod->name    = "delay";
    mod->process = delay_process;
    mod->destroy = delay_destroy;
    return mod;
}

void delay_set_delay_time(Module *mod, float v) {
    ((DelayData *)mod->data)->delay_time = clampf(v, 0.001f, DELAY_MAX_TIME_SECONDS);
}
void delay_set_feedback(Module *mod, float v) {
    ((DelayData *)mod->data)->feedback = clampf(v, 0.0f, 0.99f);
}
void delay_set_wet(Module *mod, float v) {
    ((DelayData *)mod->data)->wet = clampf(v, 0.0f, 1.0f);
}
void delay_set_dry(Module *mod, float v) {
    ((DelayData *)mod->data)->dry = clampf(v, 0.0f, 1.0f);
}
