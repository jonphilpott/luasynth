/**
 * lua_engine.c — Lua State and Event Loop implementation
 *
 * See lua_engine.h for the design overview.
 *
 * Key pattern — the LuaEngine pointer is stored in the Lua registry:
 *   lua_pushlightuserdata(L, &engine_key);  // unique key
 *   lua_pushlightuserdata(L, engine);        // value
 *   lua_settable(L, LUA_REGISTRYINDEX);
 *
 * Any binding function can retrieve it:
 *   lua_engine_get(L) → LuaEngine*
 *
 * This avoids global variables while still giving every binding function
 * access to the audio engine, signal graph, and ring buffer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "lua_engine.h"
#include "lua_osc.h"
#include "lua_noise.h"
#include "lua_filter.h"
#include "lua_lfo.h"
#include "lua_envelope.h"
#include "lua_delay.h"
#include "lua_clock.h"
#include "lua_midi.h"
#include "lua_output.h"

/* A static address used as a unique key in the Lua registry */
static char engine_registry_key;

/* -------------------------------------------------------------------------
 * lua_engine_get
 * ---------------------------------------------------------------------- */
LuaEngine *lua_engine_get(lua_State *L) {
    lua_pushlightuserdata(L, &engine_registry_key);
    lua_gettable(L, LUA_REGISTRYINDEX);
    LuaEngine *engine = (LuaEngine *)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return engine;
}

/* -------------------------------------------------------------------------
 * lua_sdl_delay — Lua binding: SDL.delay(ms)
 * Allows Lua scripts to sleep without blocking (useful in startup scripts).
 * ---------------------------------------------------------------------- */
static int lua_sdl_delay(lua_State *L) {
    int ms = (int)luaL_checkinteger(L, 1);
    SDL_Delay((Uint32)ms);
    return 0;
}

/* -------------------------------------------------------------------------
 * lua_engine_init
 * ---------------------------------------------------------------------- */
int lua_engine_init(LuaEngine *engine, SignalGraph *graph, AudioEngine *audio, RingBuffer *ring) {
    engine->graph             = graph;
    engine->audio             = audio;
    engine->ring              = ring;
    engine->beat_callback_ref = LUA_NOREF;

    /* Create the Lua state */
    engine->L = luaL_newstate();
    if (!engine->L) {
        fprintf(stderr, "lua_engine: failed to create Lua state\n");
        return -1;
    }

    /* Load standard Lua libraries (print, math, string, table, …) */
    luaL_openlibs(engine->L);

    /* Store the engine pointer in the Lua registry so bindings can find it */
    lua_pushlightuserdata(engine->L, &engine_registry_key);
    lua_pushlightuserdata(engine->L, engine);
    lua_settable(engine->L, LUA_REGISTRYINDEX);

    /* Register all LuaSynth module libraries */
    lua_osc_register(engine->L);
    lua_noise_register(engine->L);
    lua_filter_register(engine->L);
    lua_lfo_register(engine->L);
    lua_envelope_register(engine->L);
    lua_delay_register(engine->L);
    lua_clock_register(engine->L);
    lua_midi_register(engine->L);
    lua_output_register(engine->L);

    /* Register SDL table with utility functions */
    lua_newtable(engine->L);
    lua_pushcfunction(engine->L, lua_sdl_delay);
    lua_setfield(engine->L, -2, "delay");
    lua_setglobal(engine->L, "SDL");

    printf("lua_engine: Lua %s initialised\n", LUA_VERSION);
    return 0;
}

/* -------------------------------------------------------------------------
 * lua_engine_run_file
 * ---------------------------------------------------------------------- */
int lua_engine_run_file(LuaEngine *engine, const char *path) {
    if (luaL_dofile(engine->L, path) != LUA_OK) {
        fprintf(stderr, "lua_engine: error running '%s': %s\n",
                path, lua_tostring(engine->L, -1));
        lua_pop(engine->L, 1);
        return -1;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * lua_engine_poll — drain beat events and call Lua callbacks.
 *
 * The ring buffer is filled by the clock module on the audio thread.
 * We drain it here on the main thread and call the registered Lua function.
 *
 * The callback is stored in the Lua registry under `beat_callback_ref`
 * (set by lua_clock.c when the user calls clk:onBeat(fn)).
 * ---------------------------------------------------------------------- */
void lua_engine_poll(LuaEngine *engine) {
    RingEvent ev;
    while (ringbuf_pop(engine->ring, &ev)) {
        if (ev.type == RING_EVENT_BEAT && engine->beat_callback_ref != LUA_NOREF) {
            /* Retrieve the callback function from the registry */
            lua_rawgeti(engine->L, LUA_REGISTRYINDEX, engine->beat_callback_ref);

            /* Push the beat number as the argument */
            lua_pushinteger(engine->L, (lua_Integer)ev.value);

            /* Call the Lua function: 1 argument, 0 return values */
            if (lua_pcall(engine->L, 1, 0, 0) != LUA_OK) {
                fprintf(stderr, "lua_engine: beat callback error: %s\n",
                        lua_tostring(engine->L, -1));
                lua_pop(engine->L, 1);
            }
        }
    }
}

/* -------------------------------------------------------------------------
 * lua_engine_shutdown
 * ---------------------------------------------------------------------- */
void lua_engine_shutdown(LuaEngine *engine) {
    if (engine->beat_callback_ref != LUA_NOREF) {
        luaL_unref(engine->L, LUA_REGISTRYINDEX, engine->beat_callback_ref);
    }
    if (engine->L) {
        lua_close(engine->L);
        engine->L = NULL;
    }
}
