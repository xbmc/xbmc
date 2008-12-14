///
///	@file 	configByApi.cpp
/// @brief 	Configure AppWeb manually by API to minimize memory footprint.
//			in a simple single-threaded application.
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

/////////////////////////// Forward Declarations ///////////////////////

static int configure(MaHttp *http);

////////////////////////////////// Locals //////////////////////////////

MaServer	*server;					// For the HTTP server

/////////////////////////////////// Code ///////////////////////////////

int main(int argc, char** argv)
{
	MaHttp		*http;					// Http service inside our app
	Mpr			mpr("configByApi");		// Initialize the run time

#if BLD_FEATURE_LOG
	//
	//	Do the following two statements only if you want debug trace
	//
	mpr.addListener(new MprLogToFile());
	mpr.setLogSpec("stdout:4");
#endif

	//
	//	Start the Mbedthis Portable Runtime (single-threaded)
	//
	mpr.start(0);

	//
	//	Create Http, Server and Host objects for this application.
	//
	http = new MaHttp();
	
	if (configure(http) < 0) {
		mprFprintf(MPR_STDERR, "Can't configure the server\n");
		exit(2);
	}

	//
	//	Start the http service
	//
	if (http->start() < 0) {
		mprFprintf(MPR_STDERR, "Can't start the server\n");
		exit(2);
	}

	//
	//	Loop servicing events.
	//
	mpr.serviceEvents(0, -1);

	//
	//	Orderly shutdown
	//
	http->stop();

	if (server) {
		delete server;
	}
	delete http;

	//
	//	MPR run-time will automatically stop and be cleaned up
	//
	return 0;
}

////////////////////////////////////////////////////////////////////////
//
//	Configure the http server
//

static int configure(MaHttp *http)
{
	MaHost			*host;
	MaDir			*dir;
	MaAlias			*ap;
	MaLocation		*loc;
	char			*documentRoot, *serverRoot;
	char			pathBuf[MPR_MAX_FNAME];

	serverRoot = ".";
	documentRoot = ".";

	//
	//	Create a server and set the server root directory to "."
	//
	server = new MaServer(http, "default", ".");

	//
	//	Create a new host and define the document root
	//
	host = server->newHost(documentRoot, "127.0.0.1:5555");
	if (host == 0) {
		return MPR_ERR_CANT_OPEN;
	}

	//
	//	Load modules that are to be statically loaded
	//
	new MaCopyModule(0);

#if IF_YOU_WISH
	new MaAuthModule(0);
	new MaAdminModule(0);
	new MaAuthModule(0);
	new MaEspModule(0);
	new MaCgiModule(0);
	new MaCompatModule(0);
	new MaCapiModule(0);
	new MaEgiModule(0);
	new MaSslModule(0);
	new MaMatrixSslModule(0);
	new MaOpenSslModule(0);
	new MaPhp4Module(0);
	new MaPhp5Module(0);
	new MaUploadModule(0);
#endif

	//
	//	Now load the dynamically loadable modules and activate the statically
	//	linked modules from above. loadModule will do this for us.
	//

#if IF_YOU_WISH
	//	
	//	This handler must be added first to authorize all requests.
	//
	if (server->loadModule("auth") == 0) {
		host->addHandler("authHandler", "");
	}

	if (server->loadModule("upload") == 0) {
		host->addHandler("uploadHandler", "");
	}

	if (server->loadModule("cgi") == 0) {
		host->addHandler("cgiHandler", ".cgi .cgi-nph .bat .cmd .pl .py");
	}

	if (server->loadModule("egi") == 0) {
		host->addHandler("egiHandler", ".egi");
	}

	if (server->loadModule("esp") == 0) {
		host->addHandler("espHandler", ".esp .asp");
	}

	server->loadModule("ssl");
	server->loadModule("matrixSsl");
#endif

	if (server->loadModule("copy") == 0) {
		host->addHandler("copyHandler", "");
	}

	//
	//	Create the top level directory
	//
	dir = new MaDir(host);
	dir->setPath(documentRoot);
	host->insertDir(dir);

	//
	//	Add cgi-bin with a location block for the /cgi-bin URL prefix.
	//
	mprSprintf(pathBuf, sizeof(pathBuf), "%s/cgi-bin", serverRoot);
	ap = new MaAlias("/cgi-bin/", pathBuf);
	mprLog(4, "ScriptAlias \"/cgi-bin/\":\n\t\t\t\"%s\"\n", pathBuf);
	host->insertAlias(ap);
	loc = new MaLocation(dir->getAuth());
	loc->setPrefix("/cgi-bin/");
	loc->setHandler("cgiHandler");
	host->insertLocation(loc);

	return 0;
}

////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
