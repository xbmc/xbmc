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
#include <string.h>
#include "test.h"

/*
 * Basic plug-in scanning tests were already performed in collections.c.
 * Here we test some more complex things like upgrade and restart behavior.
 */

static void scanupgrade_checkpver(cp_context_t *ctx, const char *plugin, const char *ver) {
	cp_plugin_info_t *pi;
	cp_status_t status;
	
	check((pi = cp_get_plugin_info(ctx, plugin, &status)) != NULL && status == CP_OK);
	check(ver == NULL ? pi->version == NULL : (pi->version != NULL && strcmp(pi->version, ver) == 0));
	cp_release_info(ctx, pi); 
}

void scanupgrade(void) {
	cp_context_t *ctx;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1")) == CP_OK);
	check(cp_register_pcollection(ctx, pcollectiondir("collection2")) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2b") == CP_PLUGIN_INSTALLED);
	scanupgrade_checkpver(ctx, "plugin1", NULL);
	
	// Register newer version of plugin1 but do not allow upgrades
	check(cp_start_plugin(ctx, "plugin1") == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_ACTIVE);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1v2")) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_ACTIVE);
	scanupgrade_checkpver(ctx, "plugin1", NULL);

	// Now allow upgrade of plugin1
	check(cp_scan_plugins(ctx, CP_SP_UPGRADE) == CP_OK);
	scanupgrade_checkpver(ctx, "plugin1", "2");
	
	// Register even new version and upgrade while running
	check(cp_register_pcollection(ctx, pcollectiondir("collection1v3")) == CP_OK);
	check(cp_start_plugin(ctx, "plugin1") == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_ACTIVE);
	check(cp_scan_plugins(ctx, CP_SP_UPGRADE) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_INSTALLED);
	scanupgrade_checkpver(ctx, "plugin1", "3");
	
	// Check that plug-in is not downgraded when newer versions are unregistered
	cp_unregister_pcollection(ctx, pcollectiondir("collection1v3"));
	check(cp_scan_plugins(ctx, CP_SP_UPGRADE) == CP_OK);
	scanupgrade_checkpver(ctx, "plugin1", "3");

	cp_destroy();
	check(errors == 0);
}

void scanstoponupgrade(void) {
	cp_context_t *ctx;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1")) == CP_OK);
	check(cp_register_pcollection(ctx, pcollectiondir("collection2")) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	
	// First check upgrade without stopping other plug-ins
	check(cp_start_plugin(ctx, "plugin1") == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_ACTIVE);
	check(cp_start_plugin(ctx, "plugin2a") == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_ACTIVE);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1v2")) == CP_OK);
	check(cp_scan_plugins(ctx, CP_SP_UPGRADE) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_ACTIVE);
	
	// Then check upgrade with stop flag
	check(cp_start_plugin(ctx, "plugin1") == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_ACTIVE);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1v3")) == CP_OK);
	check(cp_scan_plugins(ctx, CP_SP_UPGRADE | CP_SP_STOP_ALL_ON_UPGRADE) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_RESOLVED);

	cp_destroy();
	check(errors == 0);
}

void scanstoponinstall(void) {
	cp_context_t *ctx;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1")) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	
	// First check install without stopping other plug-ins
	check(cp_start_plugin(ctx, "plugin1") == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_ACTIVE);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_UNINSTALLED);
	check(cp_register_pcollection(ctx, pcollectiondir("collection2")) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_ACTIVE);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_INSTALLED);
	
	// Then check install and stopping of other plug-ins
	check(cp_uninstall_plugin(ctx, "plugin2a") == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_ACTIVE);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_UNINSTALLED);
	check(cp_scan_plugins(ctx, CP_SP_STOP_ALL_ON_INSTALL) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_RESOLVED);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_INSTALLED);
	
	// Then check upgrade and stopping of other plug-ins
	check(cp_start_plugin(ctx, "plugin2a") == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_ACTIVE);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1v2")) == CP_OK);
	check(cp_scan_plugins(ctx, CP_SP_UPGRADE | CP_SP_STOP_ALL_ON_INSTALL) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_RESOLVED);

	cp_destroy();
	check(errors == 0);
}

void scanrestart(void) {
	cp_context_t *ctx;
	int errors;
	
	ctx = init_context(CP_LOG_ERROR, &errors);
	check(cp_register_pcollection(ctx, pcollectiondir("collection1")) == CP_OK);
	check(cp_register_pcollection(ctx, pcollectiondir("collection2")) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	check(cp_start_plugin(ctx, "plugin2b") == CP_OK);
	check(cp_start_plugin(ctx, "plugin1") == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_ACTIVE);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2b") == CP_PLUGIN_ACTIVE);

	// Check that upgraded plug-in is correctly restarted after upgrade
	check(cp_register_pcollection(ctx, pcollectiondir("collection1v2")) == CP_OK);
	check(cp_scan_plugins(ctx, CP_SP_UPGRADE | CP_SP_RESTART_ACTIVE) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_ACTIVE);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2b") == CP_PLUGIN_ACTIVE);

	// Check that other plug-ins are correctly restarted after upgrade
	check(cp_register_pcollection(ctx, pcollectiondir("collection1v3")) == CP_OK);
	check(cp_scan_plugins(ctx, CP_SP_UPGRADE | CP_SP_STOP_ALL_ON_UPGRADE | CP_SP_RESTART_ACTIVE) == CP_OK);
	check(cp_get_plugin_state(ctx, "plugin1") == CP_PLUGIN_ACTIVE);
	check(cp_get_plugin_state(ctx, "plugin2a") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "plugin2b") == CP_PLUGIN_ACTIVE);
	
	cp_destroy();
	check(errors == 0);
}
