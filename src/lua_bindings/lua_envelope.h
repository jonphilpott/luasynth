/**
 * lua_envelope.h — Lua bindings for the Envelope module
 *
 * Lua API:
 *   Envelope.new(attack, decay, sustain, release)  → env userdata
 *   env:setInput(module)    — connect upstream audio source
 *   env:trigger()           — start attack stage
 *   env:release()           — start release stage
 *   env:setAttack(secs)
 *   env:setDecay(secs)
 *   env:setSustain(level)   — [0, 1]
 *   env:setRelease(secs)
 */

#ifndef LUASYNTH_LUA_ENVELOPE_H
#define LUASYNTH_LUA_ENVELOPE_H

#include <lua.h>

#define LUA_ENVELOPE_MT "LuaSynth.Envelope"

void lua_envelope_register(lua_State *L);

#endif /* LUASYNTH_LUA_ENVELOPE_H */
