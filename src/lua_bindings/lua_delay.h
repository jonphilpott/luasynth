/**
 * lua_delay.h — Lua bindings for the Delay module
 *
 * Lua API:
 *   Delay.new(delay_time, feedback)   → delay userdata
 *   dly:setInput(module)
 *   dly:setDelayTime(secs)
 *   dly:setFeedback(amount)           — [0, 0.99]
 *   dly:setWet(amount)                — [0, 1]
 *   dly:setDry(amount)                — [0, 1]
 */

#ifndef LUASYNTH_LUA_DELAY_H
#define LUASYNTH_LUA_DELAY_H

#include <lua.h>

#define LUA_DELAY_MT "LuaSynth.Delay"

void lua_delay_register(lua_State *L);

#endif /* LUASYNTH_LUA_DELAY_H */
