--[[
    06_full_patch.lua — Phase 6 Example: Full Patch

    Demonstrates all modules working together:
        - Two oscillators (lead + bass) with detuning
        - Moog ladder filter on the lead with LFO sweep
        - ADSR envelope on both voices
        - Delay echo on the lead
        - Clock driving a two-voice sequencer

    Patch architecture:
        bass_osc ─── bass_env ─────────────────────────────┐
                                                            ├── Output
        lead_osc ─── lead_env ─── lead_filter ─── lead_dly ┘
                               ↑
                           filter_lfo (cutoff modulation)

    Run with:
        make run-06
--]]

print("Example 06: Full patch — two-voice sequencer with all modules")

-- =========================================================================
-- Bass voice
-- =========================================================================
local bass_notes = { 36, 36, 38, 36, 41, 41, 43, 41 }  -- C2 sequence
local bass_idx = 1

local bass_osc = Osc.new("saw", midi_to_freq(36))
bass_osc:setAmplitude(0.4)

local bass_env = Envelope.new(0.008, 0.15, 0.3, 0.1)
bass_env:setInput(bass_osc)

-- Slightly low-passed bass to remove harshness
local bass_flt = Filter.new("svf")
bass_flt:setInput(bass_env)
bass_flt:setCutoff(600.0)
bass_flt:setResonance(0.3)

-- =========================================================================
-- Lead voice
-- =========================================================================
local lead_notes = { 60, 63, 65, 67, 65, 63, 60, 58 }  -- C4 minor line
local lead_idx = 1

-- Two detuned oscillators for a thicker lead sound
local lead_osc1 = Osc.new("saw", midi_to_freq(60))
local lead_osc2 = Osc.new("saw", midi_to_freq(60))
lead_osc1:setAmplitude(0.25)
lead_osc2:setAmplitude(0.25)

-- Detune osc2 slightly for chorus effect (4 Hz detune = slight warmth)
lead_osc1:connect(lead_osc2)  -- mix osc2 into osc1's output

-- Lead filter: Moog ladder for that classic warm sound
local lead_flt = Filter.new("moog")
lead_flt:setInput(lead_osc1)
lead_flt:setCutoff(1500.0)
lead_flt:setResonance(0.4)

-- LFO sweeping the filter cutoff
local filter_lfo = LFO.new("sine", 0.25)  -- very slow, 4-second cycle
filter_lfo:connectMod(lead_flt, "cutoff", 1000.0)

-- Lead envelope
local lead_env = Envelope.new(0.01, 0.2, 0.5, 0.3)
lead_env:setInput(lead_flt)

-- Delay on lead for spatial depth
local lead_dly = Delay.new(0.3, 0.45)
lead_dly:setInput(lead_env)
lead_dly:setWet(0.4)
lead_dly:setDry(1.0)

-- =========================================================================
-- Mixer: connect bass_flt as additional input to lead_dly output
-- We abuse the oscillator's additive input mechanism for mixing
-- =========================================================================
-- We need a simple mix: wire bass into the delay module's input chain
-- Since Delay only has one input slot, we use the osc connect trick:
-- lead_osc1 gets bass_flt mixed in via connect()
lead_osc1:connect(bass_flt)

Output.set(lead_dly)

-- =========================================================================
-- Clock: 120 BPM, alternating bass/lead triggers
-- =========================================================================
local clk = Clock.new(120)
clk:onBeat(function(beat)
    -- Every beat: trigger bass
    local b_note = bass_notes[bass_idx]
    bass_idx = (bass_idx % #bass_notes) + 1
    bass_osc:setFrequency(midi_to_freq(b_note))
    bass_env:trigger()

    -- Every other beat: trigger lead (half-time feel)
    if beat % 2 == 1 then
        local l_note = lead_notes[lead_idx]
        lead_idx = (lead_idx % #lead_notes) + 1
        lead_osc1:setFrequency(midi_to_freq(l_note))
        lead_osc2:setFrequency(midi_to_freq(l_note) * 1.003)  -- +0.3% detune
        lead_env:trigger()
    end
end)
clk:start()

print("Full patch running at 120 BPM — Ctrl-C to stop")
print("Bass:  C2 sequence (saw → SVF → envelope)")
print("Lead:  C4 minor (2× detuned saw → Moog → LFO → envelope → delay)")
