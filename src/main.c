/**
 * main.c — LuaSynth entry point
 *
 * Initialises SDL2, the signal graph, the audio engine, and the Lua engine,
 * then runs the event loop.
 *
 * Usage:
 *   luasynth <script.lua>
 *   luasynth              -- runs scripts/startup.lua by default
 *
 * Event loop design:
 *   The main loop:
 *     1. Polls SDL events (so the process can be killed with Ctrl-C / quit).
 *     2. Calls lua_engine_poll() to drain the ring buffer and fire Lua callbacks.
 *     3. Sleeps 1 ms to avoid burning 100% CPU in the main thread.
 *
 *   The SDL audio thread runs the signal graph independently in the background.
 *   The two threads communicate only through:
 *     - SDL_LockAudioDevice / SDL_UnlockAudioDevice (for parameter changes)
 *     - C11 _Atomic (for envelope trigger / clock running flags)
 *     - The SPSC ring buffer (for clock → Lua beat events)
 */

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include "audio_engine.h"
#include "signal_graph.h"
#include "lua_bindings/lua_engine.h"
#include "util/ringbuffer.h"

/* Default script path if no argument is provided */
#define DEFAULT_SCRIPT "scripts/startup.lua"

int main(int argc, char *argv[]) {
    const char *script_path = (argc > 1) ? argv[1] : DEFAULT_SCRIPT;

    printf("LuaSynth starting — script: %s\n", script_path);

    /* -----------------------------------------------------------------------
     * Step 1: Initialise SDL2.
     * SDL_INIT_AUDIO opens the audio subsystem. We don't need video here.
     * --------------------------------------------------------------------- */
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    /* -----------------------------------------------------------------------
     * Step 2: Initialise the signal graph.
     * The graph is empty at this point — Lua scripts populate it.
     * --------------------------------------------------------------------- */
    SignalGraph graph;
    signal_graph_init(&graph);

    /* -----------------------------------------------------------------------
     * Step 3: Initialise the ring buffer for clock events.
     * --------------------------------------------------------------------- */
    RingBuffer ring;
    ringbuf_init(&ring);

    /* -----------------------------------------------------------------------
     * Step 4: Open the audio device.
     * This starts the SDL audio thread. The callback will output silence
     * until Lua sets up an output module.
     * --------------------------------------------------------------------- */
    AudioEngine audio;
    if (audio_engine_init(&audio, &graph) != 0) {
        SDL_Quit();
        return 1;
    }

    /* -----------------------------------------------------------------------
     * Step 5: Initialise the Lua engine and run the script.
     * The script is expected to create modules and call Output.set().
     * After the script returns, the audio thread keeps running with whatever
     * patch the script built.
     * --------------------------------------------------------------------- */
    LuaEngine lua_engine;
    if (lua_engine_init(&lua_engine, &graph, &audio, &ring) != 0) {
        audio_engine_shutdown(&audio);
        SDL_Quit();
        return 1;
    }

    if (lua_engine_run_file(&lua_engine, script_path) != 0) {
        /* Error already printed by lua_engine_run_file */
        lua_engine_shutdown(&lua_engine);
        audio_engine_shutdown(&audio);
        signal_graph_destroy(&graph);
        SDL_Quit();
        return 1;
    }

    /* -----------------------------------------------------------------------
     * Step 6: Event loop.
     * We run until the user presses Ctrl-C (which raises SIGINT, causing
     * SDL_PollEvent to eventually return SDL_QUIT on some platforms) or until
     * SDL sends a quit event.
     * --------------------------------------------------------------------- */
    printf("LuaSynth running — press Ctrl-C to stop\n");

    int running = 1;
    SDL_Event event;

    while (running) {
        /* Poll for SDL quit event (window close, Ctrl-C on some platforms) */
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            /* SDL_KEYDOWN on Escape also quits */
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = 0;
            }
        }

        /* Drain the ring buffer and call Lua beat callbacks */
        lua_engine_poll(&lua_engine);

        /* Sleep 1 ms — gives up CPU slice so we don't spin at 100% */
        SDL_Delay(1);
    }

    /* -----------------------------------------------------------------------
     * Step 7: Shutdown in reverse order of initialisation.
     * --------------------------------------------------------------------- */
    printf("LuaSynth shutting down\n");
    lua_engine_shutdown(&lua_engine);
    audio_engine_shutdown(&audio);
    signal_graph_destroy(&graph);
    SDL_Quit();

    return 0;
}
