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

// Core console logic 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_GETTEXT
#include <locale.h>
#endif
#include <assert.h>
#include <cpluff.h>
#include "console.h"


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

// Function declarations for command implementations 
static void cmd_help(int argc, char *argv[]);
static void cmd_set_log_level(int argc, char *argv[]);
static void cmd_register_pcollection(int argc, char *argv[]);
static void cmd_unregister_pcollection(int argc, char *argv[]);
static void cmd_unregister_pcollections(int argc, char *argv[]);
static void cmd_load_plugin(int argc, char *argv[]);
static void cmd_scan_plugins(int argc, char *argv[]);
static void cmd_list_plugins(int argc, char *argv[]);
static void cmd_show_plugin_info(int argc, char *argv[]);
static void cmd_list_ext_points(int argc, char *argv[]);
static void cmd_list_extensions(int argc, char *argv[]);
static void cmd_set_context_args(int argc, char *argv[]);
static void cmd_start_plugin(int argc, char *argv[]);
static void cmd_run_plugins_step(int argc, char *argv[]);
static void cmd_run_plugins(int argc, char *argv[]);
static void cmd_stop_plugin(int argc, char *argv[]);
static void cmd_stop_plugins(int argc, char *argv[]);
static void cmd_uninstall_plugin(int argc, char *argv[]);
static void cmd_uninstall_plugins(int argc, char *argv[]);
static void cmd_exit(int argc, char *argv[]);

/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/// The plug-in context
CP_HIDDEN cp_context_t *context;

/// The available commands 
CP_HIDDEN const command_info_t commands[] = {
	{ "help", N_("displays available commands"), cmd_help, CPC_COMPL_NONE },
	{ "set-log-level", N_("sets the displayed log level"), cmd_set_log_level, CPC_COMPL_LOG_LEVEL },
	{ "register-collection", N_("registers a plug-in collection"), cmd_register_pcollection, CPC_COMPL_FILE },
	{ "unregister-collection", N_("unregisters a plug-in collection"), cmd_unregister_pcollection, CPC_COMPL_FILE },
	{ "unregister-collections", N_("unregisters all plug-in collections"), cmd_unregister_pcollections, CPC_COMPL_NONE },
	{ "load-plugin", N_("loads and installs a plug-in from the specified path"), cmd_load_plugin, CPC_COMPL_FILE },
	{ "scan-plugins", N_("scans plug-ins in the registered plug-in collections"), cmd_scan_plugins, CPC_COMPL_FLAG },
	{ "set-context-args", N_("sets context startup arguments"), cmd_set_context_args, CPC_COMPL_FILE },
	{ "start-plugin", N_("starts a plug-in"), cmd_start_plugin, CPC_COMPL_PLUGIN },
	{ "run-plugins-step", N_("runs one plug-in run function"), cmd_run_plugins_step, CPC_COMPL_NONE },
	{ "run-plugins", N_("runs plug-in run functions until all work is done"), cmd_run_plugins, CPC_COMPL_NONE },
	{ "stop-plugin", N_("stops a plug-in"), cmd_stop_plugin, CPC_COMPL_PLUGIN },
	{ "stop-plugins", N_("stops all plug-ins"), cmd_stop_plugins, CPC_COMPL_NONE },
	{ "uninstall-plugin", N_("uninstalls a plug-in"), cmd_uninstall_plugin, CPC_COMPL_PLUGIN },
	{ "uninstall-plugins", N_("uninstalls all plug-ins"), cmd_uninstall_plugins, CPC_COMPL_NONE },
	{ "list-plugins", N_("lists the installed plug-ins"), cmd_list_plugins, CPC_COMPL_NONE },
	{ "list-ext-points", N_("lists the installed extension points"), cmd_list_ext_points, CPC_COMPL_NONE },
	{ "list-extensions", N_("lists the installed extensions"), cmd_list_extensions, CPC_COMPL_NONE },
	{ "show-plugin-info", N_("shows static plug-in information"), cmd_show_plugin_info, CPC_COMPL_PLUGIN },
	{ "quit", N_("quits the program"), cmd_exit, CPC_COMPL_NONE },
	{ "exit", N_("quits the program"), cmd_exit, CPC_COMPL_NONE },
	{ NULL, NULL, NULL, CPC_COMPL_NONE }
};

/// The available load flags 
CP_HIDDEN const flag_info_t load_flags[] = {
	{ "upgrade", N_("enables upgrades of installed plug-ins"), CP_SP_UPGRADE },
	{ "stop-all-on-upgrade", N_("stops all plug-ins on first upgrade"), CP_SP_STOP_ALL_ON_UPGRADE },
	{ "stop-all-on-install", N_("stops all plug-ins on first install or upgrade"), CP_SP_STOP_ALL_ON_INSTALL },
	{ "restart-active", N_("restarts the currently active plug-ins after the scan"), CP_SP_RESTART_ACTIVE },
	{ NULL, NULL, -1 }
};

/// The available log levels
CP_HIDDEN const log_level_info_t log_levels[] = {
	{ "debug", N_("detailed debug messages"), CP_LOG_DEBUG },
	{ "info", N_("informational messages"), CP_LOG_INFO },
	{ "warning", N_("warnings about possible problems"), CP_LOG_WARNING },
	{ "error", N_("error messages"), CP_LOG_ERROR },
	{ "none", N_("disable logging"), CP_LOG_ERROR + 1 },
	{ NULL, NULL, -1 }
};


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

/**
 * Parses a command line (in place) into white-space separated elements.
 * Returns the number of elements and the pointer to argument table including
 * command and arguments. The argument table is valid until the next call
 * to this function.
 * 
 * @param cmdline the command line to be parsed
 * @param argv pointer to the argument table is stored here
 * @return the number of command line elements, or -1 on failure
 */
static int cmdline_parse(char *cmdline, char **argv[]) {
	static char *sargv[16];
	int i, argc;
			
	for (i = 0; isspace(cmdline[i]); i++);
	for (argc = 0; cmdline[i] != '\0' && argc < 16; argc++) {
		sargv[argc] = cmdline + i;
		while (cmdline[i] != '\0' && !isspace(cmdline[i])) {
			i++;
		}
		if (cmdline[i] != '\0') {
			cmdline[i++] = '\0';
			while (isspace(cmdline[i])) {
				i++;
			}
		}
	}
	if (cmdline[i] != '\0') {
		fputs(_("Command has too many arguments.\n"), stdout);
		return -1;
	} else {
		*argv = sargv;
		return argc;
	}
}

static void cmd_exit(int argc, char *argv[]) {
	
	// Uninitialize input
	cmdline_destroy();
	
	// Destroy C-Pluff framework 
	cp_destroy();
	
	// Exit program 
	exit(0);
}

static void cmd_help(int argc, char *argv[]) {
	int i;
	
	fputs(_("The following commands are available:\n"), stdout);
	for (i = 0; commands[i].name != NULL; i++) {
		printf("  %s - %s\n", commands[i].name, _(commands[i].description));
	}
}

static void logger(cp_log_severity_t severity, const char *msg, const char *apid, void *dummy) {
	const char *level;
	switch (severity) {
		case CP_LOG_DEBUG:
			/* TRANSLATORS: A tag for debug level log entries. */
			level = _("DEBUG");
			break;
		case CP_LOG_INFO:
			/* TRANSLATORS: A tag for info level log entries. */
			level = _("INFO");
			break;
		case CP_LOG_WARNING:
			/* TRANSLATORS: A tag for warning level log entries. */
			level = _("WARNING");
			break;
		case CP_LOG_ERROR:
			/* TRANSLATORS: A tag for error level log entries. */
			level = _("ERROR");
			break;
		default:
			/* TRANSLATORS: A tag for unknown severity level. */ 
			level = _("UNKNOWN");
			break;
	}
	fprintf(stderr, "C-Pluff: %s: [%s] %s\n",
		level,
		apid != NULL ? apid :
		/* TRANSLATORS: Used when displaying log messages originating
		   from console activities. */
		_("console"),
		msg);
}

static void cmd_set_log_level(int argc, char *argv[]) {
	if (argc != 2) {
		/* TRANSLATORS: Usage instructions for setting log level */ 
		printf(_("Usage: %s <level>\n"), argv[0]);
	} else {
		int i;
		
		for (i = 0; log_levels[i].name != NULL; i++) {
			if (!strcmp(argv[1], log_levels[i].name)) {
				break;
			}
		}
		if (log_levels[i].name == NULL) {
			printf(_("Unknown log level %s.\n"), argv[1]);
			fputs(_("Available log levels are:\n"), stdout);
			for (i = 0; log_levels[i].name != NULL; i++) {
				printf("  %s - %s\n", log_levels[i].name, _(log_levels[i].description));
			}
		} else {
			if (log_levels[i].level <= CP_LOG_ERROR) {
				cp_register_logger(context, logger, NULL, log_levels[i].level);
			} else {
				cp_unregister_logger(context, logger);
			}
			/* TRANSLATORS: The first %s is the log level name and the second the localized log level description. */
			printf(_("Using display log level %s (%s).\n"), log_levels[i].name, _(log_levels[i].description));			
		}
	}
}

static const char *status_to_desc(cp_status_t status) {
	switch (status) {
		case CP_OK:
			/* TRANSLATORS: Return status for a successfull API call */
			return _("success");
		case CP_ERR_RESOURCE:
			return _("insufficient system resources");
		case CP_ERR_UNKNOWN:
			return _("an unknown object was specified");
		case CP_ERR_IO:
			return _("an input or output error");
		case CP_ERR_MALFORMED:
			return _("a malformed plug-in descriptor");
		case CP_ERR_CONFLICT:
			return _("a plug-in or symbol conflicts with an existing one");
		case CP_ERR_DEPENDENCY:
			return _("unsatisfiable dependencies");
		case CP_ERR_RUNTIME:
			return _("a plug-in runtime library encountered an error");
		default:
			return _("unknown error code");
	}
}

static void api_failed(const char *func, cp_status_t status) {
	printf(_("API function %s failed with error code %d (%s).\n"),
		func,
		status,
		status_to_desc(status));
}

static void cmd_register_pcollection(int argc, char *argv[]) {
	cp_status_t status;
	
	if (argc != 2) {
		/* TRANSLATORS: Usage instructions for registering a plug-in collection */
		printf(_("Usage: %s <path>\n"), argv[0]);
	} else if ((status = cp_register_pcollection(context, argv[1])) != CP_OK) {
		api_failed("cp_register_pcollection", status);
	} else {
		printf(_("Registered a plug-in collection in path %s.\n"), argv[1]);
	}
}

static void cmd_unregister_pcollection(int argc, char *argv[]) {
	if (argc != 2) {
		/* TRANSLATORS: Usage instructions for unregistering a plug-in collection */
		printf(_("Usage: %s <path>\n"), argv[0]);
	} else {
		cp_unregister_pcollection(context, argv[1]);
		printf(_("Unregistered a plug-in collection in path %s.\n"), argv[1]);
	}
}

static void cmd_unregister_pcollections(int argc, char *argv[]) {
	if (argc != 1) {
		/* TRANSLATORS: Usage instructions for unregistering all plug-in collections */
		printf(_("Usage: %s\n"), argv[0]);
	} else {
		cp_unregister_pcollections(context);
		fputs(_("Unregistered all plug-in collections.\n"), stdout);
	}
}

static void cmd_load_plugin(int argc, char *argv[]) {
	cp_plugin_info_t *plugin;
	cp_status_t status;
		
	if (argc != 2) {
		/* TRANSLATORS: Usage instructios for loading a plug-in */
		printf(_("Usage: %s <path>\n"), argv[0]);
	} else if ((plugin = cp_load_plugin_descriptor(context, argv[1], &status)) == NULL) {
		api_failed("cp_load_plugin_descriptor", status);
	} else if ((status = cp_install_plugin(context, plugin)) != CP_OK) {
		api_failed("cp_install_plugin", status); 
		cp_release_info(context, plugin);
	} else {
		printf(_("Installed plug-in %s.\n"), plugin->identifier);
		cp_release_info(context, plugin);
	}
}

static void cmd_scan_plugins(int argc, char *argv[]) {
	int flags = 0;
	cp_status_t status;
	int i;
	
	// Set flags 
	for (i = 1; i < argc; i++) {
		int j;
		
		for (j = 0; load_flags[j].name != NULL; j++) {
			if (!strcmp(argv[i], load_flags[j].name)) {
				flags |= load_flags[j].value;
				break;
			}
		}
		if (load_flags[j].name == NULL) {
			printf(_("Unknown flag %s.\n"), argv[i]);
			/* TRANSLATORS: Usage instructions for scanning plug-ins */
			printf(_("Usage: %s [<flag>...]\n"), argv[0]);
			fputs(_("Available flags are:\n"), stdout);
			for (j = 0; load_flags[j].name != NULL; j++) {
				printf("  %s - %s\n", load_flags[j].name, _(load_flags[j].description));
			}
			return;
		}
	}
	
	if ((status = cp_scan_plugins(context, flags)) != CP_OK) {
		api_failed("cp_scan_plugins", status);
		return;
	}
	fputs(_("Plug-ins loaded.\n"), stdout);
}

static char *state_to_string(cp_plugin_state_t state) {
	switch (state) {
		case CP_PLUGIN_UNINSTALLED:
			return _("uninstalled");
		case CP_PLUGIN_INSTALLED:
			return _("installed");
		case CP_PLUGIN_RESOLVED:
			return _("resolved");
		case CP_PLUGIN_STARTING:
			return _("starting");
		case CP_PLUGIN_STOPPING:
			return _("stopping");
		case CP_PLUGIN_ACTIVE:
			return _("active");
		default:
			return _("unknown");
	}
}

static void cmd_list_plugins(int argc, char *argv[]) {
	cp_plugin_info_t **plugins;
	cp_status_t status;
	int i;

	if (argc != 1) {
		/* TRANSLATORS: Usage instructions for listing plug-ins */
		printf(_("Usage: %s\n"), argv[0]);
	} else if ((plugins = cp_get_plugins_info(context, &status, NULL)) == NULL) {
		api_failed("cp_get_plugins_info", status);
	} else {
		const char format[] = "  %-24s %-8s %-12s %s\n";
		fputs(_("Installed plug-ins:\n"), stdout);
		printf(format,
			_("IDENTIFIER"),
			_("VERSION"),
			_("STATE"),
			_("NAME"));
		for (i = 0; plugins[i] != NULL; i++) {
			printf(format,
				plugins[i]->identifier,
				plugins[i]->version != NULL ? plugins[i]->version : "",
				state_to_string(cp_get_plugin_state(context, plugins[i]->identifier)),
				plugins[i]->name != NULL ? plugins[i]->name : "");
		}
		cp_release_info(context, plugins);
	}
}

struct str_list_entry_t {
	char *str;
	struct str_list_entry_t *next;
};

static struct str_list_entry_t *str_list = NULL;
	
static char *str_or_null(const char *str) {
	if (str != NULL) {
		char *s = malloc((strlen(str) + 3) * sizeof(char));
		struct str_list_entry_t *entry = malloc(sizeof(struct str_list_entry_t));
		if (s == NULL || entry == NULL) {
			fputs(_("Memory allocation failed.\n"), stderr);
			abort();
		}
		sprintf(s, "\"%s\"", str);
		entry->next = str_list;
		entry->str = s;
		str_list = entry;
		return s;
	} else {
		return "NULL";
	}
}

static void str_or_null_free(void) {
	while (str_list != NULL) {
		struct str_list_entry_t *next = str_list->next;
		free(str_list->str);
		free(str_list);
		str_list = next;
	}
}

static void show_plugin_info_import(cp_plugin_import_t *import) {
	printf("    plugin_id = \"%s\",\n"
		"    version = %s,\n"
		"    optional = %d,\n",
		import->plugin_id,
		str_or_null(import->version),
		import->optional);
}

static void show_plugin_info_ext_point(cp_ext_point_t *ep) {
	assert(ep->plugin != NULL);
	printf("    local_id = \"%s\",\n"
		"    identifier = \"%s\",\n"
		"    name = %s,\n"
		"    schema_path = %s,\n",
		ep->local_id,
		ep->identifier,
	 	str_or_null(ep->name),
	  	str_or_null(ep->schema_path));
}

static void strcat_quote_xml(char *dst, const char *src, int is_attr) {
	char c;
	
	while (*dst != '\0')
		dst++;
	do {
		switch ((c = *(src++))) {
			case '&':
				strcpy(dst, "&amp;");
				dst += 5;
				break;
			case '<':
				strcpy(dst, "&lt;");
				dst += 4;
				break;
			case '>':
				strcpy(dst, "&gt;");
				dst += 4;
				break;
			case '"':
				if (is_attr) {
					strcpy(dst, "&quot;");
					dst += 6;
					break;
				}
			default:
				*(dst++) = c;
				break;
		}
	} while (c != '\0');
}

static int strlen_quoted_xml(const char *str,int is_attr) {
	int len = 0;
	int i;
	
	for (i = 0; str[i] != '\0'; i++) {
		switch (str[i]) {
			case '&':
				len += 5;
				break;
			case '<':
			case '>':
				len += 4;
				break;
			case '"':
				if (is_attr) {
					len += 6;
					break;
				}
			default:
				len++;
		}
	}
	return len;
}

static void show_plugin_info_cfg(cp_cfg_element_t *ce, int indent) {
	char *buffer = NULL;
	int rs;
	int i;

	// Calculate the maximum required buffer size
	rs = 2 * strlen(ce->name) + 6 + indent;
	if (ce->value != NULL) {
		rs += strlen_quoted_xml(ce->value, 0);
	}
	for (i = 0; i < ce->num_atts; i++) {
		rs += strlen(ce->atts[2*i]);
		rs += strlen_quoted_xml(ce->atts[2*i + 1], 1);
		rs += 4;
	}
	
	// Allocate buffer
	if ((buffer = malloc(rs * sizeof(char))) == NULL) {
		fputs(_("Memory allocation failed.\n"), stderr);
		abort();
	}
	
	// Create the string
	for (i = 0; i < indent; i++) {
		buffer[i] = ' ';
	}
	buffer[i++] = '<';
	buffer[i] = '\0';
	strcat(buffer, ce->name);
	for (i = 0; i < ce->num_atts; i++) {
		strcat(buffer, " ");
		strcat(buffer, ce->atts[2*i]);
		strcat(buffer, "=\"");
		strcat_quote_xml(buffer, ce->atts[2*i + 1], 1);
		strcat(buffer, "\"");
	}
	if (ce->value != NULL || ce->num_children) {
		strcat(buffer, ">");
		if (ce->value != NULL) {
			strcat_quote_xml(buffer, ce->value, 0);
		}
		if (ce->num_children) {
			fputs(buffer, stdout);
			putchar('\n');
			for (i = 0; i < ce->num_children; i++) {
				show_plugin_info_cfg(ce->children + i, indent + 2);
			}
			for (i = 0; i < indent; i++) {
				buffer[i] = ' ';
			}
			buffer[i++] = '<';
			buffer[i++] = '/';
			buffer[i] = '\0';
			strcat(buffer, ce->name);
			strcat(buffer, ">");
		} else {
			strcat(buffer, "</");
			strcat(buffer, ce->name);
			strcat(buffer, ">");
		}
	} else {
		strcat(buffer, "/>");
	}
	fputs(buffer, stdout);
	putchar('\n');
	free(buffer);
}

static void show_plugin_info_extension(cp_extension_t *e) {
	assert(e->plugin != NULL);
	printf("    ext_point_id = \"%s\",\n"
		"    local_id = %s,\n"
		"    identifier = %s,\n"
		"    name = %s,\n"
		"    configuration = {\n",
		e->ext_point_id,
		str_or_null(e->local_id),
		str_or_null(e->identifier),
		str_or_null(e->name));
	show_plugin_info_cfg(e->configuration, 6);
	fputs("    },\n", stdout);
}

static void cmd_show_plugin_info(int argc, char *argv[]) {
	cp_plugin_info_t *plugin;
	cp_status_t status;
	int i;
	
	if (argc != 2) {
		/* TRANSLATORS: Usage instructions for showing plug-in information */
		printf(_("Usage: %s <plugin>\n"), argv[0]);
	} else if ((plugin = cp_get_plugin_info(context, argv[1], &status)) == NULL) {
		api_failed("cp_get_plugin_info", status);
	} else {
		printf("{\n"
			"  identifier = \"%s\",\n"
			"  name = %s,\n"
			"  version = %s,\n"
			"  provider_name = %s,\n"
			"  abi_bw_compatibility = %s,\n"
			"  api_bw_compatibility = %s,\n"
			"  plugin_path = %s,\n"
			"  req_cpluff_version = %s,\n",
			plugin->identifier,
			str_or_null(plugin->name),
			str_or_null(plugin->version),
			str_or_null(plugin->provider_name),
			str_or_null(plugin->abi_bw_compatibility),
			str_or_null(plugin->api_bw_compatibility),
			str_or_null(plugin->plugin_path),
			str_or_null(plugin->req_cpluff_version));
		if (plugin->num_imports) {
			fputs("  imports = {{\n", stdout);
			for (i = 0; i < plugin->num_imports; i++) {
				if (i)
					fputs("  }, {\n", stdout);
				show_plugin_info_import(plugin->imports + i);
			}
			fputs("  }},\n", stdout);
		} else {
			fputs("  imports = {},\n", stdout);
		}
		printf("  runtime_lib_name = %s,\n"
			"  runtime_funcs_symbol = %s,\n",
			str_or_null(plugin->runtime_lib_name),
			str_or_null(plugin->runtime_funcs_symbol));
		if (plugin->num_ext_points) {
			fputs("  ext_points = {{\n", stdout);
			for (i = 0; i < plugin->num_ext_points; i++) {
				if (i)
					fputs("  }, {\n", stdout);
				show_plugin_info_ext_point(plugin->ext_points + i);
			}
			fputs("  }},\n", stdout);
		} else {
			fputs("  ext_points = {},\n", stdout);
		}
		if (plugin->num_extensions) {
			fputs("  extensions = {{\n", stdout);
			for (i = 0; i < plugin->num_extensions; i++) {
				if (i)
					fputs("  }, {\n", stdout);
				show_plugin_info_extension(plugin->extensions + i);
			}
			fputs("  }}\n", stdout);
		} else {
			fputs("  extensions = {},\n", stdout);
		}
		fputs("}\n", stdout);
		cp_release_info(context, plugin);
		str_or_null_free();
	}
}

static void cmd_list_ext_points(int argc, char *argv[]) {
	cp_ext_point_t **ext_points;
	cp_status_t status;
	int i;

	if (argc != 1) {
		/* TRANSLATORS: Usage instructions for listing extension points */
		printf(_("Usage: %s\n"), argv[0]);
	} else if ((ext_points = cp_get_ext_points_info(context, &status, NULL)) == NULL) {
		api_failed("cp_get_ext_points_info", status);
	} else {
		const char format[] = "  %-32s %s\n";
		fputs(_("Installed extension points:\n"), stdout);
		printf(format,
			_("IDENTIFIER"),
			_("NAME"));
		for (i = 0; ext_points[i] != NULL; i++) {
			printf(format,
				ext_points[i]->identifier,
				ext_points[i]->name != NULL ? ext_points[i]->name : ""); 
		}
		cp_release_info(context, ext_points);
	}	
}

static void cmd_list_extensions(int argc, char *argv[]) {
	cp_extension_t **extensions;
	cp_status_t status;
	int i;

	if (argc != 1) {
		/* TRANSLATORS: Usage instructions for listing extensions */
		printf(_("Usage: %s\n"), argv[0]);
	} else if ((extensions = cp_get_extensions_info(context, NULL, &status, NULL)) == NULL) {
		api_failed("cp_get_extensions_info", status);
	} else {
		const char format[] = "  %-32s %s\n";
		fputs(_("Installed extensions:\n"), stdout);
		printf(format,
			_("IDENTIFIER"),
			_("NAME"));
		for (i = 0; extensions[i] != NULL; i++) {
			if (extensions[i]->identifier == NULL) {
				char buffer[128];
				snprintf(buffer, sizeof(buffer), "%s%s", extensions[i]->plugin->identifier, _(".<anonymous>"));
				strcpy(buffer + sizeof(buffer)/sizeof(char) - 4, "...");
				printf(format,
					buffer,
					extensions[i]->name != NULL ? extensions[i]->name : "");
			} else {
				printf(format,
					extensions[i]->identifier,
					extensions[i]->name != NULL ? extensions[i]->name : "");
			}
		}
		cp_release_info(context, extensions);
	}	
}

static char **argv_dup(int argc, char *argv[]) {
	char **dup;
	int i;
	
	if ((dup = malloc((argc + 1) * sizeof(char *))) == NULL) {
		return NULL;
	}
	dup[0] = "";
	for (i = 1; i < argc; i++) {
		if ((dup[i] = strdup(argv[i])) == NULL) {
			for (i--; i > 0; i--) {
				free(dup[i]);
			}
			free(dup);
			return NULL;
		}
	}
	dup[argc] = NULL;
	return dup;
}

static void cmd_set_context_args(int argc, char *argv[]) {
	char **ctx_argv;

	if (argc != 1) {
		/* TRANSLATORS: Usage instructions for setting context arguments */
		printf(_("Usage: %s [<arg>...]\n"), argv[0]);
	} else if ((ctx_argv = argv_dup(argc, argv)) == NULL) {
		fputs(_("Memory allocation failed.\n"), stderr);
	} else {
		cp_set_context_args(context, ctx_argv);
		fputs(_("Plug-in context startup arguments have been set.\n"), stdout);
	}
}

static void cmd_start_plugin(int argc, char *argv[]) {
	cp_status_t status;
	
	if (argc != 2) {
		/* TRANSLATORS: Usage instructions for starting a plug-in */
		printf(_("Usage: %s <plugin>\n"), argv[0]);
	} else if ((status = cp_start_plugin(context, argv[1])) != CP_OK) {
		api_failed("cp_start_plugin", status);
	} else {
		printf(_("Started plug-in %s.\n"), argv[1]);
	}
}

static void cmd_run_plugins_step(int argc, char *argv[]) {
	
	if (argc != 1) {
		/* TRANSLATORS: Usage instructions for running one plug-in run function */
		printf(_("Usage: %s\n"), argv[0]);
	} else {
		int pending = cp_run_plugins_step(context);
		if (pending) {
			fputs(_("Ran one plug-in run function. There are pending run functions.\n"), stdout);
		} else {
			fputs(_("Ran one plug-in run function. No more pending run functions.\n"), stdout);
		}
	}
}

static void cmd_run_plugins(int argc, char *argv[]) {
	if (argc != 1) {
		/* TRANSLATORS: Usage instructions for running plug-in run functions until all work is done */
		printf(_("Usage: %s\n"), argv[0]);
	} else {
		cp_run_plugins(context);
		fputs(_("Ran plug-in run functions. No more pending run functions.\n"), stdout);
	}
}

static void cmd_stop_plugin(int argc, char *argv[]) {
	cp_status_t status;
	
	if (argc != 2) {
		/* TRANSLATORS: Usage instructions for stopping a plug-in */
		printf(_("Usage: %s <plugin>\n"), argv[0]);
	} else if ((status = cp_stop_plugin(context, argv[1])) != CP_OK) {
		api_failed("cp_stop_plugin", status);
	} else {
		printf(_("Stopped plug-in %s.\n"), argv[1]);
	}
}

static void cmd_stop_plugins(int argc, char *argv[]) {
	if (argc != 1) {
		/* TRANSLATORS: Usage instructions for stopping all plug-ins */
		printf(_("Usage: %s\n"), argv[0]);
	} else {
		cp_stop_plugins(context);
		fputs(_("Stopped all plug-ins.\n"), stdout);
	}
}

static void cmd_uninstall_plugin(int argc, char *argv[]) {
	cp_status_t status;
	
	if (argc != 2) {
		/* TRANSLATORS: Usage instructions for uninstalling a plug-in */
		printf(_("Usage: %s <plugin>\n"), argv[0]);
	} else if ((status = cp_uninstall_plugin(context, argv[1])) != CP_OK) {
		api_failed("cp_uninstall_plugin", status);
	} else {
		printf(_("Uninstalled plug-in %s.\n"), argv[1]);
	}
}

static void cmd_uninstall_plugins(int argc, char *argv[]) {
	if (argc != 1) {
		/* TRANSLATORS: Usage instructions for uninstalling all plug-ins */
		printf(_("Usage: %s\n"), argv[0]);
	} else {
		cp_uninstall_plugins(context);
		fputs(_("Uninstalled all plug-ins.\n"), stdout);
	}
}

int main(int argc, char *argv[]) {
	char *prompt;
	int i;
	cp_status_t status;

	// Set locale
#ifdef HAVE_GETTEXT
	setlocale(LC_ALL, "");
#endif

	// Initialize C-Pluff library 
	if ((status = cp_init()) != CP_OK) {
		api_failed("cp_init", status);
		exit(1);
	}
	
	// Set gettext domain 
#ifdef HAVE_GETTEXT
	textdomain(PACKAGE);
#endif

	// Display startup information 
	printf(
		/* TRANSLATORS: This is a version string displayed on startup. */
		_("C-Pluff Console, version %s\n"), PACKAGE_VERSION);
	printf(
		/* TRANSLATORS: This is a version string displayed on startup.
		   The first %s is version and the second %s is platform type. */
		_("C-Pluff Library, version %s for %s\n"),
		cp_get_version(), cp_get_host_type());

	// Create a plug-in context
	context = cp_create_context(&status);
	if (context == NULL) {
		api_failed("cp_create_context", status);
		exit(1);
	}

	// Initialize logging
	cp_register_logger(context, logger, NULL, log_levels[1].level);
	printf(_("Using display log level %s (%s).\n"), log_levels[1].name, _(log_levels[1].description));

	// Command line loop 
	fputs(_("Type \"help\" for help on available commands.\n"), stdout);
	cmdline_init();
	
	/* TRANSLATORS: This is the input prompt for cpluff-console. */
	prompt = _("C-Pluff Console > ");
	
	while (1) {
		char *cmdline;
		int argc;
		char **argv;
		
		// Get command line 
		cmdline = cmdline_input(prompt);
		if (cmdline == NULL) {
			putchar('\n');
			cmdline = "exit";
		}
		
		// Parse command line 
		argc = cmdline_parse(cmdline, &argv);
		if (argc <= 0) {
			continue;
		}
		
		// Choose command 
		for (i = 0; commands[i].name != NULL; i++) {
			if (!strcmp(argv[0], commands[i].name)) {
				commands[i].implementation(argc, argv);
				break;
			}
		}
		if (commands[i].name == NULL) {
			printf(_("Unknown command %s.\n"), argv[0]);
		}
	}
}
