/**
 * audio_engine.h — SDL2 Audio Device Wrapper
 *
 * The audio engine owns the SDL2 audio device and connects it to the signal
 * graph. It registers an SDL audio callback that fires on a dedicated SDL
 * audio thread whenever the device's playback buffer needs refilling.
 *
 * Key design points:
 *
 *   - The callback runs on a SEPARATE THREAD from main()/Lua.
 *     All parameter changes from Lua must go through SDL_LockAudioDevice /
 *     SDL_UnlockAudioDevice (or C11 atomics for single-word flags).
 *
 *   - Audio format: IEEE 32-bit float, stereo (2 channels), interleaved L/R.
 *
 *   - Buffer size: 512 frames by default (≈ 11.6 ms at 44100 Hz).
 *     Lower = less latency, higher CPU pressure; higher = more latency, lower
 *     CPU pressure.
 */

#ifndef LUASYNTH_AUDIO_ENGINE_H
#define LUASYNTH_AUDIO_ENGINE_H

#include <SDL.h>
#include "signal_graph.h"

/* Desired audio configuration — the actual spec may differ slightly */
#define AUDIO_SAMPLE_RATE   44100
#define AUDIO_CHANNELS      2
#define AUDIO_BUFFER_FRAMES 512

/* -------------------------------------------------------------------------
 * AudioEngine — one instance per process, wraps the SDL2 device.
 * ---------------------------------------------------------------------- */
typedef struct {
    SDL_AudioDeviceID  device_id;     /* SDL device handle */
    SDL_AudioSpec      spec;          /* actual spec negotiated by SDL */
    SignalGraph       *graph;         /* pointer to the signal graph */
    int                sample_rate;   /* actual sample rate in use */
    int                buffer_frames; /* actual buffer size in frames */
    int                running;       /* 1 = device is open and playing */
} AudioEngine;

/* -------------------------------------------------------------------------
 * audio_engine_init — open the SDL2 audio device and start playback.
 *
 * @param engine  pointer to a caller-allocated AudioEngine (will be filled)
 * @param graph   the signal graph to pull samples from
 * @return        0 on success, -1 on SDL error (message printed to stderr)
 * ---------------------------------------------------------------------- */
int audio_engine_init(AudioEngine *engine, SignalGraph *graph);

/* -------------------------------------------------------------------------
 * audio_engine_shutdown — stop playback and close the device.
 *
 * @param engine  the engine to shut down
 * ---------------------------------------------------------------------- */
void audio_engine_shutdown(AudioEngine *engine);

/* -------------------------------------------------------------------------
 * audio_engine_lock / audio_engine_unlock — pause the audio callback so
 * parameters can be safely mutated from the main thread.
 *
 * Usage:
 *   audio_engine_lock(&engine);
 *   osc_data->frequency = 880.0;   // safe now
 *   audio_engine_unlock(&engine);
 *
 * @param engine  the engine whose device to lock/unlock
 * ---------------------------------------------------------------------- */
void audio_engine_lock(AudioEngine *engine);
void audio_engine_unlock(AudioEngine *engine);

#endif /* LUASYNTH_AUDIO_ENGINE_H */
