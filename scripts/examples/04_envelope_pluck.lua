--[[
    04_envelope_pluck.lua — Phase 4 Example: Rhythmic Melody via Clock + Envelopes

    Demonstrates:
        - The Clock module generating beat events
        - The Envelope module shaping note amplitude
        - The ring buffer pattern: audio thread fires events → main thread calls Lua
        - Sequencing: a Lua table of MIDI notes played in sequence

    The clock fires a beat callback on the main thread. The callback:
        1. Picks the next note from a sequence table.
        2. Updates the oscillator frequency using midi_to_freq().
        3. Triggers the envelope (atomic store — no lock needed).

    Run with:
        make run-04
--]]

print("Example 04: Rhythmic melody with envelopes and clock")

-- A pentatonic minor scale sequence (MIDI note numbers)
-- A3=57, C4=60, D4=62, Eb4=63, G4=67, A4=69 ...
local notes = { 57, 60, 62, 63, 67, 69, 67, 63, 62, 60 }
local beat_index = 1

-- Oscillator: square wave for a retro / chiptune feel
local osc = Osc.new("square", midi_to_freq(notes[1]))
osc:setAmplitude(0.4)

-- Short ADSR envelope for a plucked/percussive feel
-- attack=5ms, decay=80ms, sustain=0.2, release=150ms
local env = Envelope.new(0.005, 0.08, 0.2, 0.15)
env:setInput(osc)

-- Light lowpass filter to smooth out the square wave harshness
local flt = Filter.new("svf")
flt:setInput(env)
flt:setCutoff(1800.0)
flt:setResonance(0.5)

Output.set(flt)

-- Create a clock at 160 BPM (aggressive tempo for interesting rhythm)
local clk = Clock.new(160)

-- Register the beat callback.
-- This function is called from the main thread (via lua_engine_poll)
-- each time the clock fires a beat.
clk:onBeat(function(beat)
    -- Cycle through the note sequence
    local note = notes[beat_index]
    beat_index = (beat_index % #notes) + 1

    -- Update oscillator frequency (SDL lock held inside setFrequency)
    osc:setFrequency(midi_to_freq(note))

    -- Trigger the envelope (atomic store — no lock needed)
    env:trigger()
end)

-- Start the clock — it will fire the first beat almost immediately
clk:start()

print("Clock running at 160 BPM — Ctrl-C to stop")
print("Playing pentatonic minor sequence")
