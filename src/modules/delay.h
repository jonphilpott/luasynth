/**
 * delay.h — Delay Line Module
 *
 * A delay line stores incoming audio in a circular buffer and plays it back
 * after a configurable delay time. Adding a feedback path creates echo and
 * reverb-like effects.
 *
 * Parameters:
 *   delay_time  — delay time in seconds [0.001, max_delay_time]
 *   feedback    — fraction of output fed back into input [0, 0.99]
 *   wet         — mix of delayed signal [0, 1]
 *   dry         — mix of direct signal  [0, 1]
 *
 * Circular buffer mechanics:
 *   - write_head advances by 1 each sample.
 *   - read_head = (write_head - delay_samples) wrapped with modulo.
 *   - Buffer size = sample_rate × max_delay_time.
 *
 * Example (Lua):
 *   local dly = Delay.new(0.4, 0.6)   -- 400ms delay, 60% feedback
 *   dly:setInput(osc)
 *   dly:setWet(0.5)
 *   dly:setDry(0.5)
 *   Output.set(dly)
 */

#ifndef LUASYNTH_DELAY_H
#define LUASYNTH_DELAY_H

#include "../module.h"

/* Maximum delay time supported (seconds). Buffer pre-allocated at init. */
#define DELAY_MAX_TIME_SECONDS 2.0f

/* -------------------------------------------------------------------------
 * DelayData — private state.
 * ---------------------------------------------------------------------- */
typedef struct {
    float  *buffer;       /* circular buffer, length = buf_len * 2 (stereo) */
    int     buf_len;      /* buffer length in frames */
    int     write_head;   /* current write position (frames) */

    float   delay_time;   /* delay time in seconds */
    float   feedback;     /* feedback gain [0, 0.99] */
    float   wet;          /* wet mix */
    float   dry;          /* dry mix */
} DelayData;

/* -------------------------------------------------------------------------
 * delay_create — allocate a delay module.
 *
 * @param delay_time  initial delay time in seconds
 * @param feedback    feedback gain [0, 0.99]
 * @param sample_rate audio sample rate (needed to size the buffer)
 * @return            pointer to a new Module, or NULL on OOM
 * ---------------------------------------------------------------------- */
Module *delay_create(float delay_time, float feedback, int sample_rate);

void delay_set_delay_time(Module *mod, float delay_time);
void delay_set_feedback(Module *mod, float feedback);
void delay_set_wet(Module *mod, float wet);
void delay_set_dry(Module *mod, float dry);

#endif /* LUASYNTH_DELAY_H */
