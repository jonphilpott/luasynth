/**
 * lua_delay.c — Lua bindings for the Delay module
 */

#include <lua.h>
#include <lauxlib.h>
#include "lua_delay.h"
#include "lua_engine.h"
#include "../modules/delay.h"

static Module *delay_check(lua_State *L, int idx) {
    return *(Module **)luaL_checkudata(L, idx, LUA_DELAY_MT);
}

static int lua_delay_new(lua_State *L) {
    float  delay_time = (float)luaL_optnumber(L, 1, 0.3);
    float  feedback   = (float)luaL_optnumber(L, 2, 0.5);
    LuaEngine *eng    = lua_engine_get(L);

    Module *mod = delay_create(delay_time, feedback, eng->audio->sample_rate);
    if (!mod) return luaL_error(L, "Delay.new: out of memory");

    signal_graph_add(eng->graph, mod);

    Module **ud = (Module **)lua_newuserdata(L, sizeof(Module *));
    *ud = mod;
    luaL_getmetatable(L, LUA_DELAY_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_delay_set_input(lua_State *L) {
    Module    *dst    = delay_check(L, 1);
    Module   **src_ud = (Module **)lua_touserdata(L, 2);
    if (!src_ud) return luaL_error(L, "delay:setInput: expected a module");
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    dst->num_inputs = 0;
    module_connect(dst, *src_ud);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_delay_set_delay_time(lua_State *L) {
    Module    *mod = delay_check(L, 1);
    float      v   = (float)luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    delay_set_delay_time(mod, v);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_delay_set_feedback(lua_State *L) {
    Module    *mod = delay_check(L, 1);
    float      v   = (float)luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    delay_set_feedback(mod, v);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_delay_set_wet(lua_State *L) {
    Module    *mod = delay_check(L, 1);
    float      v   = (float)luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    delay_set_wet(mod, v);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_delay_set_dry(lua_State *L) {
    Module    *mod = delay_check(L, 1);
    float      v   = (float)luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    delay_set_dry(mod, v);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_delay_gc(lua_State *L) {
    Module **ud = (Module **)lua_touserdata(L, 1);
    if (ud && *ud && !(*ud)->in_graph) { module_free(*ud); *ud = NULL; }
    return 0;
}

void lua_delay_register(lua_State *L) {
    luaL_newmetatable(L, LUA_DELAY_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    static const luaL_Reg methods[] = {
        {"setInput",      lua_delay_set_input},
        {"setDelayTime",  lua_delay_set_delay_time},
        {"setFeedback",   lua_delay_set_feedback},
        {"setWet",        lua_delay_set_wet},
        {"setDry",        lua_delay_set_dry},
        {"__gc",          lua_delay_gc},
        {NULL, NULL}
    };
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);

    static const luaL_Reg lib[] = { {"new", lua_delay_new}, {NULL, NULL} };
    luaL_newlib(L, lib);
    lua_setglobal(L, "Delay");
}
