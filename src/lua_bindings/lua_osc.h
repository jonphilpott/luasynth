/**
 * lua_osc.h — Lua bindings for the Oscillator module
 *
 * Exposes the Osc module to Lua as a table of functions and a userdata type.
 *
 * Lua API:
 *   Osc.new(waveform, frequency) → osc userdata
 *   osc:setFrequency(hz)
 *   osc:setAmplitude(amp)
 *   osc:setWaveform(name)
 *   osc:connect(other_module)       -- add other as audio input
 *   osc:connectMod(target, param, depth)  -- wire self as LFO into target
 */

#ifndef LUASYNTH_LUA_OSC_H
#define LUASYNTH_LUA_OSC_H

#include <lua.h>

/** Metatable name used with luaL_newmetatable / luaL_checkudata */
#define LUA_OSC_MT "LuaSynth.Osc"

/**
 * lua_osc_register — register the Osc library into the given Lua state.
 * Creates the global table "Osc" and the "LuaSynth.Osc" metatable.
 *
 * @param L  the Lua state
 */
void lua_osc_register(lua_State *L);

#endif /* LUASYNTH_LUA_OSC_H */
