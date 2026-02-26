/**
 * module.c — Module base implementation
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "module.h"

/* -------------------------------------------------------------------------
 * module_alloc
 * ---------------------------------------------------------------------- */
Module *module_alloc(ModuleType type) {
    Module *mod = (Module *)calloc(1, sizeof(Module));
    if (!mod) {
        fprintf(stderr, "module_alloc: out of memory\n");
        return NULL;
    }
    mod->type = type;
    return mod;
}

/* -------------------------------------------------------------------------
 * module_free
 * ---------------------------------------------------------------------- */
void module_free(Module *mod) {
    if (!mod) return;
    if (mod->destroy) {
        mod->destroy(mod);
    }
    free(mod);
}

/* -------------------------------------------------------------------------
 * module_connect
 * ---------------------------------------------------------------------- */
int module_connect(Module *dst, Module *src) {
    if (dst->num_inputs >= MODULE_MAX_INPUTS) {
        fprintf(stderr, "module_connect: module '%s' already has %d inputs\n",
                dst->name ? dst->name : "?", MODULE_MAX_INPUTS);
        return -1;
    }
    dst->inputs[dst->num_inputs++] = src;
    return 0;
}

/* -------------------------------------------------------------------------
 * module_connect_mod
 * ---------------------------------------------------------------------- */
int module_connect_mod(Module *dst, Module *src, const char *param, float depth) {
    if (dst->num_mod_inputs >= MODULE_MAX_MOD_INPUTS) {
        fprintf(stderr, "module_connect_mod: module '%s' already has %d mod inputs\n",
                dst->name ? dst->name : "?", MODULE_MAX_MOD_INPUTS);
        return -1;
    }
    ModInput *mi = &dst->mod_inputs[dst->num_mod_inputs++];
    mi->source = src;
    strncpy(mi->param, param, sizeof(mi->param) - 1);
    mi->param[sizeof(mi->param) - 1] = '\0';
    mi->depth  = depth;
    return 0;
}
