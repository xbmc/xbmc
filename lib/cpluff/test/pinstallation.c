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

void install(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_INSTALLED);	
	cp_destroy();
	check(errors == 0);
}

void installtwo(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "maximal") == CP_PLUGIN_UNINSTALLED);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("maximal"), &status)) != NULL && status == CP_OK);
	check(cp_get_plugin_state(ctx, "maximal") == CP_PLUGIN_UNINSTALLED);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);		
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "maximal") == CP_PLUGIN_INSTALLED);
	cp_destroy();
	check(errors == 0);	
}

void installconflict(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	
	ctx = init_context(CP_LOG_ERROR + 1, NULL);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_INSTALLED);	
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_install_plugin(ctx, plugin) == CP_ERR_CONFLICT);
	cp_release_info(ctx, plugin);
	cp_destroy();
}

void uninstall(void) {
	cp_context_t *ctx;
	cp_plugin_info_t *plugin;
	cp_status_t status;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check((plugin = cp_load_plugin_descriptor(ctx, plugindir("minimal"), &status)) != NULL && status == CP_OK);
	check(cp_install_plugin(ctx, plugin) == CP_OK);
	cp_release_info(ctx, plugin);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_INSTALLED);
	check(cp_uninstall_plugin(ctx, "minimal") == CP_OK);
	check(cp_get_plugin_state(ctx, "minimal") == CP_PLUGIN_UNINSTALLED);	
	cp_destroy();
	check(errors == 0);	
}
