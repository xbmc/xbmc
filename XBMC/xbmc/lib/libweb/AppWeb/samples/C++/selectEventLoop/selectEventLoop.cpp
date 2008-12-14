///
///	@file 	selectEventLoop.cpp
/// @brief 	Embed the AppWeb server in a single-threaded program using
///			a select event loop.
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
	Mpr			mpr("selectEventLoop");	// Initialize the run time

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
	//	This will be overridden in selectEventLoop.conf.
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
	if (server->configure("selectEventLoop.conf") < 0) {
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
//	Sample main event loop using select. This demonstrates how to 
//	integrate AppWeb with your applications event loop using select()
//

static void eventLoop()
{
	struct timeval	timeout;
	fd_set			readFds, writeFds, exceptFds;
	fd_set			readInterest, writeInterest, exceptInterest;
#if WIN
	fd_set			*readp, *writep, *exceptp;
#endif
	Mpr				*mpr;
	int				maxFd, till, lastGet, readyFds;

	mpr = mprGetMpr();
	lastGet = -1;
	maxFd = 0;
	FD_ZERO(&readInterest);
	FD_ZERO(&writeInterest);
	FD_ZERO(&exceptInterest);

	while (!mpr->isExiting()) {

		if (mpr->runTimers() > 0) {
			till = 0;
		} else {
			till = mpr->getIdleTime();
		}

		//
		//	This will run tasks if maxThreads == 0 (single threaded). If 
		//	multithreaded, the thread pool will run tasks
		//
		if (mpr->runTasks() > 0) {			// Returns > 0 if more work to do
			till = 0;						// So don't block in select
		}

		//
		//	Mpr will populate with the FDs in use by MR on if necessary
		//
		if (mpr->getFds(&readInterest, &writeInterest, &exceptInterest, 
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
			mpr->serviceIO(readyFds, &readFds, &writeFds, &exceptFds);
		}
	}
}

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
