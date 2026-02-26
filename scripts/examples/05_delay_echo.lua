--[[
    05_delay_echo.lua — Phase 5 Example: Delay Echo

    Demonstrates:
        - The Delay module (circular buffer with feedback)
        - Wet/dry mixing for natural-sounding echo
        - Combining melodic notes with reverb-like tail

    The delay creates an echo by playing back the audio signal slightly later.
    The feedback path feeds the delayed output back into the delay input,
    creating multiple echoes that decay over time.

    Run with:
        make run-05
--]]

print("Example 05: Melodic notes with delay echo")

local notes = { 60, 64, 67, 72, 67, 64, 60, 55 }  -- C major arpeggio
local beat_index = 1

-- Sine oscillator for a clean, bell-like timbre
local osc = Osc.new("sine", midi_to_freq(60))
osc:setAmplitude(0.5)

-- Sharp envelope: fast attack, quick decay — each note is a "pluck"
local env = Envelope.new(0.002, 0.3, 0.0, 0.1)
env:setInput(osc)

-- Delay: 375ms (= 1/4 note at 160 BPM), 55% feedback, balanced wet/dry
local dly = Delay.new(0.375, 0.55)
dly:setInput(env)
dly:setWet(0.6)
dly:setDry(1.0)

Output.set(dly)

-- Clock at 160 BPM
local clk = Clock.new(160)
clk:onBeat(function(beat)
    local note = notes[beat_index]
    beat_index = (beat_index % #notes) + 1
    osc:setFrequency(midi_to_freq(note))
    env:trigger()
end)
clk:start()

print("Delay echo running — Ctrl-C to stop")
print("Delay: 375ms, Feedback: 55%")
