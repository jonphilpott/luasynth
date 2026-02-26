/**
 * signal_graph.h — Signal Graph Registry
 *
 * The signal graph is a thin registry that:
 *   1. Keeps track of all modules in the patch so we can manage their
 *      lifetimes separately from Lua's garbage collector.
 *   2. Provides a single "root" output module that the audio engine asks
 *      for samples each callback.
 *
 * Ownership model:
 *   When a module is added to the graph (signal_graph_add), its `in_graph`
 *   flag is set to 1. This tells the Lua __gc metamethod not to free it —
 *   the graph owns it now. When the graph is destroyed (signal_graph_destroy),
 *   every module in the registry is freed.
 *
 * The pull model:
 *   The audio engine calls signal_graph_process() each SDL callback. That
 *   function simply asks the output_module to process() — which recursively
 *   pulls from its inputs. No explicit traversal order is needed: the
 *   recursion naturally follows the DAG edges.
 */

#ifndef LUASYNTH_SIGNAL_GRAPH_H
#define LUASYNTH_SIGNAL_GRAPH_H

#include "module.h"

/* Maximum number of modules in a single patch */
#define SIGNAL_GRAPH_MAX_MODULES 256

/* -------------------------------------------------------------------------
 * SignalGraph — the registry struct.
 * ---------------------------------------------------------------------- */
typedef struct {
    Module  *modules[SIGNAL_GRAPH_MAX_MODULES];
    int      num_modules;
    Module  *output_module;   /* the final sink; process() is called on this */
} SignalGraph;

/* -------------------------------------------------------------------------
 * signal_graph_init — initialise the graph to an empty state.
 *
 * @param g  pointer to a caller-allocated SignalGraph struct
 * ---------------------------------------------------------------------- */
void signal_graph_init(SignalGraph *g);

/* -------------------------------------------------------------------------
 * signal_graph_add — register a module with the graph.
 * Sets mod->in_graph = 1 so Lua's __gc won't double-free it.
 *
 * @param g    the graph
 * @param mod  module to register
 * @return     0 on success, -1 if the registry is full
 * ---------------------------------------------------------------------- */
int signal_graph_add(SignalGraph *g, Module *mod);

/* -------------------------------------------------------------------------
 * signal_graph_set_output — designate a module as the final audio sink.
 * The module must already be registered (signal_graph_add).
 *
 * @param g    the graph
 * @param mod  the output/sink module
 * ---------------------------------------------------------------------- */
void signal_graph_set_output(SignalGraph *g, Module *mod);

/* -------------------------------------------------------------------------
 * signal_graph_process — pull num_frames stereo samples from the output
 * module into out_buf.  Called from the SDL audio callback.
 *
 * @param g           the graph
 * @param out_buf     destination buffer (interleaved L/R floats)
 * @param num_frames  number of audio frames
 * @param sample_rate sample rate in Hz
 * ---------------------------------------------------------------------- */
void signal_graph_process(SignalGraph *g, float *out_buf, int num_frames, int sample_rate);

/* -------------------------------------------------------------------------
 * signal_graph_destroy — free every registered module and reset the graph.
 *
 * @param g  the graph
 * ---------------------------------------------------------------------- */
void signal_graph_destroy(SignalGraph *g);

#endif /* LUASYNTH_SIGNAL_GRAPH_H */
