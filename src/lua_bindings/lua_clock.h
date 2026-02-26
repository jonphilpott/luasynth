/**
 * lua_clock.h — Lua bindings for the Clock module
 *
 * Lua API:
 *   Clock.new(bpm)              → clock userdata
 *   clk:start()                 — begin generating beats
 *   clk:stop()                  — stop the clock
 *   clk:setBPM(bpm)             — change tempo
 *   clk:onBeat(function(beat))  — register a Lua callback for each beat
 */

#ifndef LUASYNTH_LUA_CLOCK_H
#define LUASYNTH_LUA_CLOCK_H

#include <lua.h>

#define LUA_CLOCK_MT "LuaSynth.Clock"

void lua_clock_register(lua_State *L);

#endif /* LUASYNTH_LUA_CLOCK_H */
