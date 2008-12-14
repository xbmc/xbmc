///
///	@file 	pollEventLoop.cpp
/// @brief 	Embed the AppWeb server in a single-threaded program using
///			a polled event loop.
///
////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	The latest version of this code is available at http://www.mbedthis.com
//
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version.
//
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	See the GNU General Public License for more details at:
//	http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This General Public License does NOT permit incorporating this software 
//	into proprietary programs. If you are unable to comply with the GPL, a 
//	commercial license for this software and support services are available
//	from Mbedthis Software at http://www.mbedthis.com
//
/////////////////////////////// Includes ///////////////////////////////

#include	"appweb/appweb.h"

//////////////////////////// Forward Declarations //////////////////////

static void eventLoop();

/////////////////////////////////// Code ///////////////////////////////

int main(int argc, char** argv)
{
	MaHttp		*http;					// Http service inside our app
	MaServer	*server;				// For the HTTP server
	Mpr			mpr("pollEventLoop");	// Initialize the run time

#if BLD_FEATURE_LOG
	//
	//	Do the following two statements only if you want debug trace
	//
	mpr.addListener(new MprLogToFile());
	mpr.setLogSpec("stdout:4");
#endif

	//
	//	Start the Mbedthis Portable Runtime and request single threading
	//
	mpr.start();

	//
	//	Create Http and Server objects for this application. We set the
	//	ServerName to be "default" and the initial serverRoot to be ".".
	//	This will be overridden in pollEventLoop.conf.
	//
	http = new MaHttp();
	server = new MaServer(http, "default", ".");
	
	//
	//	Activate the copy module and handler
	//
	new MaCopyModule(0);

	//
	//	Configure the server with the configuration directive file
	//
	if (server->configure("pollEventLoop.conf") < 0) {
		mprFprintf(MPR_STDERR, 
			"Can't configure the server. Error on line %d\n", 
			server->getLine());
		exit(2);
	}
	
	//
	//	Start the server
	//
	if (http->start() < 0) {
		mprFprintf(MPR_STDERR, "Can't start the server\n");
		exit(2);
	}

	//
	//	Service incoming requests until time to exit.
	//
	eventLoop();

	//
	//	Orderly shutdown
	//
	http->stop();
	delete server;
	delete http;

	//
	//	MPR run-time will automatically stop and be cleaned up
	//
	return 0;
}

////////////////////////////////////////////////////////////////////////
//
//	Sample polled event loop. This demonstrates how to integrate AppWeb 
//	with your applications event loop using a polling architecture.
//

static void eventLoop()
{
	Mpr				*mpr;
	int			till, timeout;

	mpr = mprGetMpr();

	//
	//	We will nap for 50 milliseconds to avoid busy waiting
	//
	timeout = 50;

	while (!mpr->isExiting()) {

		mpr->runTimers();
		till = mpr->getIdleTime();

		//
		//	Run any queued tasks
		//
		if (mpr->runTasks() > 0) {			// Returns > 0 if more work to do
			till = 0;						// So don't block in serviceEvents
		}

		//
		//	Do some work here
		//
		

		//
		//	Now service any pending I/O
		//
		mpr->serviceEvents(1, min(timeout, till));
	}
}

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
