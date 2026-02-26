/**
 * lua_output.c — Lua binding for the Output sink
 */

#include <lua.h>
#include <lauxlib.h>
#include "lua_output.h"
#include "lua_engine.h"

/* -------------------------------------------------------------------------
 * Output.set(module) — designate a module as the audio output.
 *
 * The module must already be in the signal graph (which is guaranteed if it
 * was created via any of the Lua constructors like Osc.new()).
 * ---------------------------------------------------------------------- */
static int lua_output_set(lua_State *L) {
    /* Any module type can be the output — read its userdata directly */
    Module **ud = (Module **)lua_touserdata(L, 1);
    if (!ud || !*ud) {
        return luaL_error(L, "Output.set: expected a module userdata");
    }

    LuaEngine *engine = lua_engine_get(L);

    /* Pause the audio callback while changing the output module */
    audio_engine_lock(engine->audio);
    signal_graph_set_output(engine->graph, *ud);
    audio_engine_unlock(engine->audio);

    return 0;
}

/* -------------------------------------------------------------------------
 * Output.get() — return the current output module userdata (or nil).
 * ---------------------------------------------------------------------- */
static int lua_output_get(lua_State *L) {
    LuaEngine *engine = lua_engine_get(L);
    if (engine->graph->output_module) {
        /* Push the module pointer as a light userdata — not a managed object */
        lua_pushlightuserdata(L, engine->graph->output_module);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

void lua_output_register(lua_State *L) {
    static const luaL_Reg lib[] = {
        {"set", lua_output_set},
        {"get", lua_output_get},
        {NULL, NULL}
    };
    luaL_newlib(L, lib);
    lua_setglobal(L, "Output");
}
