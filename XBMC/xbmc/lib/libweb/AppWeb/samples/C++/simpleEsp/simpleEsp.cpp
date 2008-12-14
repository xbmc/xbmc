///
///	@file 	simpleEsp.cpp
/// @brief 	Demonstrate the use of Embedded Server Pages (ESP) in a 
///			simple multi-threaded application.
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

////////////////////////// Forward Declarations ////////////////////////
#if BLD_FEATURE_ESP_MODULE

static int helloWorld(EspRequest *ep, int argc, char **argv);

/////////////////////////////////// Code ///////////////////////////////

int main(int argc, char** argv)
{
	MaHttp		*http;					// Http service inside our app
	MaServer	*server;				// For the HTTP server
	Mpr			mpr("simpleEsp");		// Initialize the run time

#if BLD_FEATURE_LOG
	//
	//	Do the following two statements only if you want debug trace
	//
	mpr.addListener(new MprLogToFile());
	mpr.setLogSpec("stdout:4");
#endif

	//
	//	Start the Mbedthis Portable Runtime with 10 pool threads
	//
	mpr.start(MPR_SERVICE_THREAD);

#if BLD_FEATURE_MULTITHREAD
	mpr.poolService->setMaxPoolThreads(10);
#endif

	//
	//	Create Http and Server objects for this application. We set the
	//	ServerName to be "default" and the initial serverRoot to be ".".
	//	This will be overridden in simpleEsp.conf.
	//
	http = new MaHttp();
	server = new MaServer(http, "default", ".");
	
	//
	//	Activate the ESP module and handler
	//
	new MaEspModule(0);
	new MaCopyModule(0);

	//
	//	Configure the server with the configuration directive file
	//
	if (server->configure("simpleEsp.conf") < 0) {
		mprFprintf(MPR_STDERR, 
			"Can't configure the server. Error on line %d\n", 
			server->getLine());
		exit(2);
	}

	//
	//	Define our ESP procedures
	//
	espDefineStringCFunction(0, "helloWorld", helloWorld, 0);
	
	//
	//	Start the server
	//
	if (http->start() < 0) {
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

////////////////////////////////////////////////////////////////////////
//
//	Function that is run when the ESP procedure is called from the web
//	page. ep is the ESP context handle. argv contains the parameters to the
//	ESP procedure defined in the web page.
//

static int helloWorld(EspRequest *ep, int argc, char **argv)
{
	char	*s;
	int		i;

	//
	//	There are a suite of write calls available. This just writes a
	//	string.
	//
	espWriteString(ep, "<h1>Hello World</h1><p>Args: ");

	mprAssert(argv);
	for (i = 0; i < argc; ) {
		s = argv[i];
		espWriteString(ep, s);
		if (++i < argc) {
			espWriteString(ep, " ");
		}
	}
	espWriteString(ep, "</p>");

	//
	//	Procedures can return a result
	//
	espSetReturnString(ep, "sunny day");
	return 0;
}

////////////////////////////////////////////////////////////////////////
#else
int main()
{
	fprintf(stderr, "BLD_FEATURE_ESP_MODULE is not defined in config.h\n");
	exit(2);
}
#endif /* BLD_FEATURE_ESP_MODULE */

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
