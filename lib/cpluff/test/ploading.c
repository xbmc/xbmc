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
#include "test.h"

#define MEMBUFFERSIZE 16384
#define PLUGINFILENAME "addon.xml"

void loadonlymaximal(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("maximal"), &status)) != NULL && status == CP_OK);
	cp_release_info(ctx, plugin);
	cp_destroy();
	check(errors == 0);
}

void loadonlymaximalfrommemory(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	int errors, bytesloaded = 0, i;
	const char *pd;
	char *pdfilename, *membuffer;
	FILE *f;

	/* Load plugin descriptor to memory buffer */
	pd = plugindir("maximal");
	check((pdfilename = malloc((strlen(pd) + strlen(CP_FNAMESEP_STR) + strlen(PLUGINFILENAME) + 1) * sizeof(char))) != NULL);
	strcpy(pdfilename, pd);
	strcat(pdfilename, CP_FNAMESEP_STR PLUGINFILENAME);
	check((membuffer = malloc(MEMBUFFERSIZE * sizeof(unsigned char))) != NULL);
	check((f = fopen(pdfilename, "r")) != NULL);
	do {
		i = fread(membuffer + bytesloaded, sizeof(char), MEMBUFFERSIZE - bytesloaded, f);
		check(!ferror(f));
		bytesloaded += i;
	} while (i > 0);
	fclose(f);
	
	/* Load plugin descriptor from memory buffer */
	ctx = init_context(CP_LOG_ERROR, &errors);
	check((plugin = cp_load_plugin_descriptor_from_memory(ctx, membuffer, bytesloaded, &status)) != NULL && status == CP_OK);

	/* Clean up */
	free(membuffer);
	cp_release_info(ctx, plugin);
	cp_destroy();
	check(errors == 0);
}
