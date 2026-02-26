/**
 * lua_engine.h — Lua State and Event Loop
 *
 * The Lua engine owns the global lua_State and provides:
 *   1. Initialisation: create the Lua state, load all LuaSynth libraries.
 *   2. Script execution: run a Lua file.
 *   3. Event loop: poll the ring buffer for clock events and dispatch them
 *      to registered Lua callbacks.
 *   4. Shutdown: close the Lua state.
 *
 * The ring buffer event loop runs on the main thread (inside the SDL event
 * loop in main.c). This is safe because Lua's VM is not thread-safe — all
 * Lua calls must happen on the same thread that created the lua_State.
 */

#ifndef LUASYNTH_LUA_ENGINE_H
#define LUASYNTH_LUA_ENGINE_H

#include <lua.h>
#include "../signal_graph.h"
#include "../audio_engine.h"
#include "../util/ringbuffer.h"

/* -------------------------------------------------------------------------
 * LuaEngine — the Lua runtime context.
 * ---------------------------------------------------------------------- */
typedef struct {
    lua_State    *L;
    SignalGraph  *graph;
    AudioEngine  *audio;
    RingBuffer   *ring;

    /* Registry key for the clock beat callback function */
    int           beat_callback_ref;  /* LUA_NOREF if not set */
} LuaEngine;

/* -------------------------------------------------------------------------
 * lua_engine_init — create the Lua state and register all LuaSynth modules.
 *
 * @param engine  pointer to a caller-allocated LuaEngine (will be filled)
 * @param graph   the signal graph (modules will be registered here)
 * @param audio   the audio engine (for lock wrappers)
 * @param ring    the ring buffer (for clock events)
 * @return        0 on success, -1 on failure
 * ---------------------------------------------------------------------- */
int lua_engine_init(LuaEngine *engine, SignalGraph *graph, AudioEngine *audio, RingBuffer *ring);

/* -------------------------------------------------------------------------
 * lua_engine_run_file — load and execute a Lua script file.
 *
 * @param engine  the engine
 * @param path    path to the .lua file
 * @return        0 on success, -1 on Lua error (message printed to stderr)
 * ---------------------------------------------------------------------- */
int lua_engine_run_file(LuaEngine *engine, const char *path);

/* -------------------------------------------------------------------------
 * lua_engine_poll — drain the ring buffer and call registered Lua callbacks.
 * Call this repeatedly from the main event loop (typically in a SDL_Delay loop).
 *
 * @param engine  the engine
 * ---------------------------------------------------------------------- */
void lua_engine_poll(LuaEngine *engine);

/* -------------------------------------------------------------------------
 * lua_engine_shutdown — close the Lua state.
 *
 * @param engine  the engine
 * ---------------------------------------------------------------------- */
void lua_engine_shutdown(LuaEngine *engine);

/* -------------------------------------------------------------------------
 * lua_engine_get — retrieve the LuaEngine pointer stored in the Lua registry.
 * Used by Lua binding functions to access the engine without a global variable.
 *
 * @param L  the Lua state
 * @return   the LuaEngine pointer
 * ---------------------------------------------------------------------- */
LuaEngine *lua_engine_get(lua_State *L);

#endif /* LUASYNTH_LUA_ENGINE_H */
