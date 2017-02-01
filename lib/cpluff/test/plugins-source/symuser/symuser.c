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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpluff.h>

typedef struct plugin_data_t plugin_data_t;

struct plugin_data_t {
	cp_context_t *ctx;
	const char *str;
};

static void *create(cp_context_t *ctx) {
	plugin_data_t *data;
	
	if ((data = malloc(sizeof(plugin_data_t))) != NULL) {
		data->ctx = ctx;
		data->str = NULL;
	}
	return data;
}

static int start(void *d) {
	plugin_data_t *data = d;
	cp_extension_t **exts;
	
	exts = cp_get_extensions_info(data->ctx, "symuser.strings", NULL, NULL);
	if (exts != NULL && exts[0] != NULL) {
		const char *symname;
		
		symname = cp_lookup_cfg_value(exts[0]->configuration, "@string-symbol");
		if (symname != NULL) {
			data->str = cp_resolve_symbol(data->ctx, exts[0]->plugin->identifier, symname, NULL);
			if (data->str == NULL) {
				cp_log(data->ctx, CP_LOG_ERROR, "Could not resolve symbol specified by extension.");
			}
		} else {
			cp_log(data->ctx, CP_LOG_ERROR, "No string-symbol attribute present in extension.");
		} 
	} else {
		cp_log(data->ctx, CP_LOG_ERROR, "No extensions available.");
	}
	if (exts != NULL) {
		cp_release_info(data->ctx, exts);
	}
	if (data->str == NULL) {
		return CP_ERR_RUNTIME;
	}
	return cp_define_symbol(data->ctx, "used_string", (void *) data->str);
}

static void stop(void *d) {
	plugin_data_t *data = d;
	
	// Check that the provided string is still available
	if (data->str != NULL) {
		if (strcmp(data->str, "Provided string")) {
			fputs("Provided string is not available in symuser stop function.\n", stderr);
			abort();
		}
		cp_release_symbol(data->ctx, data->str);
	}
}

static void destroy(void *d) {
	free(d);
}

CP_EXPORT cp_plugin_runtime_t su_runtime = {
	create,
	start,
	stop,
	destroy
};
