/**
 * envelope.c — ADSR Envelope Generator implementation
 *
 * See envelope.h for design notes.
 */

#include <stdlib.h>
#include <string.h>
#include "envelope.h"
#include "../util/math_util.h"

/* -------------------------------------------------------------------------
 * envelope_process — generates the amplitude envelope and applies it to
 * the signal from the input module.
 *
 * For each sample:
 *   1. Check the atomic flags: if triggered, start attack; if released, start release.
 *   2. Advance time_in_stage by 1/sample_rate seconds.
 *   3. Compute envelope level based on current stage.
 *   4. Multiply input audio by the envelope level.
 * ---------------------------------------------------------------------- */
static int envelope_process(Module *self, float *out_buf, int num_frames, int sample_rate) {
    EnvelopeData *d = (EnvelopeData *)self->data;
    double dt = 1.0 / (double)sample_rate;  /* seconds per sample */

    /* Pull audio from input module */
    float *src = self->scratch;
    if (self->num_inputs > 0 && self->inputs[0] && self->inputs[0]->process) {
        self->inputs[0]->process(self->inputs[0], src, num_frames, sample_rate);
    } else {
        memset(src, 0, sizeof(float) * (size_t)(num_frames * 2));
    }

    for (int i = 0; i < num_frames; i++) {
        /* Step 1: Check trigger/release flags set by the Lua thread */
        if (atomic_exchange(&d->triggered, 0)) {
            d->stage         = ENV_ATTACK;
            d->time_in_stage = 0.0;
            /* Don't reset current_level — allows re-triggering without clicks */
        }
        if (atomic_exchange(&d->released, 0) && d->stage != ENV_IDLE) {
            d->stage         = ENV_RELEASE;
            d->time_in_stage = 0.0;
        }

        /* Step 2 + 3: Advance time and compute envelope level */
        d->time_in_stage += dt;

        switch (d->stage) {
            case ENV_IDLE:
                d->current_level = 0.0;
                break;

            case ENV_ATTACK: {
                /* Linear ramp from 0 to 1 over `attack` seconds */
                double t = d->attack > 0.0 ? d->time_in_stage / d->attack : 1.0;
                d->current_level = t;
                if (t >= 1.0) {
                    d->current_level = 1.0;
                    d->stage         = ENV_DECAY;
                    d->time_in_stage = 0.0;
                }
                break;
            }

            case ENV_DECAY: {
                /* Linear ramp from 1 to sustain over `decay` seconds */
                double t = d->decay > 0.0 ? d->time_in_stage / d->decay : 1.0;
                d->current_level = 1.0 - t * (1.0 - d->sustain);
                if (t >= 1.0) {
                    d->current_level = d->sustain;
                    d->stage         = ENV_SUSTAIN;
                    d->time_in_stage = 0.0;
                }
                break;
            }

            case ENV_SUSTAIN:
                d->current_level = d->sustain;
                break;

            case ENV_RELEASE: {
                /* Linear ramp from sustain to 0 over `release` seconds */
                double t = d->release > 0.0 ? d->time_in_stage / d->release : 1.0;
                double start_level = d->sustain; /* approximate: starts at sustain */
                d->current_level = start_level * (1.0 - t);
                if (t >= 1.0) {
                    d->current_level = 0.0;
                    d->stage         = ENV_IDLE;
                }
                break;
            }
        }

        /* Step 4: Apply envelope to input audio */
        float level = (float)clampf((float)d->current_level, 0.0f, 1.0f);
        out_buf[i * 2 + 0] = src[i * 2 + 0] * level;
        out_buf[i * 2 + 1] = src[i * 2 + 1] * level;
    }

    return 0;
}

static void envelope_destroy(Module *self) {
    free(self->data);
    self->data = NULL;
}

Module *envelope_create(double attack, double decay, double sustain, double release) {
    Module *mod = module_alloc(MODULE_ENVELOPE);
    if (!mod) return NULL;

    EnvelopeData *d = (EnvelopeData *)calloc(1, sizeof(EnvelopeData));
    if (!d) { module_free(mod); return NULL; }

    d->attack    = attack;
    d->decay     = decay;
    d->sustain   = sustain;
    d->release   = release;
    d->stage     = ENV_IDLE;
    atomic_store(&d->triggered, 0);
    atomic_store(&d->released,  0);

    mod->data    = d;
    mod->name    = "envelope";
    mod->process = envelope_process;
    mod->destroy = envelope_destroy;
    return mod;
}

void envelope_trigger(Module *mod) {
    atomic_store(&((EnvelopeData *)mod->data)->triggered, 1);
}

void envelope_release(Module *mod) {
    atomic_store(&((EnvelopeData *)mod->data)->released, 1);
}

void envelope_set_attack(Module *mod, double v)  { ((EnvelopeData *)mod->data)->attack  = v; }
void envelope_set_decay(Module *mod, double v)   { ((EnvelopeData *)mod->data)->decay   = v; }
void envelope_set_sustain(Module *mod, double v) { ((EnvelopeData *)mod->data)->sustain = v; }
void envelope_set_release(Module *mod, double v) { ((EnvelopeData *)mod->data)->release = v; }
