/**
 * lua_filter.h — Lua bindings for the Filter module
 *
 * Lua API:
 *   Filter.new(type_string)          → filter userdata
 *     type_string: "svf" or "moog"
 *   flt:setInput(module)             — connect upstream audio source
 *   flt:setCutoff(hz)                — set cutoff frequency [20, 20000]
 *   flt:setResonance(res)            — set resonance [0, 4]
 *   flt:setMode(mode_string)         — "lowpass", "bandpass", "highpass"
 *   flt:connectMod(target, param, depth)  — wire self as LFO modulator
 */

#ifndef LUASYNTH_LUA_FILTER_H
#define LUASYNTH_LUA_FILTER_H

#include <lua.h>

#define LUA_FILTER_MT "LuaSynth.Filter"

void lua_filter_register(lua_State *L);

#endif /* LUASYNTH_LUA_FILTER_H */
