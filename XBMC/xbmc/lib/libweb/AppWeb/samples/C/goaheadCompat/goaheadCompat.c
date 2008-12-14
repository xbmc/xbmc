/*!	
 *	@file 	goaheadCompat.c
 *	@brief 	Demonstrate the GoAhead WebServer API compatibility
 */
/*************************************************************************/
/*
 *	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
 *	Portions Copyright (c) GoAhead Software, 1998-2000.
 *	The latest version of this code is available at http://www.mbedthis.com
 *
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version.
 *
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	See the GNU General Public License for more details at:
 *	http://www.mbedthis.com/downloads/gplLicense.html
 *	
 *	This General Public License does NOT permit incorporating this software 
 *	into proprietary programs. If you are unable to comply with the GPL, a 
 *	commercial license for this software and support services are available
 *	from Mbedthis Software at http://www.mbedthis.com
 */
/******************************* Includes *****************************/

#define		UNSAFE_FUNCTIONS_OK 1

#include	"appweb/appweb.h"

#if BLD_FEATURE_COMPAT_MODULE
/************************** Forward Declarations **********************/

static int	addMyExtensions();
static int	aspTest(int eid, webs_t wp, int argc, char_t **argv);
static void formTest(webs_t wp, char_t *path, char_t *query);
static void formWithError(webs_t wp, char_t *path, char_t *query);
static int  websHomePageHandler(webs_t wp, char_t *urlPrefix, 
				char_t *webDir, int arg, char_t *url, char_t *path, 
				char_t *query);

/********************************* Code *******************************/
/*
 * 	See the addMyExtensions routine for the use of GoAhead APIs 
 */

int main(int argc, char** argv)
{
	MaHttp		*http;		/* For the http service inside our app */
	MaServer	*server;	/* For a HTTP server */

	/*
	 *	Initialize the run-time and give our app a name "goaheadCompat"
	 */
	mprCreateMpr("goaheadCompat");

#if BLD_FEATURE_LOG
	/*
	 *	Do the following two statements only if you want debug trace
	 */
	mprAddLogFileListener();
	mprSetLogSpec("stdout:5");
#endif

	/*
	 *	Start run-time services
	 */
	mprStartMpr(0);

	/*
	 *	Create the HTTP and server objects. Give the server a name 
	 *	"default" and define "." as the default serverRoot, ie. the 
	 *	directory with the server configuration files.
	 */
	http = maCreateHttp();
	server = maCreateServer(http, "default", ".");
	
	/*
	 *	Activate the handlers. Only needed when linking statically.
	 */
	mprEspInit(0);
	mprEgiInit(0);
	mprCopyInit(0);

	/*
	 *	Configure the server based on the directives in goaheadCompat.conf.
	 */
	if (maConfigureServer(server, "goaheadCompat.conf") < 0) {
		fprintf(stderr, 
			"Can't configure the server. Error on line %d\n", 
			maGetConfigErrorLine(server));
		exit(2);
	}

	/*
	 *	Routine to demonstrate the GA Compatibility
	 */
	addMyExtensions();
	
	/*
	 *	Start serving pages. After this we are live.
	 */
	if (maStartServers(http) < 0) {
		fprintf(stderr, "Can't start the server\n");
		exit(2);
	}

	/*
	 *	Service events. This call will block until the server is exited
	 *	Call mprTerminate() at any time to instruct the server to exit.
	 *	The -1 is a timeout on the block. Useful if you use 
	 *	MPR_LOOP_ONCE and have a polling event loop.
	 */
	mprServiceEvents(MPR_LOOP_FOREVER, -1);

	/*
	 *	Stop all HTTP services
	 */
	maStopServers(http);

	/*
	 *	Delete the server and http objects
	 */
	maDeleteServer(server);
	maDeleteHttp(http);

	/*
	 *	Stop and delete the run-time services
	 */
	mprStopMpr();
	mprDeleteMpr();

	return 0;
}

/**********************************************************************/

static int addMyExtensions()
{
	void		*mp;
	char		*cp;
	sym_t		*sp;
	value_t		v;
	int			sd, rc;

	/*
	 *	Define ASP and goForm functions
	 */
	websAspDefine(T("aspTest"), aspTest);
	websFormDefine(T("formTest"), formTest);
	websFormDefine(T("formWithError"), formWithError);

	/*
	 *	URL handler for the home page
	 */
	websUrlHandlerDefine(T("/"), NULL, 0, websHomePageHandler, 0); 

	/*
 	 *	Test other miscellaneous routines. This is just to test the 
	 *	syntax and to demonstrate basic operation. For full usage 
	 *	details -- consult the GoAhead WebServer documentation.
	 */
	mp = balloc(B_L, 5);
	brealloc(B_L, mp, 50);
	bfree(B_L, mp);
	mp = 0;
	bfreeSafe(B_L, mp);
	
	fmtAlloc(&cp, 256, "Hello %s", "World");
	bfree(B_L, cp);

	sd = symOpen(59);
	a_assert(sd >= 0);
	v.type = string;
	v.value.string = "444 Lake City Way";
	symEnter(sd, "Peter Smith", v, 0);
	sp = symLookup(sd, "Peter Smith");
	a_assert(sp);
	rc = symDelete(sd, "Peter Smith");
	a_assert(rc == 0);
	symClose(sd);

	return 0;
}

/********************************* ASP ********************************/
/*
 *	Typcial asp function. Usage "aspTest name address"
 */
static int aspTest(int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*name, *address;

	a_assert(websValid(wp));

	if (ejArgs(argc, argv, T("%s %s"), &name, &address) < 2) {
		websError(wp, 400, T("Insufficient args\n"));
		return -1;
	}
	return websWrite(wp, T("Name: %s, Address %s"), name, address);
}

/******************************* Goforms ******************************/
/*
 *	Typcial GoForm function. Parameters name address
 */

static void formTest(webs_t wp, char_t *path, char_t *query)
{
	websRedirect(wp, "/newPage.html");
#if UNUSED
	char_t	*name, *address;

	/*
	 *	The second parameter is an optional default
	 */
	name = websGetVar(wp, T("name"), T("Joe Smith")); 
	address = websGetVar(wp, T("address"), T("1212 Milky Way Ave.")); 

	websHeader(wp);
	websWrite(wp, T("Name: %s, Address: %s\n"), 
		name, address);
	websFooter(wp);
	websDone(wp, 200);
#endif
}

/**********************************************************************/
/*
 *	GoForm returning an error to the browser
 */

static void formWithError(webs_t wp, char_t *path, char_t *query)
{
	websError(wp, 400, "Intentional error testing websError");
}

/**************************** URL Handlers ****************************/
/*
 *	URL handler for the home page. Called when "/" is requested.
 */

static int websHomePageHandler(webs_t wp, char_t *urlPrefix, 
	char_t *webDir, int arg, char_t *url, char_t *path, char_t *query)
{
	if (*url == '\0' || gstrcmp(url, T("/")) == 0) {
		websRedirect(wp, T("home.asp"));
		return 1;
	}
	return 0;
}

/**********************************************************************/
#else
int main()
{
	fprintf(stderr, "BLD_FEATURE_COMPAT_MODULE is not defined in config.h\n");
	exit(2);
}
#endif /* BLD_FEATURE_COMPAT_MODULE */

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
