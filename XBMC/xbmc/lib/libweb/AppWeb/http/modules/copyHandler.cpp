///
///	@file 	copyHandler.cpp
/// @brief 	Copy static content handler
/// @overview This handler manages static content such as HTML, GIF 
///		or JPEG pages. Data is returned to the user in the background 
///		using the DataStreams buffering layer.
//
/////////////////////////////////// Copyright //////////////////////////////////
//
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//
////////////////////////////////// Includes ////////////////////////////////////

#include	"copyHandler.h"

#if BLD_FEATURE_COPY_MODULE
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// CopyModule ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int mprCopyInit(void *handle)
{
	if (maGetHttp() == 0) {
		return MPR_ERR_NOT_INITIALIZED;
	}
	(void) new MaCopyModule(handle);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MaCopyModule::MaCopyModule(void *handle) : MaModule("copy", handle)
{
	copyHandlerService = new MaCopyHandlerService();
}

////////////////////////////////////////////////////////////////////////////////

MaCopyModule::~MaCopyModule()
{
	delete copyHandlerService;
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////// MaCopyHandlerService //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaCopyHandlerService::MaCopyHandlerService() : 
	MaHandlerService("copyHandler")
{
}

////////////////////////////////////////////////////////////////////////////////

MaCopyHandlerService::~MaCopyHandlerService()
{
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaCopyHandlerService::newHandler(MaServer *server, MaHost *host, 
	char *extensions)
{
	return new MaCopyHandler();
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MaCopyHandler //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaCopyHandler::MaCopyHandler() : MaHandler("copyHandler", 0, 
	MPR_HANDLER_GET | MPR_HANDLER_HEAD | MPR_HANDLER_HEAD | 
	MPR_HANDLER_TERMINAL)
{
}

////////////////////////////////////////////////////////////////////////////////

MaCopyHandler::~MaCopyHandler()
{
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaCopyHandler::cloneHandler()
{
	return new MaCopyHandler();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Override default matchRequest and handle all file types
//

int MaCopyHandler::matchRequest(MaRequest *rq, char *uri, int uriLen)
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

int MaCopyHandler::run(MaRequest *rq)
{
	MprFileInfo		*info;
	char			*fileName;
	bool			copyData;
	int				flags;

	fileName = rq->getFileName();
	mprAssert(fileName);

	flags = rq->getFlags();
	info = rq->getFileInfo();
	hitCount++;

	if (rq->openDoc(fileName) < 0) {
		rq->requestError(404, "Can't open: %s", fileName);
		rq->finishRequest();
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	} 

	mprLog(4, "copyHandler: serving %s\n", fileName);

	copyData = 1;

#if BLD_FEATURE_IF_MODIFIED
	if (flags & MPR_HTTP_IF_MODIFIED) {
		int		same;

		//
		//	If both checks, the last modification time and etag, claim that the 
		//	request doesn't need to be performed, skip the transfer.
		//
		same = rq->matchModified((int) info->mtime) && 
			rq->matchEtag(rq->getEtag());

#if BLD_FEATURE_RANGES
		if (rq->isRangedOutput()) {
			if (!same) {
				/*
				 *	Need to transfer the entire resource
				 */
				rq->deRangeOutput();
			}
		} else 
#endif
		if (same) {
			rq->setResponseCode(MPR_HTTP_NOT_MODIFIED);
			copyData = 0;
		}
 	}
#endif

	if (copyData) {
		if (flags & (MPR_HTTP_GET_REQUEST | MPR_HTTP_HEAD_REQUEST | 
				MPR_HTTP_POST_REQUEST)) {
			rq->insertDataStream(rq->getDocBuf());
			rq->getDocBuf()->setSize(info->size);
		}
		rq->setResponseCode(200);
	}

	rq->flushOutput(MPR_HTTP_BACKGROUND_FLUSH, MPR_HTTP_FINISH_REQUEST);
	return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
}

////////////////////////////////////////////////////////////////////////////////
#else
void mprCopyHandlerDummy() {}

#endif // BLD_FEATURE_COPY_MODULE
//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
