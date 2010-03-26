/*
*      Copyright (C) 2010 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#include "core.h"

/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/


/** Type for xbmc_vfs_ops structure */
typedef struct xbmc_vfs_ops *vfsprovider_t;

/** Plug-in instance data */
struct plugin_data_t {
	
	/** The plug-in context */
	cp_context_t *ctx;
	
	/** Number of registered providers */
	int num_providers;
	
	/** An array of registered file system providers */
	registered_vfsprovider_t *providers; 
};

typedef struct plugin_data_t plugin_data_t;

/** Registered vfs addon info */
struct registered_vfsprovider_t {
	const char *protocol;
	xbmc_vfs_ops *ops;
};


/* ------------------------------------------------------------------------
 * Internal functions
 * ----------------------------------------------------------------------*/

/**
 * A run function for the core plug-in. In this case this function acts as
 * the application main function so there is no need for us to split the
 * execution into small steps. Rather, we execute the whole main loop at
 * once to make it simpler.
 */
static int run(void *d) {
	plugin_data_t *data = d;
	char **argv;
	int argc;
	int i;

	// Go through all files listed as command arguments
	for (i = 1; argv[i] != NULL; i++) {
		int j;
		int classified = 0;
		
		// Print file name
		printf("%s: ", argv[i]);
		
		// Try providers in order of descending priority
		for (j = 0; !classified && j < data->num_providers; j++) {
			vfsprovider_t *cl
				= data->providers[j].provider;
				
			classified = cl->direxists(cl->data, argv[i]);
		}
		
		// Check if unknown file
		if (!classified) {
			fputs("unknown file type\n", stdout);
		}
	}
	
	// All done
	return 0;
} 

/**
 * Creates a new plug-in instance.
 */
static void *create(cp_context_t *ctx) {
	plugin_data_t *data = malloc(sizeof(plugin_data_t));
	if (data != NULL) {
		data->ctx = ctx;
		data->num_providers = 0;
		data->providers = NULL;
	} else {
		cp_log(ctx, CP_LOG_ERROR,
			"Insufficient memory for plug-in data.");
	}
	return data;
}

/**
 * Initializes and starts the plug-in.
 */
static int start(void *d) {
	plugin_data_t *data = d;
	cp_extension_t **pr_exts;
	int num_pr_exts;
	cp_status_t status;
	int i;
	
	// Obtain list of registered file system providers 
	pr_exts = cp_get_extensions_info(
		data->ctx,
		"org.xbmc.vfs.providers",
		&status,
		&num_pr_exts
	);
	if (pr_exts == NULL) {
		
		// An error occurred and framework logged it
		return status;
	}
	
	// Allocate memory for vfsprovider information, if any
	if (num_pr_exts > 0) {
		data->providers = malloc(
			num_pr_exts * sizeof(registered_vfsprovider_t)
		);
		if (data->providers == NULL) {
			// Memory allocation failed
			cp_log(data->ctx, CP_LOG_ERROR,
				"Insufficient memory for providers list.");
			return CP_ERR_RESOURCE;
		}
	} 
	
	/* Resolve providers functions. This will implicitly start
	 * plug-ins providing the file systems. */
	for (i = 0; i < num_pr_exts; i++) {
		const char *str;
		vfsprovider_t *pr;
		
		// Resolve provider data pointer
		str = cp_lookup_cfg_value(
			pr_exts[i]->configuration, "@provider");
		if (str == NULL) {
			
			// Provider symbol name is missing
			cp_log(data->ctx, CP_LOG_ERROR,
				"Ignoring vfsprovider without symbol name.");
			continue;
		}
		pr = cp_resolve_symbol(
			data->ctx,
			pr_exts[i]->plugin->identifier,
			str,
			NULL
		);
		if (pr == NULL) {
			
			// Could not resolve provider symbol
			cp_log(data->ctx, CP_LOG_ERROR,
				"Ignoring provider which could not be resolved.");
			continue;
		}
		
		// Get the protocol supported
		str = cp_lookup_cfg_value(
			pr_exts[i]->configuration, "@protocol"
		);
		if (str == NULL) {
			
			// provider is missing mandatory protocol 
			cp_log(data->ctx, CP_LOG_ERROR,
				"Ignoring provider without protocol.");
			continue;

		// Add provider to the list of registered providers
		data->providers[data->num_providers].protocol = str;
		data->providers[data->num_providers].provider = pr;
		data->num_providers++;
		}
	}
	
	// Release extension information
	cp_release_info(data->ctx, pr_exts);
	
	// Register run function to do the real work
	cp_run_function(data->ctx, run);
	
  if (data->num_providers) {
    cp_log(data->ctx, CP_LOG_DEBUG,
      "VFS: NO providers %i");
  } else {
    cp_log(data->ctx, CP_LOG_DEBUG,
      "VFS: some providers %i");
  }

	// Successfully started
	return CP_OK;
}

/**
 * Releases resources from other plug-ins.
 */
static void stop(void *d) {
	plugin_data_t *data = d;
	int i;
	
	if (data->providers != NULL) {
		for (i = 0; i < data->num_providers; i++) {
			cp_release_symbol(
				data->ctx, data->providers[i].provider
			);
		}
		
		// Free local data
		free(data->providers);
		data->providers = NULL;
		data->num_providers = 0;
	}
}

/**
 * Destroys a plug-in instance.
 */
static void destroy(void *d) {
	free(d);
}


/* ------------------------------------------------------------------------
 * Exported runtime information
 * ----------------------------------------------------------------------*/

/**
 * Plug-in runtime information for the framework. The name of this symbol
 * is stored in the plug-in descriptor.
 */
CP_EXPORT cp_plugin_runtime_t xbmc_vfs_providers_core_funcs = {
	create,
	start,
	stop,
	destroy
};
