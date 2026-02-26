/**
 * lua_filter.c — Lua bindings for the Filter module
 */

#include <lua.h>
#include <lauxlib.h>
#include "lua_filter.h"
#include "lua_engine.h"
#include "../modules/filter.h"

static Module *filter_check(lua_State *L, int idx) {
    return *(Module **)luaL_checkudata(L, idx, LUA_FILTER_MT);
}

static int lua_filter_new(lua_State *L) {
    const char *type_str = luaL_optstring(L, 1, "svf");
    FilterType type = filter_type_from_string(type_str);
    Module *mod = filter_create(type);
    if (!mod) return luaL_error(L, "Filter.new: out of memory");

    LuaEngine *eng = lua_engine_get(L);
    signal_graph_add(eng->graph, mod);

    Module **ud = (Module **)lua_newuserdata(L, sizeof(Module *));
    *ud = mod;
    luaL_getmetatable(L, LUA_FILTER_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_filter_set_input(lua_State *L) {
    Module    *dst    = filter_check(L, 1);
    Module   **src_ud = (Module **)lua_touserdata(L, 2);
    if (!src_ud) return luaL_error(L, "filter:setInput: expected a module");
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    /* Clear existing inputs and set this one */
    dst->num_inputs = 0;
    module_connect(dst, *src_ud);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_filter_set_cutoff(lua_State *L) {
    Module    *mod = filter_check(L, 1);
    float      hz  = (float)luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    filter_set_cutoff(mod, hz);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_filter_set_resonance(lua_State *L) {
    Module    *mod = filter_check(L, 1);
    float      res = (float)luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    filter_set_resonance(mod, res);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_filter_set_mode(lua_State *L) {
    Module     *mod     = filter_check(L, 1);
    const char *mode_s  = luaL_checkstring(L, 2);
    LuaEngine  *eng     = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    filter_set_mode(mod, filter_mode_from_string(mode_s));
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_filter_connect_mod(lua_State *L) {
    /* Wire the LFO/source (arg2) as a modulator on filter (arg1) */
    Module    *dst    = filter_check(L, 1);
    Module   **src_ud = (Module **)lua_touserdata(L, 2);
    if (!src_ud) return luaL_error(L, "filter:connectMod: expected a module");
    const char *param = luaL_checkstring(L, 3);
    float depth = (float)luaL_optnumber(L, 4, 1.0);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    module_connect_mod(dst, *src_ud, param, depth);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_filter_gc(lua_State *L) {
    Module **ud = (Module **)lua_touserdata(L, 1);
    if (ud && *ud && !(*ud)->in_graph) { module_free(*ud); *ud = NULL; }
    return 0;
}

void lua_filter_register(lua_State *L) {
    luaL_newmetatable(L, LUA_FILTER_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    static const luaL_Reg methods[] = {
        {"setInput",     lua_filter_set_input},
        {"setCutoff",    lua_filter_set_cutoff},
        {"setResonance", lua_filter_set_resonance},
        {"setMode",      lua_filter_set_mode},
        {"connectMod",   lua_filter_connect_mod},
        {"__gc",         lua_filter_gc},
        {NULL, NULL}
    };
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);

    static const luaL_Reg lib[] = { {"new", lua_filter_new}, {NULL, NULL} };
    luaL_newlib(L, lib);
    lua_setglobal(L, "Filter");
}
