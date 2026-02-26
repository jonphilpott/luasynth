/**
 * clock.c — BPM Clock Module implementation
 */

#include <stdlib.h>
#include <string.h>
#include "clock.h"

/* -------------------------------------------------------------------------
 * clock_process — accumulates sample time; pushes beat events to ring buffer.
 *
 * The clock uses a phase accumulator similar to the oscillator:
 *   phase += bpm / (60 * sample_rate)  per sample
 * When phase crosses 1.0, a beat has occurred: push to ring buffer and wrap.
 *
 * The clock's output buffer is filled with zeros — it produces no audio;
 * it's a control-only module.
 * ---------------------------------------------------------------------- */
static int clock_process(Module *self, float *out_buf, int num_frames, int sample_rate) {
    ClockData *d = (ClockData *)self->data;

    /* Always output silence — this module is control-only */
    memset(out_buf, 0, sizeof(float) * (size_t)(num_frames * 2));

    if (!atomic_load(&d->running)) return 0;

    /* beats per sample = BPM / (60 * sample_rate) */
    double inc = d->bpm / (60.0 * (double)sample_rate);

    for (int i = 0; i < num_frames; i++) {
        d->phase += inc;
        if (d->phase >= 1.0) {
            d->phase -= 1.0;
            d->beat_count++;

            /* Push a beat event to the ring buffer.
             * The main thread will pick this up and call the Lua callback. */
            RingEvent ev;
            ev.type  = RING_EVENT_BEAT;
            ev.value = (float)d->beat_count;
            ringbuf_push(d->ring, ev);
        }
    }

    return 0;
}

static void clock_destroy(Module *self) {
    free(self->data);
    self->data = NULL;
}

Module *clock_create(double bpm, RingBuffer *ring) {
    Module *mod = module_alloc(MODULE_CLOCK);
    if (!mod) return NULL;

    ClockData *d = (ClockData *)calloc(1, sizeof(ClockData));
    if (!d) { module_free(mod); return NULL; }

    d->bpm        = bpm;
    d->phase      = 0.0;
    d->beat_count = 0;
    d->ring       = ring;
    atomic_store(&d->running, 0);

    mod->data    = d;
    mod->name    = "clock";
    mod->process = clock_process;
    mod->destroy = clock_destroy;
    return mod;
}

void clock_set_bpm(Module *mod, double bpm) {
    ((ClockData *)mod->data)->bpm = bpm;
}

void clock_start(Module *mod) {
    atomic_store(&((ClockData *)mod->data)->running, 1);
}

void clock_stop(Module *mod) {
    atomic_store(&((ClockData *)mod->data)->running, 0);
}
