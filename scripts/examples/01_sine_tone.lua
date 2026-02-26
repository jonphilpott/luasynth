--[[
    01_sine_tone.lua — Phase 1 Example: Hello Audio

    The simplest possible patch: one sine oscillator → audio output.

    Demonstrates:
        - Creating an Osc module from Lua
        - Routing it to the speakers with Output.set()
        - The pull model: the audio callback asks the osc for samples

    Run with:
        make run-01
        ./luasynth scripts/examples/01_sine_tone.lua
--]]

print("Example 01: 440 Hz sine tone")

-- Create a sine oscillator at 440 Hz (A4 = concert pitch)
local osc = Osc.new("sine", 440.0)

-- Route it to the audio output
-- After this call, the SDL audio thread will ask the osc for samples
-- every ~11.6 ms (at 44100 Hz / 512 frames per buffer)
Output.set(osc)

-- The main loop in main.c keeps the app alive until Ctrl-C
