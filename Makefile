# =============================================================================
# LuaSynth Makefile
# =============================================================================
# Detects macOS vs Linux and uses the appropriate flags for SDL2 and Lua 5.4.
# Sources are enumerated explicitly — no wildcards — so every file must be
# listed here when added.
#
# Targets:
#   all       — build the luasynth binary
#   clean     — remove build artifacts
#   run       — build and run with default startup.lua
#   run-01    — build and run example 01 (sine tone)
#   run-02    — build and run example 02 (filtered saw)
#   run-03    — build and run example 03 (LFO modulation)
#   run-04    — build and run example 04 (envelope pluck)
#   run-05    — build and run example 05 (delay echo)
#   run-06    — build and run example 06 (full patch)
# =============================================================================

CC      = cc
TARGET  = luasynth
BINDIR  = .

# ---------------------------------------------------------------------------
# Platform detection
# ---------------------------------------------------------------------------
UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    # macOS: SDL2 and Lua installed via Homebrew
    SDL2_CFLAGS  := $(shell sdl2-config --cflags)
    SDL2_LIBS    := $(shell sdl2-config --libs)
    LUA_CFLAGS   := $(shell pkg-config --cflags lua5.4 2>/dev/null || pkg-config --cflags lua 2>/dev/null || echo "-I/usr/local/include/lua5.4")
    LUA_LIBS     := $(shell pkg-config --libs lua5.4 2>/dev/null || pkg-config --libs lua 2>/dev/null || echo "-llua5.4")
else
    # Linux: SDL2 and Lua via system package manager
    SDL2_CFLAGS  := $(shell sdl2-config --cflags)
    SDL2_LIBS    := $(shell sdl2-config --libs)
    LUA_CFLAGS   := $(shell pkg-config --cflags lua5.4 2>/dev/null || pkg-config --cflags lua 2>/dev/null || echo "-I/usr/include/lua5.4")
    LUA_LIBS     := $(shell pkg-config --libs lua5.4 2>/dev/null || pkg-config --libs lua 2>/dev/null || echo "-llua5.4")
endif

# ---------------------------------------------------------------------------
# Compiler flags
# ---------------------------------------------------------------------------
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -g \
          -I src \
          $(SDL2_CFLAGS) \
          $(LUA_CFLAGS)

LIBS    = $(SDL2_LIBS) $(LUA_LIBS)

# ---------------------------------------------------------------------------
# Source files — add new files here when created
# ---------------------------------------------------------------------------
SRCS = \
    src/main.c \
    src/audio_engine.c \
    src/signal_graph.c \
    src/module.c \
    src/modules/osc.c \
    src/modules/noise.c \
    src/modules/filter.c \
    src/modules/lfo.c \
    src/modules/envelope.c \
    src/modules/delay.c \
    src/modules/clock.c \
    src/lua_bindings/lua_engine.c \
    src/lua_bindings/lua_osc.c \
    src/lua_bindings/lua_noise.c \
    src/lua_bindings/lua_filter.c \
    src/lua_bindings/lua_lfo.c \
    src/lua_bindings/lua_envelope.c \
    src/lua_bindings/lua_delay.c \
    src/lua_bindings/lua_clock.c \
    src/lua_bindings/lua_midi.c \
    src/lua_bindings/lua_output.c \
    src/util/ringbuffer.c \
    src/util/math_util.c

OBJS = $(SRCS:.c=.o)

# ---------------------------------------------------------------------------
# Build rules
# ---------------------------------------------------------------------------
.PHONY: all clean run run-01 run-02 run-03 run-04 run-05 run-06

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

# ---------------------------------------------------------------------------
# Run targets
# ---------------------------------------------------------------------------
run: all
	./$(TARGET) scripts/startup.lua

run-01: all
	./$(TARGET) scripts/examples/01_sine_tone.lua

run-02: all
	./$(TARGET) scripts/examples/02_filtered_saw.lua

run-03: all
	./$(TARGET) scripts/examples/03_lfo_modulation.lua

run-04: all
	./$(TARGET) scripts/examples/04_envelope_pluck.lua

run-05: all
	./$(TARGET) scripts/examples/05_delay_echo.lua

run-06: all
	./$(TARGET) scripts/examples/06_full_patch.lua
