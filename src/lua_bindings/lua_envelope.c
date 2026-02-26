/**
 * lua_envelope.c — Lua bindings for the Envelope module
 */

#include <lua.h>
#include <lauxlib.h>
#include "lua_envelope.h"
#include "lua_engine.h"
#include "../modules/envelope.h"

static Module *env_check(lua_State *L, int idx) {
    return *(Module **)luaL_checkudata(L, idx, LUA_ENVELOPE_MT);
}

static int lua_envelope_new(lua_State *L) {
    double attack  = luaL_optnumber(L, 1, 0.01);
    double decay   = luaL_optnumber(L, 2, 0.1);
    double sustain = luaL_optnumber(L, 3, 0.7);
    double release = luaL_optnumber(L, 4, 0.3);

    Module *mod = envelope_create(attack, decay, sustain, release);
    if (!mod) return luaL_error(L, "Envelope.new: out of memory");

    LuaEngine *eng = lua_engine_get(L);
    signal_graph_add(eng->graph, mod);

    Module **ud = (Module **)lua_newuserdata(L, sizeof(Module *));
    *ud = mod;
    luaL_getmetatable(L, LUA_ENVELOPE_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_envelope_set_input(lua_State *L) {
    Module    *dst    = env_check(L, 1);
    Module   **src_ud = (Module **)lua_touserdata(L, 2);
    if (!src_ud) return luaL_error(L, "env:setInput: expected a module");
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    dst->num_inputs = 0;
    module_connect(dst, *src_ud);
    audio_engine_unlock(eng->audio);
    return 0;
}

/* trigger() uses an atomic store — no audio lock needed */
static int lua_envelope_trigger(lua_State *L) {
    Module *mod = env_check(L, 1);
    envelope_trigger(mod);
    return 0;
}

static int lua_envelope_release(lua_State *L) {
    Module *mod = env_check(L, 1);
    envelope_release(mod);
    return 0;
}

static int lua_envelope_set_attack(lua_State *L) {
    Module    *mod = env_check(L, 1);
    double     v   = luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    envelope_set_attack(mod, v);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_envelope_set_decay(lua_State *L) {
    Module    *mod = env_check(L, 1);
    double     v   = luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    envelope_set_decay(mod, v);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_envelope_set_sustain(lua_State *L) {
    Module    *mod = env_check(L, 1);
    double     v   = luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    envelope_set_sustain(mod, v);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_envelope_set_release(lua_State *L) {
    Module    *mod = env_check(L, 1);
    double     v   = luaL_checknumber(L, 2);
    LuaEngine *eng = lua_engine_get(L);
    audio_engine_lock(eng->audio);
    envelope_set_release(mod, v);
    audio_engine_unlock(eng->audio);
    return 0;
}

static int lua_envelope_gc(lua_State *L) {
    Module **ud = (Module **)lua_touserdata(L, 1);
    if (ud && *ud && !(*ud)->in_graph) { module_free(*ud); *ud = NULL; }
    return 0;
}

void lua_envelope_register(lua_State *L) {
    luaL_newmetatable(L, LUA_ENVELOPE_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    static const luaL_Reg methods[] = {
        {"setInput",    lua_envelope_set_input},
        {"trigger",     lua_envelope_trigger},
        {"release",     lua_envelope_release},
        {"setAttack",   lua_envelope_set_attack},
        {"setDecay",    lua_envelope_set_decay},
        {"setSustain",  lua_envelope_set_sustain},
        {"setRelease",  lua_envelope_set_release},
        {"__gc",        lua_envelope_gc},
        {NULL, NULL}
    };
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);

    static const luaL_Reg lib[] = { {"new", lua_envelope_new}, {NULL, NULL} };
    luaL_newlib(L, lib);
    lua_setglobal(L, "Envelope");
}
