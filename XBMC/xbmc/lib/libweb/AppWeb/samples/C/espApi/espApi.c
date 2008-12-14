/*!
 *	@file 	esp.c
 *	@brief 	Demonstrate the use of Embedded Server Pages (ESP) in a 
 * 			simple multi-threaded application.
 */
/*************************************************************************/
/*
 *	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
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

/********************************* Code *******************************/
#if BLD_FEATURE_C_API_MODULE

/*
 *	Function that is run when the ESP procedure is called from the web
 *	page. ep is the ESP context. argv is the parameters to the
 *	ESP procedure defined in the web page. The arguments will be automatically
 *	converted to strings on entry.
 */

static int echoArgs(EspRequest *ep, int argc, char **argv)
{
	char	*s;
	int		i;

	/*
	 *	There are a suite of write calls available. This just writes a
	 *	string.
	 */
	espWriteString(ep, "<h1>Echo Arg</h1><p>: ");

	mprAssert(argv);
	for (i = 0; i < argc; ) {
		s = argv[i];
		espWriteString(ep, s);
		if (++i < argc) {
			espWriteString(ep, " ");
		}
	}
	espWriteString(ep, "</p>");

	/*
	 *	Procedures can return a result
	 */
	espSetReturnString(ep, "sunny day");
	return 0;
}

/**********************************************************************/
/*
 *	This function is just to demonstrate the ESP API. It really does
 *	no useful purpose. The arguments may be any JavaScript type passed
 *	via the call in the ESP page.
 */

static int testApi(EspRequest *ep, int argc, MprVar **argv)
{
	MprVar	value, obj, *vp;
	int		rc, count;

	//
	//	Get ESP standard variable
	//
	rc = espGetVar(ep, MA_REQUEST_OBJ, "REQUEST_URI", &value);
	if (rc != 0 || value.type != MPR_TYPE_STRING) {
		espError(ep, "Can't read request[REQUEST_URI]");
		return -1;
	}

	//
	//	Create and set a custom ESP form[] variable
	//
	espSetVar(ep, MA_FORM_OBJ, "myField1", mprCreateStringVar("name", 0));
	rc = espGetVar(ep, MA_FORM_OBJ, "myField1", &value);
	if (rc != 0 || strcmp(value.string, "name") != 0) {
		espError(ep, "Can't read myField1");
		return -1;
	}
	rc = espUnsetVar(ep, MA_FORM_OBJ, "myField1");
	if (rc != 0) {
		espError(ep, "Can't unset myField1");
		return -1;
	}

	//
	//	Read arbitrary ESP variable
	//
	rc = espReadVar(ep, "global.request['REQUEST_URI']", &value);
	if (rc != 0 || value.type != MPR_TYPE_STRING) {
		espError(ep, "Can't read global.request[REQUEST_URI]");
		return -1;
	}

	//
	//	Test write to ESP variables
	//
	rc = espWriteVarValue(ep, "global.request['REQUEST_URI']", 
		mprCreateStringVar("/modified/uri.html", 0));
	if (rc < 0) {
		espError(ep, "Can't write global.request[REQUEST_URI]");
		return -1;
	}

	//
	//	Test direct read / write of javascript variables
	//
	rc = espReadVar(ep, "global.request['REQUEST_URI']", &value);
	if (rc != 0 || value.type != MPR_TYPE_STRING) {
		espError(ep, "Can't read global.request[REQUEST_URI]");
		return -1;
	}
	if (strcmp(value.string, "/modified/uri.html") != 0) {
		espError(ep, "Bad value for global.request[REQUEST_URI]");
		return -1;
	}

	//
	//	Custom create an object with method and then run the method on
	//	the object.
	//
	obj = espCreateObjVar("myObj", 0);
	if (obj.type != MPR_TYPE_OBJECT) {
		espError(ep, "Can't create object");
		return -1;
	}
	if (espCreatePropertyValue(&obj, "address", 
			mprCreateStringVar("121 Park Avenue", 0)) == 0) {
		espError(ep, "Can't create address property");
		return -1;
	}

	//
	//	Manually run a function on an object.
	//
	if (espRunFunction(ep, &obj, "toString", 0) < 0) {
		espError(ep, "Can't run toString method on object");
		return -1;
	}
	vp = espGetResult(ep);
	if (vp->type != MPR_TYPE_STRING || 
			strcmp(vp->string, "[object this]") != 0) {
		espError(ep, "Bad result from toString method");
		return -1;
	}

	//
	//	Test low level property access routines.
	//
	vp = espGetProperty(&obj, "address", &value);
	if (rc < 0 || value.type != MPR_TYPE_STRING || 
			strcmp(value.string, "121 Park Avenue") != 0) {
		espError(ep, "Can't read address property");
		return -1;
	}

	//
	//	Test enumeration
	//
	count = 0;
	vp = espGetFirstProperty(&obj, MPR_ENUM_DATA);
	while (vp) {
		count++;
		if (strcmp(vp->name, "address") != 0) {
			espError(ep, "Bad property name");
			return -1;
		}
		vp = espGetNextProperty(&obj, vp, MPR_ENUM_DATA);
	}
	if (count != 1) {
		espError(ep, "Bad iteration over properties");
		return -1;
	}

	if (espDeleteProperty(&obj, "address") < 0) {
		espError(ep, "Can't create address property");
		return -1;
	}

	return 0;
}

/**********************************************************************/

int main(int argc, char** argv)
{
	MaHttp		*http;		/* For the http service inside our app */
	MaServer	*server;	/* For a HTTP server */

	/*
	 *	Initialize the run-time and give our app a name "esp"
	 */
	mprCreateMpr("esp");

#if BLD_FEATURE_LOG
	/*
	 *	Do the following two statements only if you want debug trace
	 */
	mprAddLogFileListener();
	mprSetLogSpec("stdout:4");
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
	mprCopyInit(0);

	/*
	 *	Configure the server based on the directives in 
	 *	esp.conf.
	 */
	if (maConfigureServer(server, "espApi.conf") < 0) {
		fprintf(stderr, 
			"Can't configure the server. Error on line %d\n", 
			maGetConfigErrorLine(server));
		exit(2);
	}

	/*
	 *	Define our ESP procedures
	 */
	espDefineStringCFunction(0, "echoArgs", echoArgs, 0);
	espDefineCFunction(0, "testApi", testApi, 0);
	
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
#else /* BLD_FEATURE_C_API_MODULE */

int main()
{
	fprintf(stderr, "BLD_FEATURE_C_API_MODULE is not defined in config.h\n");
	exit(2);
}
#endif /* BLD_FEATURE_C_API_MODULE */

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
