# LUA SYNTH

## Project Overview.

A small project to learn about:
1. lua embedding
2. synth programming with SDL.

I want to learn more about embedding lua inside of applications as a
methodology for building applications. this project will be an example
of that as a learning exercise.

For this project I'd like to build an application that will let me
build a "modular synthesizer" using lua scripts.

The application will provide the following objects
- oscillators with standard synth waveforms (saw, square, sine, triangle)
- noise sources.
- filters (state-variable and moog ladder filter)
- LFOs with same waveforms as standard oscillators
- functions to convert midi note numbers to frequencies
- some kind of tempo clock to trigger events on
- envelopes for triggering percussive sounds
- delay lines, for delay effects.

I should be able to create these objects from within the lua scripts
and connect them together to generate music.

some guidelines:
- We should use SDL2 to provide the audio abstractions
- no C++, plain C
- provide a Makefile that can build on macos and linux

but the reason for this project is to teach me about patterns for
building scriptable applications with the lua engine.

## Current State

All modules implemented and working. Build is clean (`make`). All 6 examples verified:
- `make run-01` — 440 Hz sine tone
- `make run-02` — filtered sawtooth with cutoff sweep
- `make run-03` — LFO modulation (filter wobble + vibrato)
- `make run-04` — rhythmic melody via clock + envelopes
- `make run-05` — delay echo
- `make run-06` — full two-voice patch

## build system
- Makefile
- SDL2 for audio
- pure C, not C++


## Code Style & Documentation Requirements

### Docstrings
- Every class, function, and non-trivial method MUST have a detailed docstring
- Include: purpose, arguments (with types/ranges), return values, and example usage
- Use `/** ... */` style for docstrings

### Comments
- Write comments as if teaching a junior developer new to DSP and C++
- Explain the "why" and "how", not just "what"
- Use inline comments to explain rationale behind specific logic choices
- Educational callouts: when using advanced or non-obvious patterns, explain how they work
- Break complex functions into numbered steps using comments as a roadmap

### other things
- HTML documentation lives at `docs/index.html`
- **MUST be updated** whenever modules, APIs, architecture, or examples change
- Documentation includes: architecture overview, file layout, module reference, usage examples, quick start guide, hardware wiring, and composition guide

### Documentation Requirements
- **Full API reference**: every  must show instantiation with constructor arguments; every method must document each input argument (name, type, range) and return type with description
- **Examples per function or module**: each function or module must include at least one runnable code example
- **Composition guide**: documentation must include a section on how to compose new function or modulees from existing modules, with examples
- **Diagrams**: use styled HTML elements (divs/spans with monospace font and proper spacing) for diagrams — do NOT use raw ASCII art in `<pre>` tags, as character alignment breaks across fonts and browsers
- Keep examples compilable and consistent with the actual API

### Other Guidelines
- you will communicate with me with the persona of Kryten from Red Dwarf as if I was Lister.

## Architecture Notes

### macOS SDL2 include path
`sdl2-config --cflags` returns `-I/opt/homebrew/include/SDL2` — headers must be
`#include <SDL.h>`, NOT `#include <SDL2/SDL.h>`.

### Clock / control modules — critical pattern
Control-only modules (Clock) are wired as audio inputs to the output module via
`clk:start()` so their `process()` runs each audio frame. Any module that is the
audio output (filter, delay, osc) MUST iterate ALL `self->inputs[]` — not just
`inputs[0]` — otherwise the clock's phase accumulator never advances and beat events
never reach the ring buffer. `filter_process` and `delay_process` both do this correctly.

### Thread safety summary
- Parameter writes: `audio_engine_lock()` / `audio_engine_unlock()` (SDL_LockAudioDevice)
- Envelope trigger/release, clock start/stop: `atomic_store()` — no lock needed
- Clock beat → Lua callback: SPSC ring buffer (ringbuffer.c); main loop polls via `lua_engine_poll()`

### Lua binding pattern (full userdata)
```c
Module **ud = lua_newuserdata(L, sizeof(Module *));
*ud = mod;
luaL_getmetatable(L, "LuaSynth.Osc");
lua_setmetatable(L, -2);
```
- All modules added to signal graph via `signal_graph_add()` which sets `mod->in_graph = 1`
- `__gc` only frees if `!mod->in_graph`
- Lua callbacks stored in registry via `luaL_ref()` (see `lua_clock.c`)
- LuaEngine pointer stored in Lua registry under a static key address (see `lua_engine.c`)

### LFO modulation
`lfo:connectMod(target, param, depth)` → calls `module_connect_mod(target, lfo, param, depth)`
which stores in `target->mod_inputs[]`. The target's `process()` calls the LFO's `process()`
per-sample for each mod_input and adds `lfo_output * depth` to the base parameter value.
Supported params: `"frequency"` (osc), `"cutoff"` (filter).
