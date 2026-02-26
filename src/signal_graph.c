/**
 * signal_graph.c — Signal Graph implementation
 *
 * See signal_graph.h for the design overview.
 */

#include <string.h>
#include <stdio.h>
#include "signal_graph.h"

/* -------------------------------------------------------------------------
 * signal_graph_init
 * ---------------------------------------------------------------------- */
void signal_graph_init(SignalGraph *g) {
    memset(g, 0, sizeof(SignalGraph));
}

/* -------------------------------------------------------------------------
 * signal_graph_add
 * ---------------------------------------------------------------------- */
int signal_graph_add(SignalGraph *g, Module *mod) {
    if (g->num_modules >= SIGNAL_GRAPH_MAX_MODULES) {
        fprintf(stderr, "signal_graph: registry full (%d modules)\n",
                SIGNAL_GRAPH_MAX_MODULES);
        return -1;
    }
    g->modules[g->num_modules++] = mod;
    /* Transfer ownership: graph now manages this module's lifetime */
    mod->in_graph = 1;
    return 0;
}

/* -------------------------------------------------------------------------
 * signal_graph_set_output
 * ---------------------------------------------------------------------- */
void signal_graph_set_output(SignalGraph *g, Module *mod) {
    g->output_module = mod;
}

/* -------------------------------------------------------------------------
 * signal_graph_process — pull model entry point.
 *
 * If no output module has been set we write silence (zeros). Otherwise we
 * delegate entirely to the output module's process() callback, which will
 * recursively pull from its upstream inputs.
 * ---------------------------------------------------------------------- */
void signal_graph_process(SignalGraph *g, float *out_buf, int num_frames, int sample_rate) {
    if (!g->output_module || !g->output_module->process) {
        /* No patch loaded yet — output silence */
        memset(out_buf, 0, sizeof(float) * (size_t)(num_frames * 2));
        return;
    }
    g->output_module->process(g->output_module, out_buf, num_frames, sample_rate);
}

/* -------------------------------------------------------------------------
 * signal_graph_destroy
 * ---------------------------------------------------------------------- */
void signal_graph_destroy(SignalGraph *g) {
    for (int i = 0; i < g->num_modules; i++) {
        module_free(g->modules[i]);
        g->modules[i] = NULL;
    }
    g->num_modules    = 0;
    g->output_module  = NULL;
}
