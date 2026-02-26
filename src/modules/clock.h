/**
 * clock.h — BPM Clock Module
 *
 * The clock module generates regular beat events at a configurable tempo
 * (BPM). It operates on the audio thread, counting samples, and fires events
 * into a ring buffer whenever a beat occurs. The main thread drains the ring
 * buffer and calls a registered Lua callback.
 *
 * This demonstrates the SPSC ring buffer pattern for safe audio→main thread
 * communication without blocking the audio callback.
 *
 * Example (Lua):
 *   local clk = Clock.new(120)
 *   clk:onBeat(function(beat)
 *       osc:setFrequency(notes[beat % #notes + 1])
 *       env:trigger()
 *   end)
 *   clk:start()
 */

#ifndef LUASYNTH_CLOCK_H
#define LUASYNTH_CLOCK_H

#include <stdatomic.h>
#include "../module.h"
#include "../util/ringbuffer.h"

/* -------------------------------------------------------------------------
 * ClockData — private state for the clock module.
 * ---------------------------------------------------------------------- */
typedef struct {
    double       bpm;            /* beats per minute */
    double       phase;          /* sample counter accumulator [0, 1) */
    int          beat_count;     /* total beats fired so far */
    _Atomic int  running;        /* 1 = clock is running */
    RingBuffer  *ring;           /* pointer to the shared ring buffer */
} ClockData;

/* -------------------------------------------------------------------------
 * clock_create — allocate a clock module.
 *
 * @param bpm   initial tempo in beats per minute
 * @param ring  shared ring buffer for beat events → main thread
 * @return      pointer to a new Module, or NULL on OOM
 * ---------------------------------------------------------------------- */
Module *clock_create(double bpm, RingBuffer *ring);

void clock_set_bpm(Module *mod, double bpm);
void clock_start(Module *mod);
void clock_stop(Module *mod);

#endif /* LUASYNTH_CLOCK_H */
