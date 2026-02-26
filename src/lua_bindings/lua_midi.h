/**
 * lua_midi.h — Lua MIDI utility functions
 *
 * Provides MIDI-to-frequency conversion as a global Lua function.
 * No userdata — this is a pure function binding.
 *
 * Lua API:
 *   midi_to_freq(note) → frequency_hz
 *
 * Example:
 *   local freq = midi_to_freq(69)  -- A4 = 440 Hz
 *   osc:setFrequency(freq)
 *
 * Formula: freq = 440 * 2^((note - 69) / 12)
 *   - MIDI note 69 = A4 = 440 Hz
 *   - Each semitone multiplies by 2^(1/12) ≈ 1.0595
 */

#ifndef LUASYNTH_LUA_MIDI_H
#define LUASYNTH_LUA_MIDI_H

#include <lua.h>

void lua_midi_register(lua_State *L);

#endif /* LUASYNTH_LUA_MIDI_H */
