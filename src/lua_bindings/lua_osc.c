/**
 * lua_osc.c — Lua bindings for the Oscillator module
 *
 * This file is the core lesson in Lua/C integration. Read it carefully.
 *
 * == Full Userdata Pattern ==
 *
 * We represent a C Module* as a Lua "full userdata" — a Lua-managed block of
 * memory that holds a Module* pointer. Lua treats it like any other value,
 * garbage-collects it, and we can attach a metatable to give it methods.
 *
 * Step-by-step how Osc.new() works:
 *   1. C allocates an OscModule struct (osc_create).
 *   2. lua_newuserdata allocates a sizeof(Module*) block inside Lua's heap.
 *   3. We write the Module* pointer into that block.
 *   4. We attach the "LuaSynth.Osc" metatable to the userdata.
 *   5. The metatable has __index = metatable, so method calls like osc:setFrequency()
 *      look up the function in the metatable.
 *   6. The metatable has __gc = osc_gc, which frees the module when Lua
 *      garbage-collects the userdata — unless mod->in_graph is set.
 *
 * == Thread Safety ==
 *
 * Setters that change audio parameters call audio_engine_lock/unlock around
 * the parameter write. This pauses the SDL audio callback while we modify data
 * that the callback reads.
 */

#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include "lua_osc.h"
#include "lua_engine.h"
#include "../modules/osc.h"

/* -------------------------------------------------------------------------
 * osc_check — retrieve the Module* from a Lua userdata, with type checking.
 * Raises a Lua error if the value at stack index `idx` is not an Osc.
 * ---------------------------------------------------------------------- */
static Module *osc_check(lua_State *L, int idx) {
    /* luaL_checkudata raises an error if the metatable doesn't match */
    Module **ud = (Module **)luaL_checkudata(L, idx, LUA_OSC_MT);
    return *ud;
}

/* -------------------------------------------------------------------------
 * lua_osc_new — Osc.new(waveform_string, frequency) → userdata
 *
 * @param L[1]  waveform name: "sine", "square", "saw", "triangle"
 * @param L[2]  frequency in Hz (number)
 * @return      1 value on stack: the new Osc userdata
 * ---------------------------------------------------------------------- */
static int lua_osc_new(lua_State *L) {
    const char *wave_str = luaL_optstring(L, 1, "sine");
    double freq          = luaL_optnumber(L, 2, 440.0);

    OscWaveform waveform = osc_waveform_from_string(wave_str);

    /* Allocate the C module */
    Module *mod = osc_create(waveform, freq);
    if (!mod) {
        return luaL_error(L, "Osc.new: out of memory");
    }

    /* Register with the signal graph so it stays alive and in_graph is set */
    LuaEngine *engine = lua_engine_get(L);
    signal_graph_add(engine->graph, mod);

    /* Create the Lua userdata: a pointer-sized block on the Lua heap */
    Module **ud = (Module **)lua_newuserdata(L, sizeof(Module *));
    *ud = mod;

    /* Attach the metatable to enable method calls */
    luaL_getmetatable(L, LUA_OSC_MT);
    lua_setmetatable(L, -2);

    return 1;  /* return the userdata */
}

/* -------------------------------------------------------------------------
 * Method: osc:setFrequency(hz)
 * ---------------------------------------------------------------------- */
static int lua_osc_set_frequency(lua_State *L) {
    Module *mod    = osc_check(L, 1);
    double  freq   = luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);

    /* Lock the audio device while modifying a parameter the callback reads */
    audio_engine_lock(eng->audio);
    osc_set_frequency(mod, freq);
    audio_engine_unlock(eng->audio);
    return 0;
}

/* -------------------------------------------------------------------------
 * Method: osc:setAmplitude(amp)
 * ---------------------------------------------------------------------- */
static int lua_osc_set_amplitude(lua_State *L) {
    Module *mod   = osc_check(L, 1);
    double  amp   = luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    osc_set_amplitude(mod, amp);
    audio_engine_unlock(eng->audio);
    return 0;
}

/* -------------------------------------------------------------------------
 * Method: osc:setWaveform(name)
 * ---------------------------------------------------------------------- */
static int lua_osc_set_waveform(lua_State *L) {
    Module     *mod  = osc_check(L, 1);
    const char *s    = luaL_checkstring(L, 2);
    LuaEngine  *eng  = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    osc_set_waveform(mod, osc_waveform_from_string(s));
    audio_engine_unlock(eng->audio);
    return 0;
}

/* -------------------------------------------------------------------------
 * Method: osc:connect(other)
 * Wire `other` as an audio input to this oscillator (additive mixing).
 * ---------------------------------------------------------------------- */
static int lua_osc_connect(lua_State *L) {
    Module *dst = osc_check(L, 1);
    /* The second argument can be any module type — peek at its userdata */
    Module **src_ud = (Module **)lua_touserdata(L, 2);
    if (!src_ud) return luaL_error(L, "osc:connect: expected a module");
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    module_connect(dst, *src_ud);
    audio_engine_unlock(eng->audio);
    return 0;
}

/* -------------------------------------------------------------------------
 * Method: osc:connectMod(target, param, depth)
 * Wire this oscillator as a modulator on a named parameter of `target`.
 * ---------------------------------------------------------------------- */
static int lua_osc_connect_mod(lua_State *L) {
    Module     *self   = osc_check(L, 1);
    Module    **dst_ud = (Module **)lua_touserdata(L, 2);
    if (!dst_ud) return luaL_error(L, "osc:connectMod: expected a module");
    const char *param  = luaL_checkstring(L, 3);
    float       depth  = (float)luaL_optnumber(L, 4, 1.0);
    LuaEngine  *eng    = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    module_connect_mod(*dst_ud, self, param, depth);
    audio_engine_unlock(eng->audio);
    return 0;
}

/* -------------------------------------------------------------------------
 * __gc metamethod — called when Lua garbage-collects the userdata.
 * Only frees the module if it is NOT owned by the signal graph.
 * ---------------------------------------------------------------------- */
static int lua_osc_gc(lua_State *L) {
    Module **ud = (Module **)lua_touserdata(L, 1);
    if (ud && *ud && !(*ud)->in_graph) {
        module_free(*ud);
        *ud = NULL;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * __tostring metamethod — for debug printing.
 * ---------------------------------------------------------------------- */
static int lua_osc_tostring(lua_State *L) {
    Module *mod = osc_check(L, 1);
    OscData *d  = (OscData *)mod->data;
    lua_pushfstring(L, "Osc(%.1fHz)", d->frequency);
    return 1;
}

/* -------------------------------------------------------------------------
 * lua_osc_register — set up the Osc global table and metatable.
 * ---------------------------------------------------------------------- */
void lua_osc_register(lua_State *L) {
    /* Create the metatable that userdata objects inherit from */
    luaL_newmetatable(L, LUA_OSC_MT);

    /* __index = metatable itself (standard OOP trick: method calls search the mt) */
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    /* Register methods in the metatable */
    static const luaL_Reg methods[] = {
        {"setFrequency",  lua_osc_set_frequency},
        {"setAmplitude",  lua_osc_set_amplitude},
        {"setWaveform",   lua_osc_set_waveform},
        {"connect",       lua_osc_connect},
        {"connectMod",    lua_osc_connect_mod},
        {"__gc",          lua_osc_gc},
        {"__tostring",    lua_osc_tostring},
        {NULL, NULL}
    };
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);  /* pop the metatable */

    /* Create the global "Osc" table with the constructor */
    static const luaL_Reg lib[] = {
        {"new", lua_osc_new},
        {NULL, NULL}
    };
    luaL_newlib(L, lib);
    lua_setglobal(L, "Osc");
}
