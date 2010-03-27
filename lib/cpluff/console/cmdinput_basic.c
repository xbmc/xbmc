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

// Basic command line input functionality 

#include <stdio.h>
#include <string.h>
#include "console.h"

#define CMDLINE_SIZE 256

CP_HIDDEN void cmdline_init(void) {}

CP_HIDDEN char *cmdline_input(const char *prompt) {
	static char cmdline[CMDLINE_SIZE];
	int i, success = 0;
	
	do {
		fputs(prompt, stdout);
		if (fgets(cmdline, CMDLINE_SIZE, stdin) == NULL) {
			return NULL;
		}
		if (strlen(cmdline) == CMDLINE_SIZE - 1
			&& cmdline[CMDLINE_SIZE - 2] != '\n') {
			char c;
			do {
				c = getchar();
			} while (c != '\n');
			fputs(_("ERROR: Command line is too long.\n"), stderr);
		} else {
			success = 1;
		}
	} while (!success);
	i = strlen(cmdline);
	if (i > 0 && cmdline[i - 1] == '\n') {
		cmdline[i - 1] = '\0';
	}
	return cmdline;
}

CP_HIDDEN void cmdline_destroy(void) {}
