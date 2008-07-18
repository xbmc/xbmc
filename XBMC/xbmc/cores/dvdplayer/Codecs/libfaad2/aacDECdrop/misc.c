/*
 * function: Miscellaneous functions for aacDECdrop
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright (C) 2002 John Edwards
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "misc.h"

static char *_filename;
void (*error_handler)(const char *fmt, ...) = error_dialog;

/*
 * Set the current input file name.
 */

void set_filename(char *filename)
{
	_filename = filename;
}

/*
 * Display an error dialog, possibly adding system error information.
 */

void error_dialog(const char *fmt, ...)
{
	va_list ap;
	char msgbuf[1024];
	char *bufp = msgbuf;

	/* A really rough sanity check to protect against blatant buffer overrun */
	if (strlen(fmt) > 750)
	{
		sprintf(msgbuf, "%s %s", "<buffer overflow> ", fmt);
	} 
	else 
	{
		if (_filename != NULL && strlen(_filename) < 255)
		{
			sprintf(msgbuf, "%s: ", _filename);
			bufp += strlen(msgbuf);
		}

		va_start(ap, fmt);
		
		vsprintf(bufp, fmt, ap);

		va_end(ap);

		if (errno != 0)
		{
			bufp = msgbuf + strlen(msgbuf);
			sprintf(bufp, " error is %s (%d)", strerror(errno), errno);
			errno = 0;
		}
	}

	MessageBox(NULL, msgbuf, "Error", 0);
}

void log_error(const char *fmt, ...)
{
	va_list ap;
	FILE *fp;
	char msgbuf[1024];
	char *bufp = msgbuf;

	/* A really rough sanity check to protect against blatant buffer overrun */
	if (strlen(fmt) > 750)
	{
		sprintf(msgbuf, "%s %s", "<buffer overflow> ", fmt);
	}
	else
	{
		if (_filename != NULL && strlen(_filename) < 255)
		{
			sprintf(msgbuf, "%s : ", _filename);
			bufp += strlen(msgbuf);
		}

		va_start(ap, fmt);

		vsprintf(bufp, fmt, ap);

		va_end(ap);

		if (errno != 0)
		{
			bufp = msgbuf + strlen(msgbuf);
			sprintf(bufp, " error is: %s (%d)", strerror(errno), errno);
			errno = 0;
		}
	}

	va_start(ap, fmt);

	if ((fp = fopen("oggdrop.log", "a")) == (FILE *)NULL)
		return;

	fprintf(fp, "%s\n", msgbuf);
	fflush(fp);
	fclose(fp);

	va_end(ap);
}

void set_use_dialogs(int use_dialogs)
{
	if (!use_dialogs)
		error_handler = error_dialog;
	else
		error_handler = log_error;
}


/******************************** end of misc.c ********************************/

