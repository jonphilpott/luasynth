/**
 * audio_engine.c — SDL2 Audio Device implementation
 *
 * See audio_engine.h for the design overview.
 */

#include <stdio.h>
#include <string.h>
#include "audio_engine.h"

/* -------------------------------------------------------------------------
 * sdl_audio_callback — called by SDL on its audio thread.
 *
 * SDL passes us a raw byte buffer `stream` of length `len`. Because we
 * requested SDL_AUDIO_F32 the buffer holds 32-bit floats, two per frame
 * (left then right).
 *
 * Step 1: Calculate how many audio frames fit in the byte buffer.
 * Step 2: Ask the signal graph to fill those frames.
 * Step 3: SDL_MixAudioFormat is NOT used — we write directly to stream.
 *         (Mixing would require a separate source buffer and is unnecessary
 *          here since the graph handles all mixing internally.)
 *
 * @param userdata  pointer to our AudioEngine (passed via SDL_OpenAudioDevice)
 * @param stream    byte buffer to fill
 * @param len       byte length of stream
 * ---------------------------------------------------------------------- */
static void sdl_audio_callback(void *userdata, Uint8 *stream, int len) {
    AudioEngine *engine = (AudioEngine *)userdata;

    /* Step 1: Convert byte length to number of audio frames.
     * Each frame = 2 channels × 4 bytes (float32) = 8 bytes/frame. */
    int bytes_per_frame = AUDIO_CHANNELS * (int)sizeof(float);
    int num_frames      = len / bytes_per_frame;

    /* Step 2: Pull samples from the signal graph into the SDL buffer.
     * We cast the byte pointer to float* — safe because we requested
     * SDL_AUDIO_F32 (which guarantees float-aligned storage). */
    signal_graph_process(engine->graph, (float *)stream, num_frames,
                         engine->sample_rate);
}

/* -------------------------------------------------------------------------
 * audio_engine_init
 * ---------------------------------------------------------------------- */
int audio_engine_init(AudioEngine *engine, SignalGraph *graph) {
    memset(engine, 0, sizeof(AudioEngine));
    engine->graph = graph;

    /* Describe what audio format we want from SDL.
     * freq      = sample rate (samples per second per channel)
     * format    = SDL_AUDIO_F32 = IEEE 754 32-bit float (native endian)
     * channels  = 2 (stereo)
     * samples   = buffer size in frames; SDL may round to a power of two */
    SDL_AudioSpec desired;
    memset(&desired, 0, sizeof(desired));
    desired.freq     = AUDIO_SAMPLE_RATE;
    desired.format   = AUDIO_F32;
    desired.channels = AUDIO_CHANNELS;
    desired.samples  = AUDIO_BUFFER_FRAMES;
    desired.callback = sdl_audio_callback;
    desired.userdata = engine;

    /* Open the default audio device.
     * Passing NULL for device name = use the system default.
     * The last argument (allowed_changes) = 0 means SDL must give us exactly
     * what we asked for; if the hardware can't support it, SDL will resample. */
    engine->device_id = SDL_OpenAudioDevice(NULL, 0, &desired, &engine->spec, 0);
    if (engine->device_id == 0) {
        fprintf(stderr, "audio_engine: SDL_OpenAudioDevice failed: %s\n",
                SDL_GetError());
        return -1;
    }

    engine->sample_rate   = engine->spec.freq;
    engine->buffer_frames = engine->spec.samples;
    engine->running       = 1;

    printf("audio_engine: opened device %u — %d Hz, %d ch, %d frames/buf\n",
           engine->device_id, engine->sample_rate,
           engine->spec.channels, engine->buffer_frames);

    /* Start playback (SDL devices start paused by default) */
    SDL_PauseAudioDevice(engine->device_id, 0);
    return 0;
}

/* -------------------------------------------------------------------------
 * audio_engine_shutdown
 * ---------------------------------------------------------------------- */
void audio_engine_shutdown(AudioEngine *engine) {
    if (!engine->running) return;
    SDL_PauseAudioDevice(engine->device_id, 1);   /* pause first */
    SDL_CloseAudioDevice(engine->device_id);
    engine->running   = 0;
    engine->device_id = 0;
}

/* -------------------------------------------------------------------------
 * audio_engine_lock / audio_engine_unlock
 * ---------------------------------------------------------------------- */
void audio_engine_lock(AudioEngine *engine) {
    SDL_LockAudioDevice(engine->device_id);
}

void audio_engine_unlock(AudioEngine *engine) {
    SDL_UnlockAudioDevice(engine->device_id);
}
