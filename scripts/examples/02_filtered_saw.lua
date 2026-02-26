--[[
    02_filtered_saw.lua — Phase 2 Example: Filtered Sawtooth

    Demonstrates:
        - The sawtooth oscillator (rich in harmonics — ideal filter input)
        - The State-Variable Filter (SVF) in lowpass mode
        - Chaining modules: osc → filter → output
        - Sweeping the filter cutoff from Lua using a timer loop

    The sawtooth wave has strong high-frequency content that the lowpass
    filter progressively attenuates as the cutoff decreases, giving that
    classic "filter sweep" sound.

    Run with:
        make run-02
--]]

print("Example 02: Filtered sawtooth with cutoff sweep")

-- Create a sawtooth oscillator at A2 (110 Hz) — low and harmonically rich
local osc = Osc.new("saw", 110.0)
osc:setAmplitude(0.5)

-- Create a State-Variable Filter in lowpass mode
local flt = Filter.new("svf")
flt:setInput(osc)
flt:setCutoff(2000.0)    -- start with cutoff wide open
flt:setResonance(1.5)    -- moderate resonance
flt:setMode("lowpass")

-- Route the filter to the output
Output.set(flt)

-- Sweep the cutoff from 2000 Hz down to 100 Hz and back, in steps
-- This loop runs synchronously in the Lua script thread — the audio
-- thread keeps running independently throughout.
print("Sweeping filter cutoff down...")
local cutoff = 2000.0
while cutoff > 100.0 do
    flt:setCutoff(cutoff)
    SDL.delay(30)          -- 30ms between steps ≈ smooth sweep
    cutoff = cutoff * 0.97 -- exponential decay (sounds more natural than linear)
end

print("Sweeping cutoff back up...")
while cutoff < 2000.0 do
    flt:setCutoff(cutoff)
    SDL.delay(30)
    cutoff = cutoff * 1.03
end

print("Holding... Ctrl-C to stop")
