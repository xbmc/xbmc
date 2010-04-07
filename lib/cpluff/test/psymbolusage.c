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

void symbolusage(void) {
	cp_context_t *ctx;
	cp_status_t status;
	int errors;
	const char *str;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_pcollection(ctx, "tmp/install/plugins") == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	
	// Start plug-in implicitly by resolving a symbol
	check((str = cp_resolve_symbol(ctx, "symuser", "used_string", &status)) != NULL && status == CP_OK);
	
	// Compare used string to the provided string
	check(strcmp(str, "Provided string") == 0);
	
	// Release string
	cp_release_symbol(ctx, str);

	// Shutdown framework
	cp_destroy();
	check(errors == 0);
}
