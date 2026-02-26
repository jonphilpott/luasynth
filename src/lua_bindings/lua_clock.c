/**
 * lua_clock.c — Lua bindings for the Clock module
 *
 * The onBeat() callback pattern is especially interesting:
 *   1. Lua provides a function: clk:onBeat(fn)
 *   2. We store `fn` in the Lua registry using luaL_ref() → integer key
 *   3. We save the key in engine->beat_callback_ref
 *   4. lua_engine_poll() (main thread) retrieves it and calls it each beat
 *
 * This is the standard pattern for storing Lua callbacks in C.
 */

#include <lua.h>
#include <lauxlib.h>
#include "lua_clock.h"
#include "lua_engine.h"
#include "../modules/clock.h"

static Module *clock_check(lua_State *L, int idx) {
    return *(Module **)luaL_checkudata(L, idx, LUA_CLOCK_MT);
}

static int lua_clock_new(lua_State *L) {
    double bpm = luaL_optnumber(L, 1, 120.0);
    LuaEngine *eng = lua_engine_get(L);

    Module *mod = clock_create(bpm, eng->ring);
    if (!mod) return luaL_error(L, "Clock.new: out of memory");

    signal_graph_add(eng->graph, mod);

    /* The clock must be processed even if it's not connected to the output.
     * We do this by registering it as an input of a special "always-process"
     * slot — but the simpler approach is to rely on main.c calling
     * signal_graph_process which will call output->process, and the clock's
     * process() is called separately if the user adds it as an input.
     * For now, the user is expected to not connect the clock as audio output;
     * it fires events via the ring buffer regardless. We still need it
     * processed each frame: the lua_engine_poll loop drains events.
     * The clock processes itself because it's added to the graph and the
     * graph calls output->process → but the clock is not in the audio chain.
     * Solution: we store the clock module in the engine and call process()
     * on it from main.c's event loop. For simplicity, we wire clock as
     * an audio input to the output module during start(). */

    Module **ud = (Module **)lua_newuserdata(L, sizeof(Module *));
    *ud = mod;
    luaL_getmetatable(L, LUA_CLOCK_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_clock_start(lua_State *L) {
    Module    *mod = clock_check(L, 1);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    clock_start(mod);
    /* Wire the clock as an input to the output module so it gets processed.
     * The clock outputs silence, so it doesn't affect audio quality. */
    if (eng->graph->output_module) {
        module_connect(eng->graph->output_module, mod);
    }
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_clock_stop(lua_State *L) {
    Module    *mod = clock_check(L, 1);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    clock_stop(mod);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_clock_set_bpm(lua_State *L) {
    Module    *mod = clock_check(L, 1);
    double     bpm = luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    clock_set_bpm(mod, bpm);
    audio_engine_unlock(eng->audio);
    return 0;
}

/* -------------------------------------------------------------------------
 * clk:onBeat(fn) — register a Lua function as the beat callback.
 *
 * We use luaL_ref() to store the function in the Lua registry. This creates
 * an integer key that we can later use to retrieve and call the function.
 * ---------------------------------------------------------------------- */
static int lua_clock_on_beat(lua_State *L) {
    clock_check(L, 1);   /* just validate the clock is valid */
    luaL_checktype(L, 2, LUA_TFUNCTION);

    LuaEngine *eng = lua_engine_get(L);

    /* Unref any previous callback */
    if (eng->beat_callback_ref != LUA_NOREF) {
        luaL_unref(L, LUA_REGISTRYINDEX, eng->beat_callback_ref);
    }

    /* Push the function and store it in the registry.
     * luaL_ref pops the value and returns an integer key. */
    lua_pushvalue(L, 2);
    eng->beat_callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    return 0;
}

static int lua_clock_gc(lua_State *L) {
    Module **ud = (Module **)lua_touserdata(L, 1);
    if (ud && *ud && !(*ud)->in_graph) { module_free(*ud); *ud = NULL; }
    return 0;
}

void lua_clock_register(lua_State *L) {
    luaL_newmetatable(L, LUA_CLOCK_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    static const luaL_Reg methods[] = {
        {"start",   lua_clock_start},
        {"stop",    lua_clock_stop},
        {"setBPM",  lua_clock_set_bpm},
        {"onBeat",  lua_clock_on_beat},
        {"__gc",    lua_clock_gc},
        {NULL, NULL}
    };
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);

    static const luaL_Reg lib[] = { {"new", lua_clock_new}, {NULL, NULL} };
    luaL_newlib(L, lib);
    lua_setglobal(L, "Clock");
}
