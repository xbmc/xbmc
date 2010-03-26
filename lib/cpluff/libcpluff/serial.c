/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2007 Johannes Lehtinen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *-----------------------------------------------------------------------*/

/** @file
 * Serial execution implementation
 */

#include <stdlib.h>
#include <string.h>
#include "cpluff.h"
#include "internal.h"


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/// A holder structure for a run function.
typedef struct run_func_t {
	
	/// The run function
	cp_run_func_t runfunc;
	
	/// The registering plug-in instance
	cp_plugin_t *plugin;
	
	/// Whether currently in execution
	int in_execution;
	
} run_func_t;

CP_C_API cp_status_t cp_run_function(cp_context_t *ctx, cp_run_func_t runfunc) {
	lnode_t *node = NULL;
	run_func_t *rf = NULL;
	cp_status_t status = CP_OK;
	
	CHECK_NOT_NULL(ctx);
	CHECK_NOT_NULL(runfunc);
	if (ctx->plugin == NULL) {
		cpi_fatalf(_("Only plug-ins can register run functions."));
	}
	if (ctx->plugin->state != CP_PLUGIN_ACTIVE
		&& ctx->plugin->state != CP_PLUGIN_STARTING) {
		cpi_fatalf(_("Only starting or active plug-ins can register run functions."));
	}
	
	cpi_lock_context(ctx);
	cpi_check_invocation(ctx, CPI_CF_STOP | CPI_CF_LOGGER, __func__);
	do {
		int found = 0;
		lnode_t *n;
	
		// Check if already registered
		n = list_first(ctx->env->run_funcs);
		while (n != NULL && !found) {
			run_func_t *r = lnode_get(n);
			if (runfunc == r->runfunc && ctx->plugin == r->plugin) {
				found = 1;
			}
			n = list_next(ctx->env->run_funcs, n);
		}
		if (found) {
			break;
		}

		// Allocate memory for a new run function entry
		if ((rf = malloc(sizeof(run_func_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		if ((node = lnode_create(rf)) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Initialize run function entry
		memset(rf, 0, sizeof(run_func_t));
		rf->runfunc = runfunc;
		rf->plugin = ctx->plugin;
		
		// Append the run function to queue
		list_append(ctx->env->run_funcs, node);
		if (ctx->env->run_wait == NULL) {
			ctx->env->run_wait = node;
		}

	} while (0);

	// Log error
	if (status == CP_ERR_RESOURCE) {
		cpi_error(ctx, N_("Could not register a run function due to insufficient memory."));
	}	
	cpi_unlock_context(ctx);
	
	// Free resources on error
	if (status != CP_OK) {
		if (node != NULL) {
			lnode_destroy(node);
		}
		if (rf != NULL) {
			free(rf);
		}
	}
	
	return status;
}

CP_C_API void cp_run_plugins(cp_context_t *ctx) {
	while (cp_run_plugins_step(ctx));
}

CP_C_API int cp_run_plugins_step(cp_context_t *ctx) {
	int runnables;
	
	CHECK_NOT_NULL(ctx);
	cpi_lock_context(ctx);
	if (ctx->env->run_wait != NULL) {
		lnode_t *node = ctx->env->run_wait;
		run_func_t *rf = lnode_get(node);
		int rerun;
		
		ctx->env->run_wait = list_next(ctx->env->run_funcs, node);
		rf->in_execution = 1;
		cpi_unlock_context(ctx);
		rerun = rf->runfunc(rf->plugin->plugin_data);
		cpi_lock_context(ctx);
		rf->in_execution = 0;
		list_delete(ctx->env->run_funcs, node);
		if (rerun) {
			list_append(ctx->env->run_funcs, node);
			if (ctx->env->run_wait == NULL) {
				ctx->env->run_wait = node;
			}
		} else {
			lnode_destroy(node);
			free(rf);
		}
		cpi_signal_context(ctx);
	}
	runnables = (ctx->env->run_wait != NULL);
	cpi_unlock_context(ctx);
	return runnables;
}

CP_HIDDEN void cpi_stop_plugin_run(cp_plugin_t *plugin) {
	int stopped = 0;
	cp_context_t *ctx;
	
	CHECK_NOT_NULL(plugin);
	ctx = plugin->context;
	assert(cpi_is_context_locked(ctx));
	while (!stopped) {
		lnode_t *node;
		
		stopped = 1;
		node = list_first(ctx->env->run_funcs);
		while (node != NULL) {
			run_func_t *rf = lnode_get(node);
			lnode_t *next_node = list_next(ctx->env->run_funcs, node);
			
			if (rf->plugin == plugin) {
				if (rf->in_execution) {
					stopped = 0;
				} else {
					if (ctx->env->run_wait == node) {
						ctx->env->run_wait = list_next(ctx->env->run_funcs, node);
					}
					list_delete(ctx->env->run_funcs, node);
					lnode_destroy(node);
					free(rf);
				}
			}
			node = next_node;
		}
		
		// If some run functions were in execution, wait for them to finish
		if (!stopped) {
			cpi_wait_context(ctx);
		}
	}
}
