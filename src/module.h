/**
 * module.h — Base Module Interface for LuaSynth
 *
 * Every audio/control source in LuaSynth is a Module. Modules form a directed
 * acyclic graph (DAG): each module's inputs[] array points to upstream modules
 * that feed audio data into it. The graph is traversed in a "pull" model —
 * the output sink asks its upstream neighbours for samples, which in turn ask
 * their upstream neighbours, recursively.
 *
 * This header defines:
 *   - ModuleType enum  — identifies which kind of module this is
 *   - Module struct    — the base record every module instance carries
 *   - module_alloc()   — heap-allocates and zeroes a Module with a given type
 *   - module_free()    — calls mod->destroy() then frees the struct
 *   - module_connect() — wires one module as an audio input of another
 *
 * The process() callback is the heart of the pull model:
 *   int process(Module *self, float *out_buf, int num_frames, int sample_rate)
 * It fills out_buf with num_frames interleaved stereo floats and returns 0 on
 * success, non-zero on error.
 *
 * Thread safety note:
 *   process() is called from the SDL audio thread. Any parameters modified
 *   from Lua (the main thread) must be protected — either with
 *   SDL_LockAudioDevice / SDL_UnlockAudioDevice, or with C11 _Atomic types
 *   for single-word flag values (see envelope.h for an example).
 */

#ifndef LUASYNTH_MODULE_H
#define LUASYNTH_MODULE_H

/* Maximum scratch buffer size: 4096 stereo frames × 2 channels */
#define MODULE_SCRATCH_FRAMES 4096
#define MODULE_SCRATCH_SAMPLES (MODULE_SCRATCH_FRAMES * 2)

/* Maximum upstream audio connections per module */
#define MODULE_MAX_INPUTS 8

/* Maximum control-rate (LFO / mod) connections per module */
#define MODULE_MAX_MOD_INPUTS 4

/* -------------------------------------------------------------------------
 * ModuleType — a tag so we can inspect what kind of module we have at
 * runtime (useful for debugging and for Lua type checks).
 * ---------------------------------------------------------------------- */
typedef enum {
    MODULE_OSC,
    MODULE_NOISE,
    MODULE_FILTER,
    MODULE_LFO,
    MODULE_ENVELOPE,
    MODULE_DELAY,
    MODULE_CLOCK,
    MODULE_OUTPUT,
    MODULE_TYPE_COUNT
} ModuleType;

/* Forward declaration so the struct can reference itself */
typedef struct Module Module;

/* -------------------------------------------------------------------------
 * ModInput — describes a control-rate modulation connection.
 * An LFO (or any Module) can be wired as a modulator for a named parameter
 * on a target module. The audio thread reads the LFO's current value and
 * adds it (scaled by depth) to the target parameter each frame.
 * ---------------------------------------------------------------------- */
typedef struct {
    Module     *source;      /* the modulating module (e.g., LFO) */
    char        param[32];   /* name of the parameter being modulated */
    float       depth;       /* modulation depth (0.0 – 1.0 typical) */
} ModInput;

/* -------------------------------------------------------------------------
 * Module — the base struct every audio/control module extends.
 *
 * "Extension" here means composition: each module allocates its own private
 * data block (OscData, FilterData, …) and stores a pointer in the `data`
 * field. The process() and destroy() function pointers are set at alloc time.
 * ---------------------------------------------------------------------- */
struct Module {
    ModuleType   type;              /* which kind of module */
    const char  *name;             /* human-readable name (static string) */

    /* -- Audio inputs --------------------------------------------------- */
    Module      *inputs[MODULE_MAX_INPUTS];
    int          num_inputs;

    /* -- Control-rate modulation inputs ---------------------------------- */
    ModInput     mod_inputs[MODULE_MAX_MOD_INPUTS];
    int          num_mod_inputs;

    /* -- Virtual dispatch ------------------------------------------------ */
    /**
     * process — fill out_buf with num_frames stereo samples.
     *
     * @param self        this module
     * @param out_buf     output buffer, interleaved L/R, length = num_frames*2
     * @param num_frames  number of audio frames to produce
     * @param sample_rate sample rate in Hz (e.g. 44100)
     * @return 0 on success, non-zero on error
     */
    int  (*process)(Module *self, float *out_buf, int num_frames, int sample_rate);

    /**
     * destroy — release any resources owned by this module's `data` block.
     * Called by module_free() before freeing the Module struct itself.
     *
     * @param self  this module
     */
    void (*destroy)(Module *self);

    /* -- Private data pointer ------------------------------------------- */
    void        *data;             /* points to OscData / FilterData / etc. */

    /* -- Ownership flag -------------------------------------------------- */
    /**
     * in_graph — set to 1 when the module is registered with the signal graph.
     * The Lua __gc metamethod checks this flag: if in_graph == 0 it frees the
     * module (it's still only owned by Lua); if in_graph == 1 the signal graph
     * owns it and Lua's gc should not free it.
     */
    int          in_graph;

    /* -- Pre-allocated scratch buffer ------------------------------------ */
    /**
     * scratch — a pre-allocated stereo sample buffer used internally by the
     * audio callback. Avoids malloc() calls in the real-time audio thread.
     * Size is sufficient for the maximum SDL buffer (4096 frames × 2 channels).
     */
    float        scratch[MODULE_SCRATCH_SAMPLES];
};

/* -------------------------------------------------------------------------
 * module_alloc — allocate and zero a Module with the given type.
 *
 * @param type  the ModuleType tag
 * @return      pointer to newly allocated Module, or NULL on OOM
 * ---------------------------------------------------------------------- */
Module *module_alloc(ModuleType type);

/* -------------------------------------------------------------------------
 * module_free — call destroy() and free the Module struct.
 *
 * @param mod  module to free (may be NULL — safe no-op)
 * ---------------------------------------------------------------------- */
void module_free(Module *mod);

/* -------------------------------------------------------------------------
 * module_connect — wire `src` as an audio input of `dst`.
 *
 * @param dst  the downstream module (consumer)
 * @param src  the upstream module (producer)
 * @return     0 on success, -1 if dst already has MODULE_MAX_INPUTS inputs
 * ---------------------------------------------------------------------- */
int module_connect(Module *dst, Module *src);

/* -------------------------------------------------------------------------
 * module_connect_mod — wire `src` as a control-rate modulator of a named
 * parameter on `dst`.
 *
 * @param dst    the target module
 * @param src    the modulating module (e.g., LFO)
 * @param param  name of the parameter (e.g., "cutoff", "frequency")
 * @param depth  modulation depth scalar
 * @return       0 on success, -1 if dst already has MODULE_MAX_MOD_INPUTS
 * ---------------------------------------------------------------------- */
int module_connect_mod(Module *dst, Module *src, const char *param, float depth);

#endif /* LUASYNTH_MODULE_H */
