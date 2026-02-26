--[[
    startup.lua — Default LuaSynth startup script

    This is the script that runs when you call `make run` with no arguments.
    It plays a simple sine tone at 440 Hz (A4) for demonstration.

    To create your own patch, edit this file or pass a different script:
        ./luasynth scripts/examples/01_sine_tone.lua
--]]

print("LuaSynth default startup: playing 440 Hz sine tone")
print("Press Ctrl-C to stop")

-- Create a sine oscillator at A4 (440 Hz)
local osc = Osc.new("sine", 440.0)

-- Designate it as the audio output
Output.set(osc)

-- Keep playing indefinitely (main.c's event loop handles Ctrl-C)
