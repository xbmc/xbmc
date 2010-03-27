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

#include <stdlib.h>
#include <cpluff.h>

typedef struct plugin_data_t plugin_data_t;

struct plugin_data_t {
	cp_context_t *ctx;
	char *str;
};

static void *create(cp_context_t *ctx) {
	plugin_data_t *data = malloc(sizeof(plugin_data_t));

	if (data != NULL) {
		data->ctx = ctx;
		data->str = NULL;
	}
	return data;
}

static int start(void *d) {
	plugin_data_t *data = d;
	
	if ((data->str = malloc(sizeof(char) * 16)) == NULL) {
		return CP_ERR_RESOURCE;
	}
	strcpy(data->str, "Provided string");
	cp_define_symbol(data->ctx, "sp_string", data->str);
	return CP_OK;
}

static void destroy(void *d) {
	plugin_data_t *data = d;
	
	if (data->str != NULL) {
		strcpy(data->str, "Cleared string");
		free(data->str);
	}
	free(d);	
}

CP_EXPORT cp_plugin_runtime_t sp_runtime = {
	create,
	start,
	NULL,
	destroy
};
