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
#include "test.h"

void nocollections(void) {
	cp_context_t *ctx;
	cp_plugin_info_t **plugins;
	cp_status_t status;
	int errors;
	int i;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check((plugins = cp_get_plugins_info(ctx, &status, &i)) != NULL && status == CP_OK && i == 0);
	cp_release_info(ctx, plugins);
	cp_destroy();
	check(errors == 0);
}

void onecollection(void) {
	cp_context_t *ctx;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1")) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_INSTALLED);
	cp_destroy();
	check(errors == 0);
}

void twocollections(void) {
	cp_context_t *ctx;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1")) == CP_OK);
	check(cp_register_pcollection(ctx, pcollectiondir("collection2")) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2b") == CP_PLUGIN_INSTALLED);
	cp_destroy();
	check(errors == 0);
}

void unregcollection(void) {
	cp_context_t *ctx;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1")) == CP_OK);
	check(cp_register_pcollection(ctx, pcollectiondir("collection2")) == CP_OK);
	cp_unregister_pcollection(ctx, pcollectiondir("collection2"));
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_UNINSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2b") == CP_PLUGIN_UNINSTALLED);
	cp_destroy();
	check(errors == 0);
}

void unregcollections(void) {
	cp_context_t *ctx;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1")) == CP_OK);
	check(cp_register_pcollection(ctx, pcollectiondir("collection2")) == CP_OK);
	cp_unregister_pcollections(ctx);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_UNINSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_UNINSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2b") == CP_PLUGIN_UNINSTALLED);
	cp_destroy();
	check(errors == 0);
}
