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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_GETTEXT
#include <libintl.h>
#include <locale.h>
#endif
#include <cpluff.h>


/* -----------------------------------------------------------------------
 * Defines
 * ---------------------------------------------------------------------*/

// Gettext defines 
#ifdef HAVE_GETTEXT
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop(String)
#else
#define _(String) (String)
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
#endif

// GNU C attribute defines
#ifndef CP_GCC_NORETURN
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5)
#define CP_GCC_NORETURN __attribute__((noreturn))
#else
#define CP_GCC_NORETURN
#endif
#endif

// Initializer for empty list
#define STR_LIST_INITIALIZER { NULL, NULL }


/* -----------------------------------------------------------------------
 * Data types
 * ---------------------------------------------------------------------*/

/// A type for str_list_t structure
typedef struct str_list_t str_list_t;

/// A type for str_list_entry_t structure
typedef struct str_list_entry_t str_list_entry_t;

/// A string list container
struct str_list_t {
	
	/// The first entry or NULL if empty
	str_list_entry_t *first;
	
	/// The last entry or NULL if empty
	str_list_entry_t *last;
	
};

/// A holder for a string list entry
struct str_list_entry_t {
	
	/// The string
	const char *str;
	
	/// Next entry
	str_list_entry_t *next;
};


/* -----------------------------------------------------------------------
 * Variables
 * ---------------------------------------------------------------------*/

/// The level of verbosity
static int verbosity = 1;


/* -----------------------------------------------------------------------
 * Functions
 * ---------------------------------------------------------------------*/

/**
 * Prints an error message and exits. In quiet mode the error message is
 * not printed.
 * 
 * @param msg the error message
 */
CP_GCC_NORETURN static void error(const char *msg) {
	if (verbosity >= 1) {
		/* TRANSLATORS: A formatting string for loader error messages. */
		fprintf(stderr, _("C-Pluff Loader: ERROR: %s\n"), msg);
	}
	exit(1);
}

/**
 * Formats and prints an error message and exits. In quiet mode the error
 * message is not printed.
 * 
 * @param msg the error message
 */
CP_GCC_NORETURN static void errorf(const char *msg, ...) {
	char buffer[256];
	va_list va;

	va_start(va, msg);
	vsnprintf(buffer, sizeof(buffer), _(msg), va);
	va_end(va);
	strcpy(buffer + sizeof(buffer)/sizeof(char) - 4, "...");
	error(buffer);
}

/**
 * Allocates memory using malloc and checks for failures.
 * 
 * @param size the amount of memory to allocate
 * @return the allocated memory (always non-NULL)
 */
static void *chk_malloc(size_t size) {
	void *ptr = malloc(size);
	if (ptr == NULL) {
		error(_("Memory allocation failed."));
	} else {
		return ptr;
	}
}

/**
 * Appends a new string to a string list. Copies strings by pointers.
 */
static void str_list_append(str_list_t *list, const char *str) {
	str_list_entry_t *entry = chk_malloc(sizeof(str_list_entry_t));
	entry->str = str;
	entry->next = NULL;
	if (list->last != NULL) {
		list->last->next = entry;
	}
	if (list->first == NULL) {
		list->first = entry;
	}
	list->last = entry;
}

/**
 * Removes all entries from a string list. Does not free contained strings.
 */
static void str_list_clear(str_list_t *list) {
	str_list_entry_t *entry = list->first;
	while (entry != NULL) {
		str_list_entry_t *n = entry->next;
		free(entry);
		entry = n;
	}
	list->first = NULL;
	list->last = NULL;
}

/**
 * Prints the help text.
 */
static void print_help(void) {
	printf(_("C-Pluff Loader, version %s\n"), PACKAGE_VERSION);
	putchar('\n');
	fputs(_("usage: cpluff-loader <option>... [--] <arguments passed to plug-ins>\n"
		"options:\n"
		"  -h       print this help text\n"
		"  -c DIR   add plug-in collection in directory DIR\n"
		"  -p DIR   add plug-in in directory DIR\n"
		"  -s PID   start plug-in PID\n"
		"  -v       be more verbose (repeat for increased verbosity)\n"
		"  -q       be quiet\n"
		"  -V       print C-Pluff version number and exit\n"
		), stdout);
}

static void logger(cp_log_severity_t severity, const char *msg, const char *apid, void *dummy) {
	const char *level;
	int minv;
	switch (severity) {
		case CP_LOG_DEBUG:
			/* TRANSLATORS: A tag for debug level log entries. */
			level = _("DEBUG");
			minv = 4;
			break;
		case CP_LOG_INFO:
			/* TRANSLATORS: A tag for info level log entries. */
			level = _("INFO");
			minv = 3;
			break;
		case CP_LOG_WARNING:
			/* TRANSLATORS: A tag for warning level log entries. */
			level = _("WARNING");
			minv = 2;
			break;
		case CP_LOG_ERROR:
			/* TRANSLATORS: A tag for error level log entries. */
			level = _("ERROR");
			minv = 1;
			break;
		default:
			/* TRANSLATORS: A tag for unknown severity level. */ 
			level = _("UNKNOWN");
			minv = 1;
			break;
	}
	if (verbosity >= minv) {
		if (apid != NULL) {
			/* TRANSLATORS: A formatting string for log messages caused by plug-in activity. */ 
			fprintf(stderr, _("C-Pluff: %s: [%s] %s\n"), level, apid, msg);
		} else {
			/* TRANSLATORS: A formatting string for log messages caused by loader activity. */ 
			fprintf(stderr, _("C-Pluff: %s: [loader] %s\n"), level, msg);
		}
	} 
}

/// The main function
int main(int argc, char *argv[]) {
	int i;
	str_list_t lst_plugin_collections = STR_LIST_INITIALIZER;
	str_list_t lst_plugin_dirs = STR_LIST_INITIALIZER;
	str_list_t lst_start = STR_LIST_INITIALIZER;
	cp_context_t *context;
	char **ctx_argv;
	str_list_entry_t *entry;

	// Set locale
#ifdef HAVE_GETTEXT
	setlocale(LC_ALL, "");
#endif
	
	// Initialize the framework
	if (cp_init() != CP_OK) {
		error(_("The C-Pluff initialization failed."));
	}
	
	// Set gettext domain 
#ifdef HAVE_GETTEXT
	textdomain(PACKAGE);
#endif

	// Parse arguments
	while ((i = getopt(argc, argv, "hc:p:s:vqV")) != -1) {
		switch (i) {
			
			// Display help and exit
			case 'h':
				print_help();
				exit(0);

			// Add a plug-in collection
			case 'c':
				str_list_append(&lst_plugin_collections, optarg);
				break;

			// Add a single plug-in
			case 'p':
				str_list_append(&lst_plugin_dirs, optarg);
				break;
				
			// Add a plug-in to be started
			case 's':
				str_list_append(&lst_start, optarg);
				break;

			// Be more verbose
			case 'v':
				if (verbosity < 1) {
					error(_("Quiet and verbose modes are mutually exclusive."));
				}
				verbosity++;
				break;

			// Quiet mode
			case 'q':
				if (verbosity > 1) {
					error(_("Quiet and verbose modes are mutually exclusive."));
				}
				verbosity--;
				break;

			// Display release version and exit
			case 'V':
				fputs(cp_get_version(), stdout);
				putchar('\n');
				exit(0);
				
			// Unrecognized option
			default:
				error(_("Unrecognized option or argument. Try option -h for help."));
		}
	}

	// Display startup information
	if (verbosity >= 1) {
		
		/* TRANSLATORS: This is a version string displayed on startup. */
		fprintf(stderr, _("C-Pluff Loader, version %s\n"), PACKAGE_VERSION);
		
		/* TRANSLATORS: This is a version string displayed on startup.
		   The first %s is version and the second %s is platform type. */
		fprintf(stderr, _("C-Pluff Library, version %s for %s\n"),
			cp_get_version(), cp_get_host_type());
	}
	
	// Check arguments
	if (lst_plugin_dirs.first == NULL && lst_plugin_collections.first == NULL) {
		error(_("No plug-ins to load. Try option -h for help."));
	}
	
	// Create the context
	if ((context = cp_create_context(NULL)) == NULL) {
		error(_("Plug-in context creation failed."));
	}
	
	// Register logger
	if (verbosity >= 1) {
		cp_log_severity_t mv = CP_LOG_DEBUG;
		switch (verbosity) {
			case 1:
				mv = CP_LOG_ERROR;
				break;
			case 2:
				mv = CP_LOG_WARNING;
				break;
			case 3:
				mv = CP_LOG_INFO;
				break;
		}
		cp_register_logger(context, logger, NULL, mv);
	}
	
	// Set context arguments
	ctx_argv = chk_malloc((argc - optind + 2) * sizeof(char *));
	ctx_argv[0] = "";
	for (i = optind; i < argc; i++) {
		ctx_argv[i - optind + 1] = argv[i];
	}
	ctx_argv[argc - optind + 1] = NULL;
	cp_set_context_args(context, ctx_argv);

	// Load individual plug-ins
	for (entry = lst_plugin_dirs.first; entry != NULL; entry = entry->next) {
		cp_plugin_info_t *pi = cp_load_plugin_descriptor(context, entry->str, NULL);
		if (pi == NULL) {
			errorf(_("Failed to load a plug-in from path %s."), entry->str);
		}
		if (cp_install_plugin(context, pi) != CP_OK) {
			errorf(_("Failed to install plug-in %s."), pi->identifier);
		}
		cp_release_info(context, pi);
	}
	str_list_clear(&lst_plugin_dirs);
	
	// Load plug-in collections
	for (entry = lst_plugin_collections.first; entry != NULL; entry = entry->next) {
		if (cp_register_pcollection(context, entry->str) != CP_OK) {
			errorf(_("Failed to register a plug-in collection at path %s."), entry->str); 
		}
	}
	if (lst_plugin_collections.first != NULL
		&& cp_scan_plugins(context, 0) != CP_OK) {
		error(_("Failed to load and install plug-ins from plug-in collections."));
	}
	str_list_clear(&lst_plugin_collections);
	
	// Start plug-ins
	for (entry = lst_start.first; entry != NULL; entry = entry->next) {
		if (cp_start_plugin(context, entry->str) != CP_OK) {
			errorf(_("Failed to start plug-in %s."), entry->str);
		}
	}
	str_list_clear(&lst_start);

	// Run plug-ins
	cp_run_plugins(context);

	// Destroy framework
	cp_destroy();
	
	// Release context argument data
	free(ctx_argv);

	// Return from the main program
	return 0;
}
