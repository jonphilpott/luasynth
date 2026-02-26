--[[
    03_lfo_modulation.lua — Phase 3 Example: LFO Modulation

    Demonstrates:
        - Creating an LFO (low-frequency oscillator)
        - Wiring it as a modulator with connectMod()
        - Two simultaneous modulations:
            1. LFO sweeps the filter cutoff (classic auto-wah / filter wobble)
            2. A slower triangle LFO adds vibrato to the oscillator pitch

    The mod_inputs[] mechanism (see module.h) allows control-rate signals
    to modify audio parameters without the overhead of per-sample locking.

    Run with:
        make run-03
--]]

print("Example 03: LFO modulation (filter wobble + pitch vibrato)")

-- Main oscillator: sawtooth for filter modulation demo
local osc = Osc.new("saw", 220.0)   -- A3

-- Vibrato LFO: very slow, small depth for subtle pitch wobble
local vibrato_lfo = LFO.new("sine", 4.0)   -- 4 Hz vibrato
-- connectMod(target_module, param_name, depth_in_Hz)
-- depth=10 means the LFO adds ±10 Hz to the oscillator frequency
vibrato_lfo:connectMod(osc, "frequency", 10.0)

-- Filter to receive cutoff modulation
local flt = Filter.new("svf")
flt:setInput(osc)
flt:setCutoff(800.0)
flt:setResonance(2.0)

-- Filter sweep LFO: 0.5 Hz (one sweep every 2 seconds)
local filter_lfo = LFO.new("sine", 0.5)
-- connectMod on the filter: "cutoff" param, depth=600 Hz
-- Cutoff will oscillate between 800-600=200 Hz and 800+600=1400 Hz
filter_lfo:connectMod(flt, "cutoff", 600.0)

Output.set(flt)

print("LFO modulation running — Ctrl-C to stop")
print("  Vibrato LFO: 4 Hz sine, ±10 Hz pitch")
print("  Filter LFO:  0.5 Hz sine, ±600 Hz cutoff")
