/**
 * lua_lfo.h — Lua bindings for the LFO module
 *
 * Lua API:
 *   LFO.new(waveform, rate_hz)          → lfo userdata
 *   lfo:setFrequency(hz)                — set LFO rate [0.01, 20]
 *   lfo:setDepth(depth)                 — set output scale [0, 1]
 *   lfo:setWaveform(name)               — "sine", "square", "saw", "triangle"
 *   lfo:connectMod(target, param, depth)— wire LFO as modulator
 */

#ifndef LUASYNTH_LUA_LFO_H
#define LUASYNTH_LUA_LFO_H

#include <lua.h>

#define LUA_LFO_MT "LuaSynth.LFO"

void lua_lfo_register(lua_State *L);

#endif /* LUASYNTH_LUA_LFO_H */
