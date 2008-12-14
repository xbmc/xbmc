///
///	@file 	simpleClient.cpp
/// @brief 	Simple client using the GET method to retrieve a web page.
///
///	This sample demonstrates retrieving content using the HTTP GET 
///	method via the Client class. This is a multi-threaded application.
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
////////////////////////////// Includes ////////////////////////////////

#include	"appweb/appweb.h"

////////////////////////////////// Code ////////////////////////////////

int main(int argc, char** argv)
{
	MaClient	*client;
	Mpr			mpr("simpleClient");
	char		*content;
	int			code, contentLen;

	//
	//	Start the Mbedthis Portable Runtime
	//
	mpr.start(MPR_SERVICE_THREAD);

	//
	//	Get a client object to work with. We can issue multiple 
	//	requests with this one object.
	//
	client = new MaClient();

	//
	//	Get a URL
	//
	if (client->getRequest("http://www.mbedthis.com/index.html") < 0) {
		mprFprintf(MPR_STDERR, "Can't get URL");
		exit(2);
	}

	//
	//	Examine the HTTP response HTTP code. 200 is success.
	//
	code = client->getResponseCode();
	if (code != 200) {
		mprFprintf(MPR_STDERR, "Server responded with code %d\n", code);
		exit(1);
	} 

	//
	//	Get the actual response content
	//
	content = client->getResponseContent(&contentLen);
	if (content) {
		mprPrintf("Server responded with:\n");
		write(1, content, contentLen);
	}

	delete client;
	return 0;
}

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
