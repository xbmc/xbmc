///
///	@file 	egiHandler.cpp
/// @brief 	Embedded Gateway Interface (EGI) handler. 
///	@overview The EGI handler implements a very fast in-process CGI scheme.
///
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
//////////////////////////////////// Includes //////////////////////////////////

#include	"egiHandler.h"

//////////////////////////////////// Locals ////////////////////////////////////
#if BLD_FEATURE_EGI_MODULE
//
//	Local to make it easier for EgiForm to access
//

static MaEgiHandlerService *egiHandlerService;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MaEgiModule /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int mprEgiInit(void *handle)
{
	if (maGetHttp() == 0) {
		return MPR_ERR_NOT_INITIALIZED;
	}
	(void) new MaEgiModule(handle);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MaEgiModule::MaEgiModule(void *handle) : MaModule("egi", handle)
{
	egiHandlerService = new MaEgiHandlerService();
}

////////////////////////////////////////////////////////////////////////////////

MaEgiModule::~MaEgiModule()
{
	delete egiHandlerService;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// MaEgiHandlerService /////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaEgiHandlerService::MaEgiHandlerService() : MaHandlerService("egiHandler")
{
	forms = new MprHashTable(31);
#if BLD_FEATURE_LOG
	log = new MprLogModule("egi");
#endif
#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaEgiHandlerService::~MaEgiHandlerService()
{
	lock();
	delete forms;
#if BLD_FEATURE_LOG
	delete log;
#endif

#if BLD_FEATURE_MULTITHREAD
	delete mutex;
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaEgiHandlerService::newHandler(MaServer *server, MaHost *host, 
	char *ext)
{
	MaEgiHandler	*ep;

	//
	//	Currently only a single forms hash table for all servers/hosts
	//
	ep = new MaEgiHandler(ext, log, forms);
	return ep;
}

////////////////////////////////////////////////////////////////////////////////

void MaEgiHandlerService::insertForm(MaServer *server, MaHost *host, 
	MaEgiForm *form)
{
	forms->insert(form);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MaEgiHandler ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaEgiHandler::MaEgiHandler(char *ext, MprLogModule *serviceLog, 
	MprHashTable *masterForms) : 
	MaHandler("egiHandler", ext, 
	MPR_HANDLER_GET | MPR_HANDLER_HEAD | MPR_HANDLER_POST | 
	MPR_HANDLER_MAP_VIRTUAL | MPR_HANDLER_EXTRA_PATH | MPR_HANDLER_NEED_ENV | 
	MPR_HANDLER_TERMINAL)
{
	log = serviceLog;

	//
	//	Cloned handlers (per-request) use the master forms table
	//
	if (masterForms == 0) {
		mprAssert(0);
		forms = new MprHashTable(31);
	} else {
		forms = masterForms;
	}

	egiFlags = 0;
	postBuf = 0;
}

////////////////////////////////////////////////////////////////////////////////

MaEgiHandler::~MaEgiHandler()
{
	delete postBuf;
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaEgiHandler::cloneHandler()
{
	MaEgiHandler	*ep;

	ep = new MaEgiHandler(extensions, log, forms);
	ep->flags |= MPR_EGI_CLONED;
	return ep;
}

////////////////////////////////////////////////////////////////////////////////

int MaEgiHandler::setup(MaRequest *rq)
{
	MaLimits	*limits;

	limits = rq->host->getLimits();
	mprAssert(postBuf == 0);
	postBuf = new MprBuf(MPR_HTTP_IN_BUFSIZE, limits->maxBody);
	mprLog(5, log, "%d: setup\n", rq->getFd());
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Read all post data and convert to variables. But only if urlencoded.
//

void MaEgiHandler::postData(MaRequest *rq, char *buf, int len)
{
	int		rc;

	mprLog(5, log, "%d: postData %d bytes\n", rq->getFd(), len);

	if (len < 0 && rq->getRemainingContent() > 0) {
		rq->finishRequest(MPR_HTTP_CLOSE);
		return;
	}

	rc = postBuf->put((uchar*) buf, len);
	postBuf->addNull();

	if (rc != len) {
		rq->requestError(MPR_HTTP_REQUEST_TOO_LARGE, "Too much post data");
		rq->finishRequest();

	} else {
		//
		//	If we have all the post data, convert it into vars and call the 
		//	run method.
		//
		if (rq->getRemainingContent() <= 0) {
			mprLog(4, log, "%d: Post Data: length %d\n< %s\n", rq->getFd(), 
				postBuf->getLength(), postBuf->getStart());

			if (egiFlags & MPR_EGI_URL_ENCODED) {
#if BLD_FEATURE_GACOMPAT_MODULE
				char	*query, *oldQuery;
				if (strncmp(rq->getUri(), "/goform/", 8) == 0) {
					oldQuery = rq->getVar(MA_REQUEST_OBJ, "QUERY_STRING", "");
					if (*oldQuery) {
						mprAllocStrcat(&query, MPR_HTTP_MAX_URL, 0, 
							oldQuery, "&", postBuf->getStart(), (void*) 0);
						rq->setVar(MA_REQUEST_OBJ, "QUERY_STRING", query);
						mprFree(query);
					} else {
						query = mprStrdup(postBuf->getStart());
						rq->setVar(MA_REQUEST_OBJ, "QUERY_STRING", 
							postBuf->getStart());
					}
				}
#endif
				rq->createQueryVars(postBuf->getStart(), postBuf->getLength());
			}
			run(rq);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

int MaEgiHandler::run(MaRequest *rq)
{
	MaEgiForm		*form;
	MaDataStream	*dynBuf;
	MaHeader		*header;
	char			*uri;
	int				flags;

	flags = rq->getFlags();
	if (flags & MPR_HTTP_POST_REQUEST && rq->getRemainingContent() > 0) {
		//
		//	When all the post data is received the run method will be recalled
		//	by the postData method.
		//
		header = rq->getHeader();
		if (mprStrCmpAnyCase(header->contentMimeType, 
				"application/x-www-form-urlencoded") == 0) {
			egiFlags |= MPR_EGI_URL_ENCODED;
		}
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}

	hitCount++;
	rq->setResponseCode(200);
	rq->setResponseMimeType("text/html");
	rq->setHeaderFlags(MPR_HTTP_DONT_CACHE, 0);
	rq->insertDataStream(rq->getDynBuf());

	uri = rq->getUri();
	mprLog(4, log, "%d: serving: %s\n", rq->getFd(), uri);

	form = (MaEgiForm*) forms->lookup(uri);
	if (form == 0) {
		rq->requestError(404, "EGI Form: \"%s\" is not defined", uri);
		rq->finishRequest();

	} else {
		form->run(rq, uri, rq->getOriginalUri(), rq->getQueryString(), 
			postBuf->getStart(), postBuf->getLength());
		if (rq->getState() == MPR_HTTP_RUNNING) {
			dynBuf = rq->getDynBuf();

			//
			//	This flag is only used by the GoAhead compatibility layer.
			//	Users needing this functionality should create their own 
			//	custom handler
			//
			if (rq->getFlags() & MPR_HTTP_DONT_FINISH) {
				rq->flushOutput(MPR_HTTP_BACKGROUND_FLUSH, 0);
			} else {
				rq->flushOutput(MPR_HTTP_BACKGROUND_FLUSH, 
					MPR_HTTP_FINISH_REQUEST);
			}
		}
	}
	return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
}

////////////////////////////////////////////////////////////////////////////////

void MaEgiHandler::insertForm(MaEgiForm *form)
{
	forms->insert(form);
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MaEgiForm //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaEgiForm::MaEgiForm(char *formName)
{
	name = mprStrdup(formName);
#if WIN
	mprStrLower(name);
#endif
	setKey(name);
	egiHandlerService->insertForm(0, 0, this);
}

////////////////////////////////////////////////////////////////////////////////

MaEgiForm::MaEgiForm(MaServer *server, MaHost *host, char *formName)
{
	name = mprStrdup(formName);
#if WIN
	mprStrLower(name);
#endif
	setKey(name);
	egiHandlerService->insertForm(server, host, this);
}

////////////////////////////////////////////////////////////////////////////////

MaEgiForm::~MaEgiForm()
{
	mprFree(name);
}

////////////////////////////////////////////////////////////////////////////////

char *MaEgiForm::getName()
{
	return name;
}

////////////////////////////////////////////////////////////////////////////////

void MaEgiForm::run(MaRequest *rq, char *script, char *path, char *query, 
	char *postData, int postLen)
{
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// EGI C API ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_EGI_MODULE
//
//	Almost DEPRECATED: in 2.0 but back by user demand.
//

//
//	Egi Form class for C forms
//
class CEgiForm : public MaEgiForm {
  private:
	MaEgiCb		egiCallback;
  public:
				CEgiForm(char *egiName, MaEgiCb fn);
				~CEgiForm();
	void		run(MaRequest *rq, char *script, char *path, char *query, 
						char *postData, int postLen);
};
#endif
////////////////////////////////////////////////////////////////////////////////

CEgiForm::CEgiForm(char *formName, MaEgiCb fn) : MaEgiForm(formName)
{
	egiCallback = fn;
}

////////////////////////////////////////////////////////////////////////////////

CEgiForm::~CEgiForm()
{
}

////////////////////////////////////////////////////////////////////////////////

void CEgiForm::run(MaRequest *rq, char *script, char *path, char *query, 
	char *postData, int postLen)
{
	(*egiCallback)(rq, script, path, query, postData, postLen);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Part of the old C-API. Has not been replaced
//

int maDefineEgiForm(char *name, MaEgiCb fn) 
{
	(void) new CEgiForm(name, fn);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#else
void mprEgiHandlerDummy() {}

#endif // BLD_FEATURE_EGI_MODULE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
