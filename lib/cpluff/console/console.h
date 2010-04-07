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

// Global declarations 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_GETTEXT
#include <libintl.h>
#endif
#include <cpluff.h>


/* ------------------------------------------------------------------------
 * Defines
 * ----------------------------------------------------------------------*/

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


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/// Type of argument completion
typedef enum arg_compl_t {
	
	/// Do not use completion
	CPC_COMPL_NONE,
	
	/// Use file name completion
	CPC_COMPL_FILE,
	
	/// Use scan flag completion
	CPC_COMPL_FLAG,
	
	/// Use log level completion
	CPC_COMPL_LOG_LEVEL,
	
	/// Use plug-in identifier completion
	CPC_COMPL_PLUGIN,
	
} arg_compl_t;

/// Type for command implementations 
typedef void (*command_func_t)(int argc, char *argv[]);

/// Type for command information 
typedef struct command_info_t {
	
	/// The name of the command 
	char *name;
	
	/// The description for the command 
	const char *description;
	
	/// The command implementation 
	command_func_t implementation;
	
	/// The type of argument completion to use
	arg_compl_t arg_completion;
	
} command_info_t;

/// Type for flag information 
typedef struct flag_info_t {
	
	/// The name of the flag 
	const char *name;
	
	/// The description of the flag
	const char *description;
	
	/// The value of the flag 
	int value;
	
} flag_info_t;

/// Type for log level information
typedef struct log_level_info_t {
	
	/// The name of the log level
	const char *name;
	
	/// The descriptor of the log level
	const char *description;
	
	/// The value of the log level
	int level;
	
} log_level_info_t;


/* ------------------------------------------------------------------------
 * Global variables
 * ----------------------------------------------------------------------*/

/// The plug-in context
CP_HIDDEN extern cp_context_t *context;

/// The available commands 
CP_HIDDEN extern const command_info_t commands[];

/// The available load flags 
CP_HIDDEN extern const flag_info_t load_flags[];

/// The available logging levels
CP_HIDDEN extern const log_level_info_t log_levels[];


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

/**
 * Initializes command line reading. Must be called once to initialize
 * everything before using cmdline_input.
 */
CP_HIDDEN void cmdline_init(void);

/**
 * Returns a command line entered by the user. Uses the specified prompt.
 * The returned command line is valid and it may be modified until the
 * next call to this function.
 * 
 * @param prompt the prompt to be used
 * @return the command line entered by the user
 */
CP_HIDDEN char *cmdline_input(const char *prompt);

/**
 * Releases command line reading resources. Must be called after command
 * line input is not needed and before destroying the context.
 */
CP_HIDDEN void cmdline_destroy(void);

#ifndef CONSOLE_H_
#define CONSOLE_H_

#endif //CONSOLE_H_
