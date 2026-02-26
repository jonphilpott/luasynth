/**
 * lua_output.h — Lua binding for the Output sink
 *
 * The Output module is a special global sink. There is exactly one output:
 * the signal graph's output_module. From Lua, you designate which module's
 * audio reaches the speakers with Output.set(mod).
 *
 * The Output module itself is a thin pass-through that just calls its single
 * input's process() and copies the result to the output buffer.
 *
 * Lua API:
 *   Output.set(module)   -- designate a module as the final audio output
 *   Output.get()         -- return the current output module
 */

#ifndef LUASYNTH_LUA_OUTPUT_H
#define LUASYNTH_LUA_OUTPUT_H

#include <lua.h>

void lua_output_register(lua_State *L);

#endif /* LUASYNTH_LUA_OUTPUT_H */
