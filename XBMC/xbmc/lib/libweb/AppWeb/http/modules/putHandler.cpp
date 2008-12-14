///
///	@file 	putHandler.cpp
/// @brief 	Put resources to the server
/// @overview 
//
/////////////////////////////////// Putright //////////////////////////////////
//
//	@put	default
//	
//	Putright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
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

#include	"putHandler.h"

#if BLD_FEATURE_PUT_MODULE
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// PutModule ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int mprPutInit(void *handle)
{
	if (maGetHttp() == 0) {
		return MPR_ERR_NOT_INITIALIZED;
	}
	(void) new MaPutModule(handle);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MaPutModule::MaPutModule(void *handle) : MaModule("put", handle)
{
	putHandlerService = new MaPutHandlerService();
}

////////////////////////////////////////////////////////////////////////////////

MaPutModule::~MaPutModule()
{
	delete putHandlerService;
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////// MaPutHandlerService //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaPutHandlerService::MaPutHandlerService() : 
	MaHandlerService("putHandler")
{
}

////////////////////////////////////////////////////////////////////////////////

MaPutHandlerService::~MaPutHandlerService()
{
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaPutHandlerService::newHandler(MaServer *server, MaHost *host, 
	char *extensions)
{
	return new MaPutHandler();
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MaPutHandler //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaPutHandler::MaPutHandler() : MaHandler("putHandler", 0, 
	MPR_HANDLER_PUT | MPR_HANDLER_DELETE | MPR_HANDLER_TERMINAL)
{
	file = 0;
}

////////////////////////////////////////////////////////////////////////////////

MaPutHandler::~MaPutHandler()
{
	if (file) {
		delete file;
	}
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaPutHandler::cloneHandler()
{
	return new MaPutHandler();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Override default matchRequest
//

int MaPutHandler::matchRequest(MaRequest *rq, char *uri, int uriLen)
{
	if (rq->getMethodFlags() & (MPR_HANDLER_PUT | MPR_HANDLER_DELETE)) {
		return 1;
	} 
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaPutHandler::postData(MaRequest *rq, char *buf, int len)
{
	char	*msg;
	int		rc, code;

	mprAssert(file);

	if (len < 0 && rq->getRemainingContent() > 0) {

		mprLog(5, "%d: postData short post data, len %d, remaining %d\n",
			rq->getFd(), len, rq->getRemainingContent());
		file->close();
		delete file;
		file = 0;

	} else {
#if BLD_FEATURE_RANGES
		int start = rq->getInputStart();
		int end = rq->getInputEnd();
		if (!disableRange) {
			if (start >= 0 && end > 0) {
				file->lseek(start, 0);
				len = min(len, end - start);
			}
			if (len <= 0) {
				return;
			}
		}
#endif
		rc = file->write(buf, len);

		mprLog(5, "%d: postData of %d bytes, rc %d, errno %d\n", 
			len, rc, errno);

		if (rc != len) {
			rq->requestError(MPR_HTTP_BAD_GATEWAY, "Can't PUT to %s", rq->getUri());
			rq->finishRequest();
			return;
		}

		//
		//	We use files for CGI and so it delays calling run till all
		//	the post data has been received.
		//
		if (rq->getRemainingContent() <= 0) {
			file->close();
			code = rq->getResponseCode();
			mprAllocSprintf(&msg, MPR_HTTP_BUFSIZE, 
				"<HTML><HEAD><TITLE>%s</TITLE></HEAD>\r\n"
				"<BODY><H2>%s</H2>\r\n"
				"</BODY></HTML>\r\n",
				rq->getErrorMsg(code), rq->getErrorMsg(code));
			rq->formatAltResponse(code, msg, MPR_HTTP_DONT_ESCAPE);
			mprFree(msg);

			rq->setFlags(MPR_HTTP_INCOMPLETE, -1);
			rq->cancelOutput();
			rq->flushOutput(MPR_HTTP_FOREGROUND_FLUSH, MPR_HTTP_FINISH_REQUEST);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	If request is a PUT, then the run method is always called before postData.
//

int MaPutHandler::run(MaRequest *rq)
{
	MprFileInfo		*info;
	char			*uri, path[MPR_MAX_FNAME];
	int				flags;

	flags = rq->getFlags();
	info = rq->getFileInfo();
	uri = rq->getUri();
	hitCount++;

	if (flags & MPR_HTTP_PUT_REQUEST && rq->getRemainingContent() > 0) {
		//
		//	When all the post data is received the run method will be recalled
		//	by the postData method.
		//
		file = new MprFile();

		if (rq->host->mapToStorage(rq, path, sizeof(path), uri, 0) < 0) {
			rq->requestError(500, "Can't map the URI to put");
			rq->finishRequest();
			return 0;
		}

		if (rq->isRangedInput()) {
			if (file->open(path, O_BINARY | O_WRONLY, 0644) < 0) {
				if (info && !info->isReg) {
					if (file->open(path, O_CREAT | O_TRUNC | O_BINARY | O_WRONLY, 0644) < 0){
						rq->requestError(500, "Can't create the put URI");
						rq->finishRequest();
					} else {
						rq->setResponseCode(MPR_HTTP_CREATED);
					}
					disableRange = 1;

				} else {
					rq->requestError(500, "Can't PUT to open URI");
					rq->finishRequest();
				}

			} else {
				file->lseek(0, 0);
				rq->setResponseCode(MPR_HTTP_OK);
			}

		} else {
			if (file->open(path, O_CREAT | O_TRUNC | O_BINARY | O_WRONLY, 0644) < 0){
				rq->requestError(500, "Can't create the put URI");
				rq->finishRequest();

			} else {
				if (info && info->isReg) {
					rq->setResponseCode(MPR_HTTP_OK);
				} else {
					rq->setResponseCode(MPR_HTTP_CREATED);
				}
			}
		}
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}

	if (flags & MPR_HTTP_DELETE_REQUEST) {
		if (rq->host->mapToStorage(rq, path, sizeof(path), uri, 0) < 0) {
			rq->requestError(MPR_HTTP_NOT_FOUND, "URI not found in storage");
			rq->finishRequest();
			return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
		}
		if (info && !info->isReg) {
			rq->requestError(MPR_HTTP_NOT_FOUND, "URI not found");
			rq->finishRequest();

		} else {
			if (unlink(path) < 0) {
				rq->requestError(MPR_HTTP_NOT_FOUND, "Can't remove URI");
				rq->finishRequest();

			} else {
				/* Success with no response content */
				rq->setResponseCode(MPR_HTTP_NO_CONTENT);
			}
		}
	}

	rq->flushOutput(MPR_HTTP_FOREGROUND_FLUSH, MPR_HTTP_FINISH_REQUEST);
	return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
}

////////////////////////////////////////////////////////////////////////////////
#else
void mprPutHandlerDummy() {}

#endif // BLD_FEATURE_PUT_MODULE
//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
