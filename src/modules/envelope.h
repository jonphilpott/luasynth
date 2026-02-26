/**
 * envelope.h — ADSR Envelope Generator Module
 *
 * An ADSR envelope shapes the amplitude of a signal over time in four stages:
 *
 *   Attack  — ramp from 0 → 1 over `attack` seconds
 *   Decay   — ramp from 1 → sustain level over `decay` seconds
 *   Sustain — hold at `sustain` level until released
 *   Release — ramp from sustain → 0 over `release` seconds
 *
 * The envelope multiplies the output of its input module each sample.
 *
 * Thread Safety:
 *   trigger() and release() are called from the Lua / main thread.
 *   The audio thread reads the stage and computes envelope values.
 *   We use C11 _Atomic int flags so no mutex is needed for these single-word
 *   writes — the audio thread atomically checks and clears them.
 *
 * Example (Lua):
 *   local osc = Osc.new("sine", 440)
 *   local env = Envelope.new(0.01, 0.1, 0.7, 0.3)
 *   env:setInput(osc)
 *   env:trigger()
 *   Output.set(env)
 */

#ifndef LUASYNTH_ENVELOPE_H
#define LUASYNTH_ENVELOPE_H

#include <stdatomic.h>
#include "../module.h"

/* -------------------------------------------------------------------------
 * EnvStage — which phase of the ADSR we are currently in.
 * ---------------------------------------------------------------------- */
typedef enum {
    ENV_IDLE,
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE
} EnvStage;

/* -------------------------------------------------------------------------
 * EnvelopeData — private state for the envelope module.
 * ---------------------------------------------------------------------- */
typedef struct {
    double       attack;         /* seconds [0.001, 10] */
    double       decay;          /* seconds [0.001, 10] */
    double       sustain;        /* level   [0, 1] */
    double       release;        /* seconds [0.001, 10] */

    EnvStage     stage;          /* current ADSR stage */
    double       current_level;  /* current envelope output value */
    double       time_in_stage;  /* seconds elapsed in current stage */

    _Atomic int  triggered;      /* set by Lua thread; cleared by audio thread */
    _Atomic int  released;       /* set by Lua thread; cleared by audio thread */
} EnvelopeData;

/* -------------------------------------------------------------------------
 * envelope_create — allocate an envelope module.
 *
 * @param attack   attack time in seconds
 * @param decay    decay time in seconds
 * @param sustain  sustain level [0, 1]
 * @param release  release time in seconds
 * @return         pointer to a new Module, or NULL on OOM
 * ---------------------------------------------------------------------- */
Module *envelope_create(double attack, double decay, double sustain, double release);

/* -------------------------------------------------------------------------
 * envelope_trigger — start the attack stage.
 * Safe to call from the main thread; uses atomic write.
 *
 * @param mod  the envelope module
 * ---------------------------------------------------------------------- */
void envelope_trigger(Module *mod);

/* -------------------------------------------------------------------------
 * envelope_release — start the release stage.
 * Safe to call from the main thread; uses atomic write.
 *
 * @param mod  the envelope module
 * ---------------------------------------------------------------------- */
void envelope_release(Module *mod);

/* -------------------------------------------------------------------------
 * envelope_set_* — update ADSR parameters.
 * Caller should hold SDL audio lock.
 * ---------------------------------------------------------------------- */
void envelope_set_attack(Module *mod, double attack);
void envelope_set_decay(Module *mod, double decay);
void envelope_set_sustain(Module *mod, double sustain);
void envelope_set_release(Module *mod, double release);

#endif /* LUASYNTH_ENVELOPE_H */
