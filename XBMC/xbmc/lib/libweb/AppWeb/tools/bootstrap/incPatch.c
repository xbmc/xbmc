///
///	@file 	incPatch.cpp
/// @brief 	Patch include
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//////////////////////////////// Documentation /////////////////////////////////
///
///	usage:  incPatch -l includeDir [<fileList] [files ...]
///
////////////////////////////////// Includes ////////////////////////////////////

#include	<ctype.h>
#include	<signal.h>
#include	<sys/stat.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<time.h>
#if WIN || _WIN32
	#include	<direct.h>
	#include	<io.h>
#else
	#include	<libgen.h>
	#include	<unistd.h>
#endif
#include	<stdio.h>

#include	"getopt.h"
#include	"posixRemap.h"

/////////////////////////////////// Locals /////////////////////////////////////

#define MPR_MAX_STRING		8192
#define MPR_MAX_FNAME		1024

#if WIN
#define MPR_TEXT			"t"
#else
#define MPR_TEXT			""
#endif

static int		finished;
static int		verbose;
static char		*includeDir;

///////////////////////////// Forward Declarations /////////////////////////////

static char 	*mprStrTok(char *str, const char *delim, char **tok);
static char		*mprStrTrim(char *str, char c);
static void		openSignals();
static void		catchInterrupt(int signo);
static int 		patchFileList(FILE *fp);
static int 		patch(char *path);

///////////////////////////////////// Code /////////////////////////////////////

int main(int argc, char *argv[])
{
	int 		errors, c, i;

	verbose = errors = 0;
	includeDir = ".";

	while ((c = getopt(argc, argv, "l:v?")) != EOF) {
		switch(c) {
		case 'l':
			includeDir = optarg;
			break;

		case 'v':
			verbose++;
			break;

		case '?':
			errors++;
			break;
		}
	}
	if (errors) {
		fprintf(stderr, 
			"incPatch: usage: -l includeDir [<fileList] [files ...]\n");
		exit(2);
	}

	openSignals();

	if (optind >= argc) {
		patchFileList(stdin);
		
	} else {
		for (i = optind; !finished && i < argc; i++) {
			patch(argv[i]);
		}
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Read a list of files from stdin and patch each in-turn.
//	Keep going on errors.
//

static int patchFileList(FILE *fp)
{
	char	buf[MPR_MAX_FNAME];
	char	*cp;
	int		len;

	while (!finished && !feof(fp)) {
		if (fgets(buf, sizeof(buf), fp) == 0) {
			break;
		}
		cp = buf;
		for (cp = buf; *cp && isspace(*cp); ) {
			cp++;
		}
		len = strlen(cp);
		if (cp[len - 1] == '\n') {
			cp[len - 1] = '\0';
		}
		patch(cp);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Patch a file
//

static int patch(char *inputPath)
{
	FILE	*ifp, *ofp, *includeFp;
	char	buf[MPR_MAX_STRING], includePath[MPR_MAX_FNAME];
	char	tmpFile[MPR_MAX_FNAME];
	char	*prefix, *include, *directive;
	int		patched;
	
	patched = 0;
	prefix = 0;
	ifp = ofp = includeFp = 0;

	ifp = fopen(inputPath, "r" MPR_TEXT);
	if (ifp == 0) {
		fprintf(stderr, "Can't open %s\n", inputPath);
		return -1;
	}

	sprintf(tmpFile, "%s.new", inputPath);
	ofp = fopen(tmpFile, "w" MPR_TEXT);
	if (ofp == 0) {
		fprintf(stderr, "Can't open %s\n", tmpFile);
		goto error;
	}

	while (fgets(buf, sizeof(buf), ifp)) {
		mprStrTrim(buf, '\n');
		if ((directive = strstr(buf, "@copy")) == 0) {
			fprintf(ofp, "%s\n", buf);
			continue;
		}
		if (directive > buf && directive[-1] == '\"') {
			fprintf(ofp, "%s\n", buf);
			continue;
		}
		fprintf(ofp, "%s\n", buf);
		patched++;

		if (prefix == 0) {
			if (buf[0] == '/' && buf[1] == '/' && buf[2] == '/') {
				prefix = "///	";
			} else if (buf[0] == '/' && buf[1] == '/') {
				prefix = "//	";
			} else if (buf[0] == '/' && buf[1] == '*') {
				prefix = "/*	";
			} else if (buf[0] == ' ' && buf[1] == '*') {
				prefix = " *	";
			} else {
				prefix = " *	";
			}
		}

		//
		//	Found copy directive
		//
		mprStrTok(directive, " \t\r\n", &include);
		mprStrTrim(include, '\r');

		sprintf(includePath, "%s/%s", includeDir, include);
		includeFp = fopen(includePath, "r" MPR_TEXT);
		if (includeFp == 0) {
			fprintf(stderr, "Can't open %s\n for %s\n", includePath, inputPath);
			return -1;
		}

		//
		//	Output the included file (prepend prefix)
		//
		while (fgets(buf, sizeof(buf), includeFp)) {
			mprStrTrim(buf, '\n');
			fprintf(ofp, "%s%s\n", prefix, buf);
			continue;
		}
		fclose(includeFp);

		//
		//	Skip till we see @end
		//
		while (fgets(buf, sizeof(buf), ifp)) {
			mprStrTrim(buf, '\n');
			if (strstr(buf, "@end") != 0) {
				fprintf(ofp, "%s\n", buf);
				break;
			}
		}
	}
	fclose(ifp);
	fclose(ofp);

	if (verbose && patched) {
		printf("Patched: %s\n", inputPath);
	}

	if (! patched) {
		unlink(tmpFile);

	} else {
		//
		//	If we found an include directive, rename the patched file.
		//
		unlink(inputPath);
		if (rename(tmpFile, inputPath) < 0) {
			fprintf(stderr, "Can't rename %s to %s\n", tmpFile, inputPath);
			return -1;
		}
	}
	return 0;

error:
	if (ifp) {
		fclose(ifp);
	}
	if (ofp) {
		fclose(ofp);
	}
	return -1;
}
	
////////////////////////////////////////////////////////////////////////////////
//
//	Initialize signals
//

static void openSignals() 
{
#if !WIN && !_WIN32
	struct sigaction	act;

	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);

	act.sa_handler = catchInterrupt;
	sigaction(SIGINT, &act, 0);
#endif
}

////////////////////////////////////////////////////////////////////////////////

static void catchInterrupt(int signo)
{
	finished++;
}

////////////////////////////////////////////////////////////////////////////////

char* mprStrTrim(char *str, char c)
{
	if (str == 0) {
		return str;
	}
	while (*str == c) {
		str++;
	}
	while (str[strlen(str) - 1] == c) {
		str[strlen(str) - 1] = '\0';
	}
	return str;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Thread-safe wrapping of strtok. Note "str" is modifed as per strtok()
//

char *mprStrTok(char *str, const char *delim, char **tok)
{
	char	*start, *end;
	int		i;

	start = str ? str : *tok;

	if (start == 0) {
		return 0;
	}
	
	i = strspn(start, delim);
	start += i;
	if (*start == '\0') {
		*tok = 0;
		return 0;
	}
	end = strpbrk(start, delim);
	if (end) {
		*end++ = '\0';
		i = strspn(end, delim);
		end += i;
	}
	*tok = end;
	return start;
}

///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4
//
