///
///	@file 	getopt.cpp
/// @brief 	Standard getopt facility
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
////////////////////////////////// Includes ////////////////////////////////////
#if WIN || _WIN32 || CYGWIN

#include 	<stdio.h>
#include 	<string.h>

#if WIN || _WIN32
#include	"posixRemap.h"
#endif

////////////////////////////////// Defines /////////////////////////////////////

#define V_ERR(s, c) err(argv[0], s, c);

////////////////////////////////// Globals /////////////////////////////////////

int		opterr = 1;
int		optind = 1;
int		optopt;
char*	optarg;
char	optswi = '-';

///////////////////////////////////// Code /////////////////////////////////////

static void err(char* a, char* s, int c)
{	
	if (opterr){
		fputs(a, stderr);
		fputs(s, stderr);
		fputc(c, stderr);
		fputc('\n', stderr);
	}
}

////////////////////////////////////////////////////////////////////////////////

int	getopt(int argc, char* const * argv, const char* opts)
{
	static int sp = 1;
	register int c;
	register char *cp;
	char noswitch[3];

	memset(noswitch, optswi, 2);

	noswitch[2] = 0;
	if (sp == 1) {
		if (optind >= argc ||
		   argv[optind][0] != optswi || argv[optind][1] == '\0') {
			return(EOF);
		} else if (strcmp(argv[optind], noswitch) == 0) {
			optind++;
			return(EOF);
		}
	}
	optopt = c = argv[optind][sp];
	if (c == ':' || (cp = (char*) strchr(opts, c)) == NULL) {
		V_ERR(": illegal option -- ", c);
		if (argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}
	if (*++cp == ':') {
		if (argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if (++optind >= argc) {
			V_ERR(": option requires an argument -- ", c);
			sp = 1;
			return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		if (argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return(c);
}


////////////////////////////////////////////////////////////////////////////////

#endif /* WIN */

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4
//
