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

static int active(cp_context_t *ctx, const char * const * const plugins) {
	cp_plugin_info_t **pis;
	cp_status_t status;
	int i;
	int errors = 0;
	
	check((pis = cp_get_plugins_info(ctx, &status, NULL)) != NULL && status == CP_OK);
	for (i = 0; !errors && pis[i] != NULL; i++) {
		int j;
		int should_be_active = 0;
		cp_plugin_state_t state;
		
		for (j = 0; !should_be_active && plugins[j] != NULL; j++) {
			if (!strcmp(pis[i]->identifier, plugins[j])) {
				should_be_active = 1;
			}
		}
		state = cp_get_plugin_state(ctx, pis[i]->identifier);
		if ((should_be_active && state != CP_PLUGIN_ACTIVE)
			|| (!should_be_active && state == CP_PLUGIN_ACTIVE)) {
			fprintf(stderr, "plug-in %s has unexpected state %d\n", pis[i]->identifier, state);
			errors++;
		}
	}
	cp_release_info(ctx, pis);
	return errors == 0;
}

void pluginmissingdep(void) {
	cp_context_t *ctx;
	const char * const act_none[] = { NULL };
	
	ctx = init_context(CP_LOG_ERROR + 1, NULL);
	check((cp_register_pcollection(ctx, pcollectiondir("dependencies"))) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	
	// Try starting a plugin depending on plug-in missing dependency
	check(cp_start_plugin(ctx, "chainmissingdep") == CP_ERR_DEPENDENCY);
	check(active(ctx, act_none));

	// Try starting a plug-in with missing dependency
	check(cp_start_plugin(ctx, "missingdep") == CP_ERR_DEPENDENCY);
	check(active(ctx, act_none));

	cp_destroy();
}

void plugindepchain(void) {
	cp_context_t *ctx;
	const char * const act_none[] = { NULL };
	const char * const act_chain123[] = { "chain1", "chain2", "chain3", NULL };
	const char * const act_chain23[] = { "chain2", "chain3", NULL };
	const char * const act_chain3[] = { "chain3", NULL };
	
	ctx = init_context(CP_LOG_ERROR, NULL);
	check((cp_register_pcollection(ctx, pcollectiondir("dependencies"))) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);
	
	// Try starting and stopping plug-ins in dependency chain
	check(cp_start_plugin(ctx, "chain1") == CP_OK);
	check(active(ctx, act_chain123));
	check(cp_stop_plugin(ctx, "chain3") == CP_OK);
	check(active(ctx, act_none));
	check(cp_start_plugin(ctx, "chain2") == CP_OK);
	check(active(ctx, act_chain23));
	check(cp_stop_plugin(ctx, "chain2") == CP_OK);
	check(active(ctx, act_chain3));
	check(cp_stop_plugin(ctx, "chain3") == CP_OK);
	check(active(ctx, act_none));
	check(cp_start_plugin(ctx, "chain3") == CP_OK);
	check(active(ctx, act_chain3));

	// Check that chain is unresolved when a plug-in is uninstalled
	check(cp_uninstall_plugin(ctx, "chain3") == CP_OK);
	check(cp_get_plugin_state(ctx, "chain1") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "chain2") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "chain3") == CP_PLUGIN_UNINSTALLED);

	cp_destroy();
}

void plugindeploop(void) {
	cp_context_t *ctx;
	const char * const act_none[] = { NULL };
	const char * const act_sloop12[] = { "sloop1", "sloop2", NULL };
	const char * const act_loop1234[] = { "loop1", "loop2", "loop3", "loop4", NULL };
	const char * const act_loop4[] = { "loop4", NULL };
	const char * const act_loop12345[] = { "loop1", "loop2", "loop3", "loop4", "loop5", NULL };
	
	ctx = init_context(CP_LOG_ERROR, NULL);
	check((cp_register_pcollection(ctx, pcollectiondir("dependencies"))) == CP_OK);
	check(cp_scan_plugins(ctx, 0) == CP_OK);

	// Start/stop checks for a short loop sloop1 <--> sloop2
	check(active(ctx, act_none));
	check(cp_start_plugin(ctx, "sloop1") == CP_OK);
	check(active(ctx, act_sloop12));
	check(cp_stop_plugin(ctx, "sloop1") == CP_OK);
	check(active(ctx, act_none));

	// Start/stop checks for an extended loop
	//   loop1 --> loop2
	//   loop2 --> loop3
	//   loop3 --> loop1
	//   loop2 --> loop4
	//   loop5 --> loop3
	check(cp_start_plugin(ctx, "loop5") == CP_OK);
	check(active(ctx, act_loop12345));
	check(cp_stop_plugin(ctx, "loop4") == CP_OK);
	check(active(ctx, act_none));
	check(cp_start_plugin(ctx, "loop4") == CP_OK);
	check(active(ctx, act_loop4));
	check(cp_start_plugin(ctx, "loop1") == CP_OK);
	check(active(ctx, act_loop1234));
	check(cp_stop_plugin(ctx, "loop3") == CP_OK);
	check(active(ctx, act_loop4));

	// Unresolve check for the short loop
	check(cp_uninstall_plugin(ctx, "sloop1") == CP_OK);
	check(cp_get_plugin_state(ctx, "sloop1") == CP_PLUGIN_UNINSTALLED);
	check(cp_get_plugin_state(ctx, "sloop2") == CP_PLUGIN_INSTALLED);

	// Unresolve check for the extended loop
	check(cp_uninstall_plugin(ctx, "loop4") == CP_OK);
	check(cp_get_plugin_state(ctx, "loop1") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "loop2") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "loop3") == CP_PLUGIN_INSTALLED);
	check(cp_get_plugin_state(ctx, "loop4") == CP_PLUGIN_UNINSTALLED);
	check(cp_get_plugin_state(ctx, "loop5") == CP_PLUGIN_INSTALLED);

	cp_destroy();	
}
