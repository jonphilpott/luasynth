/**
 * lua_noise.h — Lua bindings for the Noise module
 *
 * Lua API:
 *   Noise.new()             → noise userdata
 *   noise:setAmplitude(amp) → nil
 *   noise:connect(other)    → nil  (add other as audio input)
 */

#ifndef LUASYNTH_LUA_NOISE_H
#define LUASYNTH_LUA_NOISE_H

#include <lua.h>

#define LUA_NOISE_MT "LuaSynth.Noise"

void lua_noise_register(lua_State *L);

#endif /* LUASYNTH_LUA_NOISE_H */
