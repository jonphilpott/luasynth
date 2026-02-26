/**
 * lua_midi.c — MIDI utility Lua bindings
 */

#include <math.h>
#include <lua.h>
#include <lauxlib.h>
#include "lua_midi.h"

/**
 * midi_to_freq(note) — convert a MIDI note number to a frequency in Hz.
 *
 * @param L[1]  MIDI note number (integer, 0–127; 69 = A4)
 * @return      frequency in Hz as a Lua number
 */
static int lua_midi_to_freq(lua_State *L) {
    double note = luaL_checknumber(L, 1);
    /* Equal temperament: A4 (note 69) = 440 Hz
     * Each octave doubles frequency; 12 semitones per octave */
    double freq = 440.0 * pow(2.0, (note - 69.0) / 12.0);
    lua_pushnumber(L, freq);
    return 1;
}

void lua_midi_register(lua_State *L) {
    /* Register as a global function (not a table) for convenience */
    lua_pushcfunction(L, lua_midi_to_freq);
    lua_setglobal(L, "midi_to_freq");
}
