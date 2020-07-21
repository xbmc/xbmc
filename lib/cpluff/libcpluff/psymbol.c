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
 * Dynamic plug-in symbols
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../kazlib/hash.h"
#include "cpluff.h"
#include "defines.h"
#include "internal.h"
#include "util.h"
#ifdef _WIN32
#include <windows.h>
#endif

#define hash_lookup cpluff_hash_lookup

/* ------------------------------------------------------------------------
 * Data structures
 * ----------------------------------------------------------------------*/

/// Information about symbol providing plug-in
typedef struct symbol_provider_info_t {
	
	// The providing plug-in
	cp_plugin_t *plugin;
	
	// Whether there is also an import dependency for the plug-in
	int imported;
	
	// Total symbol usage count
	int usage_count;
	
} symbol_provider_info_t;

/// Information about used symbol
typedef struct symbol_info_t {

	// Symbol usage count
	int usage_count;
	
	// Information about providing plug-in
	symbol_provider_info_t *provider_info;
	
} symbol_info_t;


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

CP_C_API cp_status_t cp_define_symbol(cp_context_t *context, const char *name, void *ptr) {
	cp_status_t status = CP_OK;
	
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(name);
	CHECK_NOT_NULL(ptr);
	if (context->plugin == NULL) {
		cpi_fatalf(_("Only plug-ins can define context specific symbols."));
	}
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER | CPI_CF_LISTENER, __func__);
	do {
		char *n;
		
		// Create a symbol hash if necessary
		if (context->plugin->defined_symbols == NULL) {
			if ((context->plugin->defined_symbols = hash_create(HASHCOUNT_T_MAX, (int (*)(const void *, const void *)) strcmp, NULL)) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		
		// Check for a previously defined symbol
		if (hash_lookup(context->plugin->defined_symbols, name) != NULL) {
			status = CP_ERR_CONFLICT;
			break;
		}

		// Insert the symbol into the symbol hash
		n = strdup(name);
		if (n == NULL || !hash_alloc_insert(context->plugin->defined_symbols, n, ptr)) {
			free(n);
			status = CP_ERR_RESOURCE;
			break;
		} 

	} while (0);
	
	// Report error
	if (status != CP_OK) {
		switch (status) {
			case CP_ERR_RESOURCE:
				cpi_errorf(context, N_("Plug-in %s could not define symbol %s due to insufficient memory."), context->plugin->plugin->identifier, name);
				break;
			case CP_ERR_CONFLICT:
				cpi_errorf(context, N_("Plug-in %s tried to redefine symbol %s."), context->plugin->plugin->identifier, name);
				break;
			default:
				break;
		}
	}
	cpi_unlock_context(context);
	
	return status;
}

CP_C_API void * cp_resolve_symbol(cp_context_t *context, const char *id, const char *name, cp_status_t *error) {
	cp_status_t status = CP_OK;
	int error_reported = 1;
	hnode_t *node;
	void *symbol = NULL;
	symbol_info_t *symbol_info = NULL;
	symbol_provider_info_t *provider_info = NULL;
	cp_plugin_t *pp = NULL;

	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(id);
	CHECK_NOT_NULL(name);
	
	// Resolve the symbol
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER | CPI_CF_LISTENER | CPI_CF_STOP, __func__);
	do {

		// Allocate space for symbol hashes, if necessary
		if (context->resolved_symbols == NULL) {
			context->resolved_symbols = hash_create(HASHCOUNT_T_MAX, cpi_comp_ptr, cpi_hashfunc_ptr);
		}
		if (context->symbol_providers == NULL) {
			context->symbol_providers = hash_create(HASHCOUNT_T_MAX, cpi_comp_ptr, cpi_hashfunc_ptr);
		}
		if (context->resolved_symbols == NULL
			|| context->symbol_providers == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}

		// Look up the symbol defining plug-in
		node = hash_lookup(context->env->plugins, id);
		if (node == NULL) {
			cpi_warnf(context, N_("Symbol %s in unknown plug-in %s could not be resolved."), name, id);
			status = CP_ERR_UNKNOWN;
			break;
		}
		pp = hnode_get(node);

		// Make sure the plug-in has been started
		if ((status = cpi_start_plugin(context, pp)) != CP_OK) {
			cpi_errorf(context, N_("Symbol %s in plug-in %s could not be resolved because the plug-in could not be started."), name, id);
			error_reported = 1;
			break;
		}

		// Check for a context specific symbol
		if (pp->defined_symbols != NULL && (node = hash_lookup(pp->defined_symbols, name)) != NULL) {
			symbol = hnode_get(node);
		}

		// Fall back to global symbols, if necessary
		if (symbol == NULL && pp->runtime_lib != NULL) {
			symbol = DLSYM(pp->runtime_lib, name);
		}
		if (symbol == NULL) {
			const char *error = DLERROR();
			if (error == NULL) {
				error = _("Unspecified error.");
			}
			cpi_warnf(context, N_("Symbol %s in plug-in %s could not be resolved: %s"), name, id, error);
			status = CP_ERR_UNKNOWN;
			break;
		}

		// Lookup or initialize symbol provider information
		if ((node = hash_lookup(context->symbol_providers, pp)) != NULL) {
			provider_info = hnode_get(node);
		} else {
			if ((provider_info = malloc(sizeof(symbol_provider_info_t))) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			memset(provider_info, 0, sizeof(symbol_provider_info_t));
			provider_info->plugin = pp;
			provider_info->imported = (context->plugin == NULL || cpi_ptrset_contains(context->plugin->imported, pp));
			if (!hash_alloc_insert(context->symbol_providers, pp, provider_info)) {
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		
		// Lookup or initialize symbol information
		if ((node = hash_lookup(context->resolved_symbols, symbol)) != NULL) {
			symbol_info = hnode_get(node);
		} else {
			if ((symbol_info = malloc(sizeof(symbol_info_t))) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			memset(symbol_info, 0, sizeof(symbol_info_t));
			symbol_info->provider_info = provider_info;
			if (!hash_alloc_insert(context->resolved_symbols, symbol, symbol_info)) {
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		
		// Add dependencies (for plug-in)
		if (provider_info != NULL
			&& !provider_info->imported
			&& provider_info->usage_count == 0) {
			if (!cpi_ptrset_add(context->plugin->imported, pp)) {
				status = CP_ERR_RESOURCE;
				break;
			}
			if (!cpi_ptrset_add(pp->importing, context->plugin)) {
				cpi_ptrset_remove(context->plugin->imported, pp);
				status = CP_ERR_RESOURCE;
				break;
			}
			cpi_debugf(context, N_("A dynamic dependency was created from plug-in %s to plug-in %s."), context->plugin->plugin->identifier, pp->plugin->identifier);
		}
		
		// Increase usage counts
		symbol_info->usage_count++;
		provider_info->usage_count++;

		if (cpi_is_logged(context, CP_LOG_DEBUG)) {
			char owner[64];
			/* TRANSLATORS: First %s is the context owner */
			cpi_debugf(context, N_("%s resolved symbol %s defined by plug-in %s."), cpi_context_owner(context, owner, sizeof(owner)), name, id);
		}
	} while (0);

	// Clean up
	if (symbol_info != NULL && symbol_info->usage_count == 0) {
		if ((node = hash_lookup(context->resolved_symbols, symbol)) != NULL) {
			hash_delete_free(context->resolved_symbols, node);
		}
		free(symbol_info);
	}
	if (provider_info != NULL && provider_info->usage_count == 0) {
		if ((node = hash_lookup(context->symbol_providers, pp)) != NULL) {
			hash_delete_free(context->symbol_providers, node);
		}
		free(provider_info);
	}

	// Report insufficient memory error
	if (status == CP_ERR_RESOURCE && !error_reported) {
		cpi_errorf(context, N_("Symbol %s in plug-in %s could not be resolved due to insufficient memory."), name, id);
	}
	cpi_unlock_context(context);

	// Return error code
	if (error != NULL) {
		*error = status;
	}
	
	// Return symbol
	return symbol;
}

CP_C_API void cp_release_symbol(cp_context_t *context, const void *ptr) {
	hnode_t *node;
	symbol_info_t *symbol_info;
	symbol_provider_info_t *provider_info;
	
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(ptr);

	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER | CPI_CF_LISTENER, __func__);
	do {

		// Look up the symbol
		if ((node = hash_lookup(context->resolved_symbols, ptr)) == NULL) {
			cpi_errorf(context, N_("Could not release unknown symbol at address %p."), ptr);
			break;
		}
		symbol_info = hnode_get(node);
		provider_info = symbol_info->provider_info;
	
		// Decrease usage count
		assert(symbol_info->usage_count > 0);
		symbol_info->usage_count--;
		assert(provider_info->usage_count > 0);
		provider_info->usage_count--;
	
		// Check if the symbol is not being used anymore
		if (symbol_info->usage_count == 0) {
			hash_delete_free(context->resolved_symbols, node);
			free(symbol_info);
			if (cpi_is_logged(context, CP_LOG_DEBUG)) {
				char owner[64];
				/* TRANSLATORS: First %s is the context owner */
				cpi_debugf(context, N_("%s released the symbol at address %p defined by plug-in %s."), cpi_context_owner(context, owner, sizeof(owner)), ptr, provider_info->plugin->plugin->identifier);
			}
		}
	
		// Check if the symbol providing plug-in is not being used anymore
		if (provider_info->usage_count == 0) {
			node = hash_lookup(context->symbol_providers, provider_info->plugin);
			assert(node != NULL);
			hash_delete_free(context->symbol_providers, node);
			if (!provider_info->imported) {
				cpi_ptrset_remove(context->plugin->imported, provider_info->plugin);
				cpi_ptrset_remove(provider_info->plugin->importing, context->plugin);
				cpi_debugf(context, N_("A dynamic dependency from plug-in %s to plug-in %s was removed."), context->plugin->plugin->identifier, provider_info->plugin->plugin->identifier);
			}
			free(provider_info);
		}
		
	} while (0);
	cpi_unlock_context(context);
}
