/**
 * lua_noise.c — Lua bindings for the Noise module
 */

#include <lua.h>
#include <lauxlib.h>
#include "lua_noise.h"
#include "lua_engine.h"
#include "../modules/noise.h"

static Module *noise_check(lua_State *L, int idx) {
    return *(Module **)luaL_checkudata(L, idx, LUA_NOISE_MT);
}

static int lua_noise_new(lua_State *L) {
    Module *mod = noise_create();
    if (!mod) return luaL_error(L, "Noise.new: out of memory");

    LuaEngine *eng = lua_engine_get(L);
    signal_graph_add(eng->graph, mod);

    Module **ud = (Module **)lua_newuserdata(L, sizeof(Module *));
    *ud = mod;
    luaL_getmetatable(L, LUA_NOISE_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_noise_set_amplitude(lua_State *L) {
    Module    *mod = noise_check(L, 1);
    float      amp = (float)luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    noise_set_amplitude(mod, amp);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_noise_connect(lua_State *L) {
    Module    *dst    = noise_check(L, 1);
    Module   **src_ud = (Module **)lua_touserdata(L, 2);
    if (!src_ud) return luaL_error(L, "noise:connect: expected a module");
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    module_connect(dst, *src_ud);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_noise_gc(lua_State *L) {
    Module **ud = (Module **)lua_touserdata(L, 1);
    if (ud && *ud && !(*ud)->in_graph) { module_free(*ud); *ud = NULL; }
    return 0;
}

void lua_noise_register(lua_State *L) {
    luaL_newmetatable(L, LUA_NOISE_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    static const luaL_Reg methods[] = {
        {"setAmplitude", lua_noise_set_amplitude},
        {"connect",      lua_noise_connect},
        {"__gc",         lua_noise_gc},
        {NULL, NULL}
    };
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);

    static const luaL_Reg lib[] = { {"new", lua_noise_new}, {NULL, NULL} };
    luaL_newlib(L, lib);
    lua_setglobal(L, "Noise");
}
