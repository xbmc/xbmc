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
 * Plug-in information functions
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "../kazlib/hash.h"
#include "cpluff.h"
#include "defines.h"
#include "util.h"
#include "internal.h"


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/// Registration of a dynamically allocated information object
typedef struct info_resource_t {

	/// Pointer to the resource
	void *resource;	

	/// Usage count for the resource
	int usage_count;
	
	/// Deallocation function
	cpi_dealloc_func_t dealloc_func;
	
} info_resource_t;

/// A plug-in listener registration
typedef struct el_holder_t {
	
	/// The plug-in listener
	cp_plugin_listener_func_t plugin_listener;
	
	/// The registering plug-in or NULL for the client program
	cp_plugin_t *plugin;
	
	/// Associated user data
	void *user_data;
	
} el_holder_t;



/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

// General information object management

CP_HIDDEN cp_status_t cpi_register_info(cp_context_t *context, void *res, cpi_dealloc_func_t df) {
	cp_status_t status = CP_OK;
	info_resource_t *ir = NULL;

	assert(context != NULL);
	assert(res != NULL);
	assert(df != NULL);
	assert(cpi_is_context_locked(context));
	do {
		if ((ir = malloc(sizeof(info_resource_t))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		ir->resource = res;
		ir->usage_count = 1;
		ir->dealloc_func = df;
		if (!hash_alloc_insert(context->env->infos, res, ir)) {
			status = CP_ERR_RESOURCE;
			break;
		}
	} while (0);
	
	// Report success
	if (status == CP_OK) {
		cpi_debugf(context, _("An information object at address %p was registered."), res);
	}		
	
	// Release resources on failure
	if (status != CP_OK) {
		if (ir != NULL) {
			free(ir);
		}
	}
	
	return status;
}

CP_HIDDEN void cpi_use_info(cp_context_t *context, void *res) {
	hnode_t *node;
	
	assert(context != NULL);
	assert(res != NULL);
	assert(cpi_is_context_locked(context));
	if ((node = hash_lookup(context->env->infos, res)) != NULL) {
		info_resource_t *ir = hnode_get(node);
		ir->usage_count++;
		cpi_debugf(context, _("Reference count of the information object at address  %p increased to %d."), res, ir->usage_count);
	} else {
		cpi_fatalf(_("Reference count of an unknown information object at address %p could not be increased."), res);
	}
}

CP_HIDDEN void cpi_release_info(cp_context_t *context, void *info) {
	hnode_t *node;
	
	assert(context != NULL);
	assert(info != NULL);
	assert(cpi_is_context_locked(context));
	if ((node = hash_lookup(context->env->infos, info)) != NULL) {
		info_resource_t *ir = hnode_get(node);
		assert(ir != NULL && info == ir->resource);
		if (--ir->usage_count == 0) {
			hash_delete_free(context->env->infos, node);
			ir->dealloc_func(context, info);
			cpi_debugf(context, _("The information object at address %p was unregistered."), info);
			free(ir);
		} else {
			cpi_debugf(context, _("Reference count of the information object at address %p decreased to %d."), info, ir->usage_count);
		}
	} else {
		cpi_fatalf(_("Could not release an unknown information object at address %p."), info);
	}
}

CP_C_API void cp_release_info(cp_context_t *context, void *info) {
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(info);
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	cpi_release_info(context, info);
	cpi_unlock_context(context);
}

CP_HIDDEN void cpi_release_infos(cp_context_t *context) {
	hscan_t scan;
	hnode_t *node;
		
	hash_scan_begin(&scan, context->env->infos);
	while ((node = hash_scan_next(&scan)) != NULL) {
		info_resource_t *ir = hnode_get(node);			
		cpi_lock_context(context);
		cpi_errorf(context, _("An unreleased information object was encountered at address %p with reference count %d when destroying the associated plug-in context. Not releasing the object."), ir->resource, ir->usage_count);
		cpi_unlock_context(context);
		hash_scan_delfree(context->env->infos, node);
		free(ir);
	}
}


// Information acquiring functions

CP_C_API cp_plugin_info_t * cp_get_plugin_info(cp_context_t *context, const char *id, cp_status_t *error) {
	hnode_t *node;
	cp_plugin_info_t *plugin = NULL;
	cp_status_t status = CP_OK;

	CHECK_NOT_NULL(context);
	if (id == NULL && context->plugin == NULL) {
		cpi_fatalf(_("The plug-in identifier argument to cp_get_plugin_info must not be NULL when the main program calls it."));
	}

	// Look up the plug-in and return information 
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	do {
		
		// Lookup plug-in information
		if (id != NULL) {
			if ((node = hash_lookup(context->env->plugins, id)) == NULL) {
				cpi_warnf(context, N_("Could not return information about unknown plug-in %s."), id);
				status = CP_ERR_UNKNOWN;
				break;
			}
			plugin = ((cp_plugin_t *) hnode_get(node))->plugin;
		} else {
			plugin = context->plugin->plugin;
			assert(plugin != NULL);
		}
		cpi_use_info(context, plugin);
	} while (0);
	cpi_unlock_context(context);

	if (error != NULL) {
		*error = status;
	}
	return plugin;
}

static void dealloc_plugins_info(cp_context_t *context, cp_plugin_info_t **plugins) {
	int i;
	
	assert(context != NULL);
	assert(plugins != NULL);
	for (i = 0; plugins[i] != NULL; i++) {
		cpi_release_info(context, plugins[i]);
	}
	free(plugins);
}

CP_C_API cp_plugin_info_t ** cp_get_plugins_info(cp_context_t *context, cp_status_t *error, int *num) {
	cp_plugin_info_t **plugins = NULL;
	int i, n;
	cp_status_t status = CP_OK;
	
	CHECK_NOT_NULL(context);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	do {
		hscan_t scan;
		hnode_t *node;
		
		// Allocate space for pointer array 
		n = hash_count(context->env->plugins);
		if ((plugins = malloc(sizeof(cp_plugin_info_t *) * (n + 1))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Get plug-in information structures 
		hash_scan_begin(&scan, context->env->plugins);
		i = 0;
		while ((node = hash_scan_next(&scan)) != NULL) {
			cp_plugin_t *rp = hnode_get(node);
			
			assert(i < n);
			cpi_use_info(context, rp->plugin);
			plugins[i] = rp->plugin;
			i++;
		}
		plugins[i] = NULL;
		
		// Register the array
		status = cpi_register_info(context, plugins, (void (*)(cp_context_t *, void *)) dealloc_plugins_info);
		
	} while (0);

	// Report error
	if (status != CP_OK) {
		cpi_error(context, N_("Plug-in information could not be returned due to insufficient memory."));
	}
	cpi_unlock_context(context);

	// Release resources on error 
	if (status != CP_OK) {
		if (plugins != NULL) {
			dealloc_plugins_info(context, plugins);
			plugins = NULL;
		}
	}
	
	assert(status != CP_OK || n == 0 || plugins[n - 1] != NULL);
	if (error != NULL) {
		*error = status;
	}
	if (num != NULL && status == CP_OK) {
		*num = n;
	}
	return plugins;
}

CP_C_API cp_plugin_state_t cp_get_plugin_state(cp_context_t *context, const char *id) {
	cp_plugin_state_t state = CP_PLUGIN_UNINSTALLED;
	hnode_t *hnode;
	
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(id);
	
	// Look up the plug-in state 
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	if ((hnode = hash_lookup(context->env->plugins, id)) != NULL) {
		cp_plugin_t *rp = hnode_get(hnode);
		state = rp->state;
	}
	cpi_unlock_context(context);
	return state;
}

static void dealloc_ext_points_info(cp_context_t *context, cp_ext_point_t **ext_points) {
	int i;
	
	assert(context != NULL);
	assert(ext_points != NULL);
	for (i = 0; ext_points[i] != NULL; i++) {
		cpi_release_info(context, ext_points[i]->plugin);
	}
	free(ext_points);
}

CP_C_API cp_ext_point_t ** cp_get_ext_points_info(cp_context_t *context, cp_status_t *error, int *num) {
	cp_ext_point_t **ext_points = NULL;
	int i, n;
	cp_status_t status = CP_OK;
	
	CHECK_NOT_NULL(context);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	do {
		hscan_t scan;
		hnode_t *node;
		
		// Allocate space for pointer array 
		n = hash_count(context->env->ext_points);
		if ((ext_points = malloc(sizeof(cp_ext_point_t *) * (n + 1))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Get extension point information structures 
		hash_scan_begin(&scan, context->env->ext_points);
		i = 0;
		while ((node = hash_scan_next(&scan)) != NULL) {
			cp_ext_point_t *ep = hnode_get(node);
			
			assert(i < n);
			cpi_use_info(context, ep->plugin);
			ext_points[i] = ep;
			i++;
		}
		ext_points[i] = NULL;
		
		// Register the array
		status = cpi_register_info(context, ext_points, (void (*)(cp_context_t *, void *)) dealloc_ext_points_info);
		
	} while (0);
	
	// Report error
	if (status != CP_OK) {
		cpi_error(context, N_("Extension point information could not be returned due to insufficient memory."));
	}
	cpi_unlock_context(context);
	
	// Release resources on error 
	if (status != CP_OK) {
		if (ext_points != NULL) {
			dealloc_ext_points_info(context, ext_points);
			ext_points = NULL;
		}
	}
	
	assert(status != CP_OK || n == 0 || ext_points[n - 1] != NULL);
	if (error != NULL) {
		*error = status;
	}
	if (num != NULL && status == CP_OK) {
		*num = n;
	}
	return ext_points;
}

static void dealloc_extensions_info(cp_context_t *context, cp_extension_t **extensions) {
	int i;
	
	assert(context != NULL);
	assert(extensions != NULL);
	for (i = 0; extensions[i] != NULL; i++) {
		cpi_release_info(context, extensions[i]->plugin);
	}
	free(extensions);
}

CP_C_API cp_extension_t ** cp_get_extensions_info(cp_context_t *context, const char *extpt_id, cp_status_t *error, int *num) {
	cp_extension_t **extensions = NULL;
	int i, n;
	cp_status_t status = CP_OK;
	
	CHECK_NOT_NULL(context);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	do {
		hscan_t scan;
		hnode_t *hnode;

		// Count the number of extensions
		if (extpt_id != NULL) {
			if ((hnode = hash_lookup(context->env->extensions, extpt_id)) != NULL) {
				n = list_count((list_t *) hnode_get(hnode));
			} else {
				n = 0;
			}
		} else {
			hscan_t scan;
			
			n = 0;
			hash_scan_begin(&scan, context->env->extensions);
			while ((hnode = hash_scan_next(&scan)) != NULL) {
				n += list_count((list_t *) hnode_get(hnode));
			}
		}
		
		// Allocate space for pointer array 
		if ((extensions = malloc(sizeof(cp_extension_t *) * (n + 1))) == NULL) {
			status = CP_ERR_RESOURCE;
			break;
		}
		
		// Get extension information structures
		if (extpt_id != NULL) {
			i = 0;
			if ((hnode = hash_lookup(context->env->extensions, extpt_id)) != NULL) {
				list_t *el = hnode_get(hnode);
				lnode_t *lnode;
				
				lnode = list_first(el);
				while (lnode != NULL) {
					cp_extension_t *e = lnode_get(lnode);
				
					assert(i < n);
					cpi_use_info(context, e->plugin);
					extensions[i] = e;
					i++;
					lnode = list_next(el, lnode);
				}
			}
			extensions[i] = NULL;
		} else { 
			hash_scan_begin(&scan, context->env->extensions);
			i = 0;
			while ((hnode = hash_scan_next(&scan)) != NULL) {
				list_t *el = hnode_get(hnode);
				lnode_t *lnode;
			
				lnode = list_first(el);
				while (lnode != NULL) {
					cp_extension_t *e = lnode_get(lnode);
				
					assert(i < n);
					cpi_use_info(context, e->plugin);
					extensions[i] = e;
					i++;
					lnode = list_next(el, lnode);
				}
			}
		}
		extensions[i] = NULL;
		
		// Register the array
		status = cpi_register_info(context, extensions, (void (*)(cp_context_t *, void *)) dealloc_extensions_info);
		
	} while (0);
	
	// Report error
	if (status != CP_OK) {
		cpi_error(context, N_("Extension information could not be returned due to insufficient memory."));
	}
	cpi_unlock_context(context);
	
	// Release resources on error 
	if (status != CP_OK) {
		if (extensions != NULL) {
			dealloc_extensions_info(context, extensions);
			extensions = NULL;
		}
	}
	
	assert(status != CP_OK || n == 0 || extensions[n - 1] != NULL);
	if (error != NULL) {
		*error = status;
	}
	if (num != NULL && status == CP_OK) {
		*num = n;
	}
	return extensions;
}


// Plug-in listeners 

/**
 * Compares plug-in listener holders.
 * 
 * @param h1 the first holder to be compared
 * @param h2 the second holder to be compared
 * @return zero if the holders point to the same function, otherwise non-zero
 */
static int comp_el_holder(const void *h1, const void *h2) {
	const el_holder_t *plh1 = h1;
	const el_holder_t *plh2 = h2;
	
	return (plh1->plugin_listener != plh2->plugin_listener);
}

/**
 * Processes a node by delivering the specified event to the associated
 * plug-in listener.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param event the event
 */
static void process_event(list_t *list, lnode_t *node, void *event) {
	el_holder_t *h = lnode_get(node);
	cpi_plugin_event_t *e = event;
	h->plugin_listener(e->plugin_id, e->old_state, e->new_state, h->user_data);
}

/**
 * Processes a node by unregistering the associated plug-in listener.
 * 
 * @param list the list being processed
 * @param node the node being processed
 * @param plugin plugin whose listeners are to be unregistered or NULL for all
 */
static void process_unregister_plistener(list_t *list, lnode_t *node, void *plugin) {
	el_holder_t *h = lnode_get(node);
	if (plugin == NULL || h->plugin == plugin) {
		list_delete(list, node);
		lnode_destroy(node);
		free(h);
	}
}

CP_HIDDEN void cpi_unregister_plisteners(list_t *listeners, cp_plugin_t *plugin) {
	list_process(listeners, plugin, process_unregister_plistener);
}

CP_C_API cp_status_t cp_register_plistener(cp_context_t *context, cp_plugin_listener_func_t listener, void *user_data) {
	cp_status_t status = CP_ERR_RESOURCE;
	el_holder_t *holder;
	lnode_t *node;

	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(listener);
	
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER | CPI_CF_LISTENER, __func__);
	if ((holder = malloc(sizeof(el_holder_t))) != NULL) {
		holder->plugin_listener = listener;
		holder->plugin = context->plugin;
		holder->user_data = user_data;
		if ((node = lnode_create(holder)) != NULL) {
			list_append(context->env->plugin_listeners, node);
			status = CP_OK;
		} else {
			free(holder);
		}
	}
	
	// Report error or success
	if (status != CP_OK) {
		cpi_error(context, _("A plug-in listener could not be registered due to insufficient memory."));
	} else if (cpi_is_logged(context, CP_LOG_DEBUG)) {
		char owner[64];
		/* TRANSLATORS: %s is the context owner */
		cpi_debugf(context, N_("%s registered a plug-in listener."), cpi_context_owner(context, owner, sizeof(owner)));
	}
	cpi_unlock_context(context);
	
	return status;
}

CP_C_API void cp_unregister_plistener(cp_context_t *context, cp_plugin_listener_func_t listener) {
	el_holder_t holder;
	lnode_t *node;
	
	CHECK_NOT_NULL(context);
	holder.plugin_listener = listener;
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER | CPI_CF_LISTENER, __func__);
	node = list_find(context->env->plugin_listeners, &holder, comp_el_holder);
	if (node != NULL) {
		process_unregister_plistener(context->env->plugin_listeners, node, NULL);
	}
	if (cpi_is_logged(context, CP_LOG_DEBUG)) {
		char owner[64];
		/* TRANSLATORS: %s is the context owner */
		cpi_debugf(context, N_("%s unregistered a plug-in listener."), cpi_context_owner(context, owner, sizeof(owner)));
	}
	cpi_unlock_context(context);
}

CP_HIDDEN void cpi_deliver_event(cp_context_t *context, const cpi_plugin_event_t *event) {
	assert(event != NULL);
	assert(event->plugin_id != NULL);
	cpi_lock_context(context);
	context->env->in_event_listener_invocation++;
	list_process(context->env->plugin_listeners, (void *) event, process_event);
	context->env->in_event_listener_invocation--;
	cpi_unlock_context(context);
	if (cpi_is_logged(context, CP_LOG_INFO)) {
		char *str;
		switch (event->new_state) {
			case CP_PLUGIN_UNINSTALLED:
				str = N_("Plug-in %s has been uninstalled.");
				break;
			case CP_PLUGIN_INSTALLED:
				if (event->old_state < CP_PLUGIN_INSTALLED) {
					str = N_("Plug-in %s has been installed.");
				} else {
					str = N_("Plug-in %s runtime library has been unloaded.");
				}
				break;
			case CP_PLUGIN_RESOLVED:
				if (event->old_state < CP_PLUGIN_RESOLVED) {
					str = N_("Plug-in %s runtime library has been loaded.");
				} else {
					str = N_("Plug-in %s has been stopped.");
				}
				break;
			case CP_PLUGIN_STARTING:
				str = N_("Plug-in %s is starting.");
				break;
			case CP_PLUGIN_STOPPING:
				str = N_("Plug-in %s is stopping.");
				break;
			case CP_PLUGIN_ACTIVE:
				str = N_("Plug-in %s has been started.");
				break;
			default:
				str = NULL;
				break;
		}
		if (str != NULL) {
			cpi_infof(context, str, event->plugin_id);
		}
	}
}


// Configuration element helpers

static cp_cfg_element_t * lookup_cfg_element(cp_cfg_element_t *base, const char *path, int len) {
	int start = 0;
	
	CHECK_NOT_NULL(base);
	CHECK_NOT_NULL(path);
	
	// Traverse the path
	while (base != NULL && path[start] != '\0' && (len == -1 || start < len)) {
		int end = start;
		while (path[end] != '\0' && path[end] != '/' && (len == -1 || end < len))
			end++;
		if (end - start == 2 && !strncmp(path + start, "..", 2)) {
			base = base->parent;
		} else {
			int i;
			int found = 0;
			
			for (i = 0; !found && i < base->num_children; i++) {
				cp_cfg_element_t *e = base->children + i;
				if (end - start == strlen(e->name)
					&& !strncmp(path + start, e->name, end - start)) {
					base = e;
					found = 1;
				}
			}
			if (!found) {
				base = NULL;
			}
		}
		start = end;
		if (path[start] == '/') {
			start++;
		}
	}
	return base;
}

CP_C_API cp_cfg_element_t * cp_lookup_cfg_element(cp_cfg_element_t *base, const char *path) {
	return lookup_cfg_element(base, path, -1);
}

CP_C_API char * cp_lookup_cfg_value(cp_cfg_element_t *base, const char *path) {
	cp_cfg_element_t *e;
	const char *attr;
	
	CHECK_NOT_NULL(base);
	CHECK_NOT_NULL(path);
	
	if ((attr = strrchr(path, '@')) == NULL) {
		e = lookup_cfg_element(base, path, -1);
	} else {
		e = lookup_cfg_element(base, path, attr - path);
		attr++;
	}
	if (e != NULL) {
		if (attr == NULL) {
			return e->value;
		} else {
			int i;
			
			for (i = 0; i < e->num_atts; i++) {
				if (!strcmp(attr, e->atts[2*i])) {
					return e->atts[2*i + 1];
				}
			}
			return NULL;
		}
	} else {
		return NULL;
	}
}
