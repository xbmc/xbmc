///
///	@file 	simpleHandler.cpp
/// @brief 	Create a simple AppWeb dynamically loadable module
///
///	This sample demonstrates creating a simple module that can be 
///	dynamically or statically loaded into the AppWeb server. Modules 
///	can be URL handlers, Scripting Engines or just extension APIs to 
///	extend the AppWeb server.
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

#define		IN_SIMPLE_HANDLER 1

#include	"appweb/appweb.h"
#include	"simpleHandler.h"

////////////////////////////////////////////////////////////////////////
////////////////////////// SimpleHandlerModule /////////////////////////
////////////////////////////////////////////////////////////////////////

//
//	Module entry point. This is only called when the module is loaded 
//	as a DLL.
//

int mprSimpleHandlerInit(void *handle)
{
	mprLog(0, "In SimpleHandlerInit()\n");
	new SimpleHandlerModule(handle);
	return 0;
}

////////////////////////////////////////////////////////////////////////

///
///	Constructor. Called either above when the DLL is loaded or from the 
///	application main if loading statically.
///

SimpleHandlerModule::SimpleHandlerModule(void *handle) : 
	MaModule("simpleHandler", handle)
{
	mprLog(0, "In SimpleHandlerModule()\n");

	//
	//	Create the handler service (one per application) and insert into
	//	the HTTP service.
	//
	simpleService = new SimpleHandlerService();
}

////////////////////////////////////////////////////////////////////////

///
///	Module destructor
///

SimpleHandlerModule::~SimpleHandlerModule()
{
	mprLog(0, "In ~SimpleHandlerModule()\n");
	delete simpleService;
}

////////////////////////////////////////////////////////////////////////
///////////////////////// SimpleHandlerService /////////////////////////
////////////////////////////////////////////////////////////////////////

///
///	Constructor for the SimpleHandler service. An instance is created 
///	for each host (default and virtual).
///

SimpleHandlerService::SimpleHandlerService() : 
	MaHandlerService("simpleHandler")
{
	mprLog(0, "In SimpleHandlerService()\n");
}

////////////////////////////////////////////////////////////////////////

///
///	Destructor for the SimpleHandler. 
///

SimpleHandlerService::~SimpleHandlerService()
{
	mprLog(0, "In ~SimpleHandlerService()\n");
}

////////////////////////////////////////////////////////////////////////

///
///	Setup the SimpleHandler service. One-time setup for a given host.
///

int SimpleHandlerService::setup()
{
	mprLog(0, "In SimpleHandlerService:setup()\n");
	return 0;
}

////////////////////////////////////////////////////////////////////////

///
///	New Handler factory. Create a new SimpleHandler for an incoming 
///	HTTP request for a given server
///

MaHandler *SimpleHandlerService::newHandler(MaServer *server, 
	MaHost *host, char *extensions)
{
	mprLog(0, "In SimpleHandlerService:newHandler()\n");
	return new SimpleHandler(extensions);
}

////////////////////////////////////////////////////////////////////////
///////////////////////////// SimpleHandler ////////////////////////////
////////////////////////////////////////////////////////////////////////

///
///	A SimpleHandler is created for each incoming HTTP request. We tell 
///	the Handler base class that we will be a terminal handler. Handlers 
///	can be non-terminal where they can modify the request, but not 
///	actually handle it. This handler only accepts "GET" requests.
///

SimpleHandler::SimpleHandler(char *extensions) :
	MaHandler("simpleHandler", extensions, 
	MPR_HANDLER_GET | MPR_HANDLER_POST | MPR_HANDLER_TERMINAL)
{
	mprLog(0, "In SimpleHandler::simpleHandler()\n");
}

////////////////////////////////////////////////////////////////////////
 
///
///	Destructor for the SimpleHandler
///

SimpleHandler::~SimpleHandler()
{
	mprLog(0, "In SimpleHandler::~simpleHandler()\n");
}

////////////////////////////////////////////////////////////////////////

///
///	For maximum speed in servicing requests, we clone a pre-existing 
///	handler when an incoming request arrives.
///

MaHandler *SimpleHandler::cloneHandler()
{
	SimpleHandler	*ep;

	mprLog(0, "In SimpleHandler::cloneHandler()\n");
	ep = new SimpleHandler(extensions);
	return ep;
}

////////////////////////////////////////////////////////////////////////

///
///	Called to setup any data structures before running the request
///

int SimpleHandler::setup(MaRequest *rq)
{
	mprLog(0, "In SimpleHandler::setup()\n");
	return 0;
}

////////////////////////////////////////////////////////////////////////

///
///	Called to see if this handler should process this request.
///	Return TRUE if you want this handler to match this request
///

int SimpleHandler::matchRequest(MaRequest *rq, char *uri, int uriLen)
{
	MprStringData	*sd;
	int				len;

	//
	//	Example custom matching. 
	//	For example, to match against URLs that start with "/myUrl":
	//
	if (strncmp(uri, "/myUrl", 6) == 0) {
		return 1;
	}

	//
	//	Match against extensions defined for this handler
	//
	sd = (MprStringData*) extList.getFirst();
	while (sd) {
		len = strlen(sd->string);
		if (uriLen > len && 
			strncmp(sd->string, &uri[uriLen - len], len) == 0) {
			return 1;
		}
		sd = (MprStringData*) extList.getNext(sd);
	}

	//
	//	No match. The next handler in the chain will match.
	//
	return 0;
}

////////////////////////////////////////////////////////////////////////

///
///	Receive any post data. Will be called after the run() method has 
///	been called and may be called multiple times. End of data is when 
///	remainingContent() == 0.
///

void SimpleHandler::postData(MaRequest *rq, char *buf, int len)
{
	mprLog(0, 
		"SimpleHandler::postData: postData %d bytes, remaining %d\n", 
		rq->getFd(), len, rq->getRemainingContent());

	if (len < 0 && rq->getRemainingContent() > 0) {
		rq->requestError(400, "Incomplete post data");
		return;
	}

	if (rq->getRemainingContent() <= 0) {
		/*
		 *	If we have all the post data, create query variables for it
		 *
		 *	MaHeader	*header;
		 *	header = rq->getHeader();
		 *	if (mprStrCmpAnyCase(header->contentMimeType, 
		 *			"application/x-www-form-urlencoded") == 0) {
		 *
		 *	Change this to be wherever you stuffed the post data
		 *
		 *		rq->createQueryVars(postBuffer, totalLength);
		 *	}
		 */
		
		/*
		 *	Now that we have all the post data, execute the run handler 
		 */
		run(rq);

	} else {

		//	Do something with the post data
	}
}

////////////////////////////////////////////////////////////////////////

///
///	Run the handler to service the request
///

int SimpleHandler::run(MaRequest *rq)
{
	MprFileInfo		info;
	MaDataStream	*dynBuf;
	char			*fileName;
	int				flags;

	hitCount++;

	flags = rq->getFlags();

	if (flags & MPR_HTTP_POST_REQUEST && rq->getRemainingContent() > 0) {
		//
		//	When all the post data is received the run method will be recalled
		//	by the postData method.
		//
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}

	dynBuf = rq->getDynBuf();

	//
	//	Set a default "success" response with HTML content. 
	//
	rq->setResponseCode(200);
	rq->setResponseMimeType("text/html");

	//
	//	If we are generating dynamic data, we should add a cache control
	//	header to the response.
	//
	rq->setHeaderFlags(MPR_HTTP_DONT_CACHE, 0);

	//
	//	Open a document to return to the client
	//
	fileName = rq->getFileName();
	if (rq->openDoc(fileName) < 0) {
		rq->requestError(404, "Can't open document: %s", fileName);
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	} 
	mprLog(4, "%d: serving: %s\n", rq->getFd(), fileName);

	//
	//	Insert the document DataStream and define the file size
	//	Alternatively you could dynamically generate data and use the 
	//	Dyn buffer.
	//
	if (!(flags & MPR_HTTP_HEAD_REQUEST)) {
		rq->insertDataStream(rq->getDocBuf());
		rq->statDoc(&info);
		rq->getDocBuf()->setSize(info.size);
	}

	//
	//	Flush in the background
	//
	rq->flushOutput(MPR_HTTP_BACKGROUND_FLUSH, MPR_HTTP_FINISH_REQUEST);
	return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
}

////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4
//
