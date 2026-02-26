/**
 * ringbuffer.c — Lock-Free SPSC Ring Buffer implementation
 *
 * See ringbuffer.h for the design overview.
 */

#include <stdio.h>
#include <string.h>
#include "ringbuffer.h"

/* -------------------------------------------------------------------------
 * ringbuf_init
 * ---------------------------------------------------------------------- */
void ringbuf_init(RingBuffer *rb) {
    memset(rb->slots, 0, sizeof(rb->slots));
    atomic_store(&rb->write_pos, 0);
    atomic_store(&rb->read_pos,  0);
}

/* -------------------------------------------------------------------------
 * ringbuf_push — write one event (producer side).
 *
 * The producer checks whether there is a free slot:
 *   free slots = CAPACITY - (write_pos - read_pos)
 * If none are free, the event is dropped.
 *
 * Atomic ordering:
 *   - We acquire read_pos to get the consumer's latest progress.
 *   - We release write_pos after writing, so the consumer sees the new data.
 * ---------------------------------------------------------------------- */
int ringbuf_push(RingBuffer *rb, RingEvent event) {
    int w = atomic_load_explicit(&rb->write_pos, memory_order_relaxed);
    int r = atomic_load_explicit(&rb->read_pos,  memory_order_acquire);

    if ((w - r) >= RINGBUF_CAPACITY) {
        /* Buffer full — drop the event */
        return 0;
    }

    rb->slots[w % RINGBUF_CAPACITY] = event;

    /* Release the new write_pos so the consumer sees the updated slot */
    atomic_store_explicit(&rb->write_pos, w + 1, memory_order_release);
    return 1;
}

/* -------------------------------------------------------------------------
 * ringbuf_pop — read one event (consumer side).
 *
 * The consumer checks whether there is a populated slot:
 *   available = write_pos - read_pos
 * If none, returns 0 immediately (non-blocking).
 * ---------------------------------------------------------------------- */
int ringbuf_pop(RingBuffer *rb, RingEvent *event) {
    int r = atomic_load_explicit(&rb->read_pos,  memory_order_relaxed);
    int w = atomic_load_explicit(&rb->write_pos, memory_order_acquire);

    if (r == w) {
        /* Buffer empty */
        return 0;
    }

    *event = rb->slots[r % RINGBUF_CAPACITY];

    /* Release the new read_pos so the producer sees the freed slot */
    atomic_store_explicit(&rb->read_pos, r + 1, memory_order_release);
    return 1;
}
