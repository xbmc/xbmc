///
///	@file 	simpleServer.cpp
/// @brief 	Embed the AppWeb server in a simple multi-threaded 
//			application.
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

/////////////////////////////////// Code ///////////////////////////////

int main(int argc, char** argv)
{
	MaHttp		*http;					// Http service inside our app
	MaServer	*server;				// For the HTTP server
	Mpr			mpr("simpleServer");	// Initialize the run time
    MaHost      *host = NULL;
    MaHandler   *handler;

#if BLD_FEATURE_LOG
	//
	//	Do the following two statements only if you want debug trace
	//
	mpr.addListener(new MprLogToFile());
	mpr.setLogSpec("stdout:4");
#endif

	//
	//	Start the Mbedthis Portable Runtime with 12 pool threads
	//
	mpr.start(MPR_SERVICE_THREAD);

#if BLD_FEATURE_MULTITHREAD
	mpr.poolService->setMaxPoolThreads(10);
#endif

	//
	//	Create Http and Server objects for this application. We set the
	//	ServerName to be "default" and the initial serverRoot to be ".".
	//	This will be overridden in simpleServer.conf.
	//
	http = new MaHttp();
	server = new MaServer(http, "default", ".", "127.0.0.1", 8080);
    host = server->newHost(".");
	//
	//	Activate the copy module and handler
	//
	new MaCopyModule(0);
    host->addHandler("copyHandler","");
    host->insertAlias(new MaAlias("/","index.html",404));
    //server->insertAfter(handler);

	//
	//	Configure the server with the configuration directive file
	//
    /*
	if (server->configure("simpleServer.conf") < 0) {
		mprFprintf(MPR_STDERR, 
			"Can't configure the server. Error on line %d\n", 
			server->getLine());
		exit(2);
	}
	*/
	//
	//	Start the server
	//
	if (server->start() < 0) {
		mprFprintf(MPR_STDERR, "Can't start the server\n");
		exit(2);
	}

	//
	//	Tell the MPR to loop servicing incoming requests. We can 
	//	replace this call with a variety of event servicing 
	//	mechanisms offered by AppWeb.
	//
	mpr.serviceEvents(0, -1);

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

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
