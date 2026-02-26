/**
 * ringbuffer.h — Lock-Free Single-Producer Single-Consumer Ring Buffer
 *
 * Used to pass clock beat events from the SDL audio thread (producer) to the
 * main thread (consumer) without any mutex locking. This is important because:
 *
 *   - The audio callback must never block (mutexes can block).
 *   - The Lua VM must only be called from the main thread.
 *
 * How it works:
 *   - read_pos and write_pos are C11 _Atomic ints, guaranteeing that each
 *     thread sees the other's updates with acquire/release semantics.
 *   - The producer (audio thread) writes an event and increments write_pos.
 *   - The consumer (main thread) reads an event and increments read_pos.
 *   - No locking needed because only one thread writes and only one reads.
 *
 * Buffer capacity: RINGBUF_CAPACITY events. If the buffer fills up (producer
 * too fast, consumer not draining), new events are dropped and a warning is
 * printed. In practice the main thread processes events at ~1 kHz so this
 * should not happen at musical tempos.
 */

#ifndef LUASYNTH_RINGBUFFER_H
#define LUASYNTH_RINGBUFFER_H

#include <stdatomic.h>

#define RINGBUF_CAPACITY 64

/* -------------------------------------------------------------------------
 * RingEvent — one item stored in the ring buffer.
 * We keep it small so the ring buffer fits in a cache line.
 * ---------------------------------------------------------------------- */
typedef struct {
    int   type;      /* event type (e.g., RING_EVENT_BEAT) */
    float value;     /* optional payload (beat number, tempo, …) */
} RingEvent;

/* Event type constants */
#define RING_EVENT_BEAT 1

/* -------------------------------------------------------------------------
 * RingBuffer — the ring buffer state.
 * ---------------------------------------------------------------------- */
typedef struct {
    RingEvent      slots[RINGBUF_CAPACITY];
    _Atomic int    write_pos;   /* next slot to write (producer-owned) */
    _Atomic int    read_pos;    /* next slot to read  (consumer-owned) */
} RingBuffer;

/* -------------------------------------------------------------------------
 * ringbuf_init — zero-initialise the ring buffer.
 *
 * @param rb  pointer to the RingBuffer to initialise
 * ---------------------------------------------------------------------- */
void ringbuf_init(RingBuffer *rb);

/* -------------------------------------------------------------------------
 * ringbuf_push — producer side: write one event.
 * Called from the audio thread — must never block.
 *
 * @param rb     the ring buffer
 * @param event  the event to write
 * @return       1 if the event was written, 0 if the buffer was full
 * ---------------------------------------------------------------------- */
int ringbuf_push(RingBuffer *rb, RingEvent event);

/* -------------------------------------------------------------------------
 * ringbuf_pop — consumer side: read one event.
 * Called from the main thread.
 *
 * @param rb     the ring buffer
 * @param event  out-parameter: filled with the event if one was available
 * @return       1 if an event was read, 0 if the buffer was empty
 * ---------------------------------------------------------------------- */
int ringbuf_pop(RingBuffer *rb, RingEvent *event);

#endif /* LUASYNTH_RINGBUFFER_H */
