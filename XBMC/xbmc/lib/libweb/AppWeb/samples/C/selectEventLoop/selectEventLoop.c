/*!
 *	@file 	selectEventLoop.c
 *	@brief 	Embed the AppWeb server in a simple single-threaded C
 *			application that uses a select event loop.
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

#if BLD_FEATURE_C_API_MODULE
/************************** Forward Declarations **********************/

static void eventLoop();

/********************************* Code *******************************/

int main(int argc, char** argv)
{
	MaHttp		*http;		/* For the http service inside our app */
	MaServer	*server;	/* For a HTTP server */

	/*
	 *	Initialize the run-time and give our app a name 
	 *	"selectEventLoop"
	 */
	mprCreateMpr("selectEventLoop");

#if BLD_FEATURE_LOG
	/*
	 *	Do the following two statements only if you want debug trace
	 */
	mprAddLogFileListener();
	mprSetLogSpec("stdout:4");
#endif

	/*
	 *	Start run-time services. Zero means single-threaded.
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
	 *	Activate the copy handler. Only needed when linking statically.
	 */	
	mprCopyInit(0);
	
	/*
	 *	Configure the server based on the directives in 
	 *	selectEventLoop.conf.
	 */
	if (maConfigureServer(server, "selectEventLoop.conf") < 0) {
		fprintf(stderr, 
			"Can't configure the server. Error on line %d\n", 
			maGetConfigErrorLine(server));
		exit(2);
	}

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
	 */
	eventLoop();

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
/*
 *	Sample main event loop using select. This demonstrates how to 
 *	integrate AppWeb with your applications event loop using select()
 */

static void eventLoop()
{
	struct timeval	timeout;
	fd_set			readFds, writeFds, exceptFds;
	fd_set			readInterest, writeInterest, exceptInterest;
#if WIN
	fd_set			*readp, *writep, *exceptp;
#endif
	int				maxFd, till, lastGet, readyFds;

	lastGet = -1;
	maxFd = 0;
	FD_ZERO(&readInterest);
	FD_ZERO(&writeInterest);
	FD_ZERO(&exceptInterest);

	while (!mprIsExiting()) {

		if (mprRunTimers() > 0) {
			till = 0;
		} else {
			till = mprGetIdleTime();
		}

		//
		//	This will run tasks if maxThreads == 0 (single threaded). If 
		//	multithreaded, the thread pool will run tasks
		//
		if (mprRunTasks() > 0) {			// Returns > 0 if more work to do
			till = 0;						// So don't block in select
		}

		//
		//	Mpr will populate with the FDs in use by MR on if necessary
		//
		if (mprGetFds(&readInterest, &writeInterest, &exceptInterest, 
				&maxFd, &lastGet)) {
			//
			//	Masks have been rebuilt, so add user fds here ....
			//
		}

		//
		//	Copy as select will modify readFds etc.
		//
		memcpy((void*) &readFds, (void*) &readInterest, sizeof(readFds));
		memcpy((void*) &writeFds, (void*) &writeInterest, sizeof(readFds));
		memcpy((void*) &exceptFds, (void*) &exceptInterest, sizeof(exceptFds));

		timeout.tv_sec = till / 1000;
		timeout.tv_usec = (till % 1000) * 1000;

#if WIN
		//
		//	Windows does not accept empty descriptor arrays
		//
		readp = (readFds.fd_count == 0) ? 0 : &readFds;
		writep = (writeFds.fd_count == 0) ? 0 : &writeFds;
		exceptp = (exceptFds.fd_count == 0) ? 0 : &exceptFds;
		readyFds = select(maxFd, readp, writep, exceptp, &timeout);
#else
		readyFds = select(maxFd, &readFds, &writeFds, &exceptFds, &timeout);
#endif

		if (readyFds > 0) {
			mprServiceIO(readyFds, &readFds, &writeFds, &exceptFds);
		}
	}
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
