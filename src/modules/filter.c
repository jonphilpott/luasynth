/**
 * filter.c — Filter Module implementation (SVF + Moog Ladder)
 *
 * See filter.h for design notes and equations.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "filter.h"
#include "../util/math_util.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* -------------------------------------------------------------------------
 * apply_svf — run one sample through the Chamberlin SVF.
 *
 * The state variables `low` and `band` carry the filter's memory between
 * samples. They are updated in place each call.
 *
 * f = cutoff coefficient: how fast energy moves between integrators.
 * q = damping (inverse resonance): 1.0 = critically damped.
 *
 * @param in    input sample
 * @param low   pointer to the lowpass state variable (in/out)
 * @param band  pointer to the bandpass state variable (in/out)
 * @param f     cutoff coefficient
 * @param q     damping (= 1/resonance)
 * @param mode  which output tap to return
 * @return      filtered sample
 * ---------------------------------------------------------------------- */
static float apply_svf(float in, float *low, float *band,
                       float f, float q, FilterMode mode) {
    /* Chamberlin update — two coupled integrators */
    *low  += f * (*band);
    float high = in - *low - q * (*band);
    *band += f * high;

    switch (mode) {
        case FILTER_LOWPASS:  return *low;
        case FILTER_BANDPASS: return *band;
        case FILTER_HIGHPASS: return high;
    }
    return *low;
}

/* -------------------------------------------------------------------------
 * apply_moog — run one sample through the Moog ladder filter.
 *
 * 4 cascaded one-pole lowpass stages with feedback. The resonance feedback
 * path uses tanh saturation to prevent blowup and add warmth.
 *
 * @param in        input sample
 * @param stage     array of 4 stage state variables (in/out)
 * @param f         cutoff coefficient (0–1, computed from cutoff/sample_rate)
 * @param res       resonance [0.0, ~0.99]
 * @return          filtered output (= stage[3])
 * ---------------------------------------------------------------------- */
static float apply_moog(float in, float *stage, float f, float res) {
    /* Feedback from output stage back to input creates resonance */
    float feedback = res * stage[3];

    /* Saturate the input to prevent blow-up and add harmonic warmth */
    float x = fast_tanhf(in - feedback);

    /* Each stage is a one-pole lowpass: new = old + f * (tanh(x) - tanh(old))
     * The tanh on stage[k] models the transistor's non-linear characteristic. */
    stage[0] += f * (fast_tanhf(x)        - fast_tanhf(stage[0]));
    stage[1] += f * (fast_tanhf(stage[0]) - fast_tanhf(stage[1]));
    stage[2] += f * (fast_tanhf(stage[1]) - fast_tanhf(stage[2]));
    stage[3] += f * (fast_tanhf(stage[2]) - fast_tanhf(stage[3]));

    return stage[3];
}

/* -------------------------------------------------------------------------
 * filter_process
 * ---------------------------------------------------------------------- */
static int filter_process(Module *self, float *out_buf, int num_frames, int sample_rate) {
    FilterData *d = (FilterData *)self->data;

    /* Pull audio from ALL upstream inputs, mixing them additively into scratch.
     * We iterate every input rather than just inputs[0] because control-only
     * modules (e.g. the clock) may be wired here as a side-channel so that
     * their process() runs each audio frame — even though they output silence.
     * We reuse out_buf as a per-input temporary (safe: we haven't written it yet). */
    float *src_buf = self->scratch;
    memset(src_buf, 0, sizeof(float) * (size_t)(num_frames * 2));
    for (int in_idx = 0; in_idx < self->num_inputs; in_idx++) {
        Module *inp = self->inputs[in_idx];
        if (!inp || !inp->process) continue;
        inp->process(inp, out_buf, num_frames, sample_rate);
        for (int j = 0; j < num_frames * 2; j++) src_buf[j] += out_buf[j];
    }

    /* Pre-compute per-block cutoff (modulation is applied per-frame below) */
    float base_cutoff = d->cutoff;

    /* Pre-compute SVF coefficients:
     * f = 2 * sin(π * cutoff / sample_rate)
     * Clamped to [0, 1.99] to keep the filter stable.
     * q = 1 / (resonance + 0.001) — add epsilon to avoid division by zero */
    float q = 1.0f / (d->resonance + 0.001f);
    q = clampf(q, 0.2f, 10.0f);

    /* Moog cutoff coefficient: 1 - exp(-2π * cutoff / sr) — approximate with
     * a cheap linear mapping for low cutoffs: f ≈ π * cutoff / sample_rate */
    float moog_f_base = (float)(M_PI * (double)base_cutoff / (double)sample_rate);
    moog_f_base = clampf(moog_f_base, 0.0001f, 0.999f);

    for (int i = 0; i < num_frames; i++) {
        /* Apply cutoff modulation */
        float cutoff = base_cutoff;
        for (int m = 0; m < self->num_mod_inputs; m++) {
            ModInput *mi = &self->mod_inputs[m];
            if (strcmp(mi->param, "cutoff") == 0 && mi->source && mi->source->process) {
                float lsamp[2];
                mi->source->process(mi->source, lsamp, 1, sample_rate);
                cutoff += lsamp[0] * mi->depth;
            }
        }
        cutoff = clampf(cutoff, 20.0f, (float)sample_rate * 0.48f);

        float svf_f  = 2.0f * sinf((float)M_PI * cutoff / (float)sample_rate);
        svf_f = clampf(svf_f, 0.0001f, 1.99f);
        float moog_f = (float)(M_PI * (double)cutoff / (double)sample_rate);
        moog_f = clampf(moog_f, 0.0001f, 0.999f);

        /* Process left channel */
        float inL = src_buf[i * 2 + 0];
        float inR = src_buf[i * 2 + 1];
        float outL, outR;

        if (d->type == FILTER_SVF) {
            outL = apply_svf(inL, &d->svf_low, &d->svf_band, svf_f, q, d->mode);
            /* For stereo: run the SVF again with the same state for right channel
             * (simple mono SVF applied to both channels; state is shared) */
            outR = outL; /* simplified: treat as mono */
            (void)inR;
        } else {
            /* Moog ladder — mono (process L only) */
            outL = apply_moog(inL, d->moog_stage, moog_f, d->resonance);
            outR = outL;
            (void)inR;
        }

        out_buf[i * 2 + 0] = outL;
        out_buf[i * 2 + 1] = outR;
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * filter_destroy
 * ---------------------------------------------------------------------- */
static void filter_destroy(Module *self) {
    free(self->data);
    self->data = NULL;
}

/* -------------------------------------------------------------------------
 * filter_create
 * ---------------------------------------------------------------------- */
Module *filter_create(FilterType type) {
    Module *mod = module_alloc(MODULE_FILTER);
    if (!mod) return NULL;

    FilterData *d = (FilterData *)calloc(1, sizeof(FilterData));
    if (!d) { module_free(mod); return NULL; }

    d->type      = type;
    d->mode      = FILTER_LOWPASS;
    d->cutoff    = 1000.0f;
    d->resonance = 0.5f;

    mod->data    = d;
    mod->name    = "filter";
    mod->process = filter_process;
    mod->destroy = filter_destroy;
    return mod;
}

/* -------------------------------------------------------------------------
 * Parameter setters
 * ---------------------------------------------------------------------- */
void filter_set_cutoff(Module *mod, float cutoff) {
    ((FilterData *)mod->data)->cutoff = cutoff;
}

void filter_set_resonance(Module *mod, float resonance) {
    ((FilterData *)mod->data)->resonance = resonance;
}

void filter_set_mode(Module *mod, FilterMode mode) {
    ((FilterData *)mod->data)->mode = mode;
}

FilterType filter_type_from_string(const char *s) {
    if (strcmp(s, "moog") == 0) return FILTER_MOOG;
    return FILTER_SVF;
}

FilterMode filter_mode_from_string(const char *s) {
    if (strcmp(s, "bandpass") == 0) return FILTER_BANDPASS;
    if (strcmp(s, "highpass") == 0) return FILTER_HIGHPASS;
    return FILTER_LOWPASS;
}
