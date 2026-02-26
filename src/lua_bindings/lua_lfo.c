/**
 * lua_lfo.c — Lua bindings for the LFO module
 */

#include <lua.h>
#include <lauxlib.h>
#include "lua_lfo.h"
#include "lua_engine.h"
#include "../modules/lfo.h"
#include "../modules/osc.h"

static Module *lfo_check(lua_State *L, int idx) {
    return *(Module **)luaL_checkudata(L, idx, LUA_LFO_MT);
}

static int lua_lfo_new(lua_State *L) {
    const char *wave_str = luaL_optstring(L, 1, "sine");
    double freq          = luaL_optnumber(L, 2, 1.0);
    OscWaveform waveform = osc_waveform_from_string(wave_str);
    Module *mod = lfo_create(waveform, freq);
    if (!mod) return luaL_error(L, "LFO.new: out of memory");

    LuaEngine *eng = lua_engine_get(L);
    signal_graph_add(eng->graph, mod);

    Module **ud = (Module **)lua_newuserdata(L, sizeof(Module *));
    *ud = mod;
    luaL_getmetatable(L, LUA_LFO_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_lfo_set_frequency(lua_State *L) {
    Module    *mod = lfo_check(L, 1);
    double     hz  = luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    lfo_set_frequency(mod, hz);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_lfo_set_depth(lua_State *L) {
    Module    *mod   = lfo_check(L, 1);
    double     depth = luaL_checknumber(L, 2);
    LuaEngine *eng   = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    lfo_set_depth(mod, depth);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_lfo_set_waveform(lua_State *L) {
    Module     *mod = lfo_check(L, 1);
    const char *s   = luaL_checkstring(L, 2);
    LuaEngine  *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    lfo_set_waveform(mod, osc_waveform_from_string(s));
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_lfo_connect_mod(lua_State *L) {
    Module    *self   = lfo_check(L, 1);
    Module   **dst_ud = (Module **)lua_touserdata(L, 2);
    if (!dst_ud) return luaL_error(L, "lfo:connectMod: expected a module");
    const char *param = luaL_checkstring(L, 3);
    float depth = (float)luaL_optnumber(L, 4, 1.0);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    module_connect_mod(*dst_ud, self, param, depth);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_lfo_gc(lua_State *L) {
    Module **ud = (Module **)lua_touserdata(L, 1);
    if (ud && *ud && !(*ud)->in_graph) { module_free(*ud); *ud = NULL; }
    return 0;
}

void lua_lfo_register(lua_State *L) {
    luaL_newmetatable(L, LUA_LFO_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    static const luaL_Reg methods[] = {
        {"setFrequency", lua_lfo_set_frequency},
        {"setDepth",     lua_lfo_set_depth},
        {"setWaveform",  lua_lfo_set_waveform},
        {"connectMod",   lua_lfo_connect_mod},
        {"__gc",         lua_lfo_gc},
        {NULL, NULL}
    };
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);

    static const luaL_Reg lib[] = { {"new", lua_lfo_new}, {NULL, NULL} };
    luaL_newlib(L, lib);
    lua_setglobal(L, "LFO");
}
