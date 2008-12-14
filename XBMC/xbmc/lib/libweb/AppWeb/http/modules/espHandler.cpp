///
///	@file 	espHandler.cpp
/// @brief 	Embedded Server Pages (ESP) handler.
/// @overview The ESP handler provides an efficient way to generate 
///		dynamic pages using server-side Javascript. The ESP module 
///		processes ESP pages and executes embedded scripts.
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
/////////////////////////////////// Includes ///////////////////////////////////

#include	"espHandler.h"

//////////////////////////////////// Locals ////////////////////////////////////
#if BLD_FEATURE_ESP_MODULE
//
//	Local to make it easier for EspProc to access
//

static MaEspHandlerService *espService;

//
//	Master ESP control block
//
static Esp		 			espControl;

#if BLD_FEATURE_MULTITHREAD
static void	espLock(void *lockData);
static void	espUnlock(void *lockData);
#endif

////////////////////////////// Forward Declarations ////////////////////////////

static void	createSession(EspHandle handle, int timeout);
static void	destroySession(EspHandle handle);
static char	*getSessionId(EspHandle handle);
static int	mapToStorage(EspHandle handle, char *path, int len, char *uri,
				int flags);
static int	readFile(EspHandle handle, char **buf, int *len, char *path);
static void	redirect(EspHandle handle, int code, char *url);
static void	setCookie(EspHandle handle, char *name, char *value, int lifetime, 
				char *path, bool secure);
static void	setHeader(EspHandle handle, char *value, bool allowMultiple);
static void	setResponseCode(EspHandle handle, int code);
static int	writeBlock(EspHandle handle, char *buf, int size);
static int	writeFmt(EspHandle handle, char *fmt, ...);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MaEspModule /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int mprEspInit(void *handle)
{
	if (maGetHttp() == 0) {
		return MPR_ERR_NOT_INITIALIZED;
	}
	(void) new MaEspModule(handle);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MaEspModule::MaEspModule(void *handle) : MaModule("esp", handle)
{
	espService = new MaEspHandlerService();
}

////////////////////////////////////////////////////////////////////////////////

MaEspModule::~MaEspModule()
{
	delete espService;
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////// MaEspHandlerService //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaEspHandlerService::MaEspHandlerService() : MaHandlerService("espHandler")
{
	serviceFlags = 0;

	espControl.createSession = createSession;
	espControl.destroySession = destroySession;
	espControl.getSessionId = getSessionId;
	espControl.maxScriptSize = maGetHttp()->getLimits()->maxScriptSize;
	espControl.mapToStorage = mapToStorage;
	espControl.readFile = readFile;
	espControl.redirect = redirect;
	espControl.setCookie = setCookie;
	espControl.setHeader = setHeader;
	espControl.setResponseCode = setResponseCode;
	espControl.writeBlock = writeBlock;
	espControl.writeFmt = writeFmt;

#if BLD_FEATURE_MULTITHREAD
	//
	//	This mutex is used very sparingly and must be an application global
	//	lock.
	//
	mutex = new MprMutex();
	espControl.lock = espLock;
	espControl.unlock = espUnlock;
	espControl.lockData = mutex;
#endif

	espOpen(&espControl);
}

////////////////////////////////////////////////////////////////////////////////

MaEspHandlerService::~MaEspHandlerService()
{
	lock();
	espClose();
#if BLD_FEATURE_MULTITHREAD
	delete mutex;
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MaEspHandlerService::setErrors(int where)
{
	lock();
	serviceFlags &= ~(ESP_FLAGS_ERRORS_TO_LOG | ESP_FLAGS_ERRORS_TO_BROWSER);
	if (where & ESP_FLAGS_ERRORS_TO_BROWSER) {
		serviceFlags |= ESP_FLAGS_ERRORS_TO_BROWSER;
	} else if (where & ESP_FLAGS_ERRORS_TO_LOG) {
		serviceFlags |= ESP_FLAGS_ERRORS_TO_LOG;
	}
	unlock(); 
}

////////////////////////////////////////////////////////////////////////////////

int MaEspHandlerService::start()
{
	//
	//	May have been updated in the config file parsing
	//
	espControl.maxScriptSize = maGetHttp()->getLimits()->maxScriptSize;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Used to create a master handler for each virtual host.
//

MaHandler *MaEspHandlerService::newHandler(MaServer *server, MaHost *host, 
	char *extensions)
{
	return new MaEspHandler(this, extensions);
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaEspHandler /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaEspHandler::MaEspHandler(MaEspHandlerService *hs, char *extensions) :
		MaHandler("espHandler", extensions, 
		MPR_HANDLER_GET | MPR_HANDLER_HEAD | MPR_HANDLER_POST | 
		MPR_HANDLER_NEED_ENV | MPR_HANDLER_OWN_GLOBAL | MPR_HANDLER_TERMINAL)
{
	espHandlerService = hs;
	postBuf = 0;
	espRequest = 0;
}

////////////////////////////////////////////////////////////////////////////////
 
MaEspHandler::~MaEspHandler()
{
	if (espRequest) {
		espDestroyRequest(espRequest);
	}
	delete postBuf;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Used per-request
//

MaHandler *MaEspHandler::cloneHandler()
{
	return new MaEspHandler(espHandlerService, extensions);
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_CONFIG_PARSE

int MaEspHandler::parseConfig(char *key, char *value, MaServer *server, 
	MaHost *host, MaAuth *auth, MaDir *dir, MaLocation *location)
{
	if (mprStrCmpAnyCase(key, "EspErrors") == 0) {

		if (mprStrCmpAnyCase(value, "log") == 0) {
			espService->setErrors(ESP_FLAGS_ERRORS_TO_LOG);

		} else if (mprStrCmpAnyCase(value, "browser") == 0) {
			espService->setErrors(ESP_FLAGS_ERRORS_TO_BROWSER);

		} else {
			mprError(MPR_L, MPR_LOG, "Bad value for EspErrors %s", value);
			return -1;
		}
		return 1;
	}
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Called after all the headers have been read but before any POST data.
//

int MaEspHandler::setup(MaRequest *rq)
{
	MaLimits	*limits;
	MaHost		*host;

	host = rq->host;
	limits = host->getLimits();
	mprAssert(postBuf == 0);
	postBuf = new MprBuf(MPR_HTTP_IN_BUFSIZE, limits->maxBody);
	mprLog(5, "esp: %d: setup\n", rq->getFd());

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Read all post data and convert to variables. But only if urlencoded.
//

void MaEspHandler::postData(MaRequest *rq, char *buf, int len)
{
	MaHeader	*header;
	int			rc;

	mprLog(5, "esp: %d: postData %d bytes\n", rq->getFd(), len);

	if (len < 0 && rq->getRemainingContent() > 0) {
		rq->requestError(400, "Incomplete post data");
		rq->finishRequest();
		return;
	}

	rc = postBuf->put((uchar*) buf, len);
	postBuf->addNull();

	if (rc != len) {
		rq->requestError(MPR_HTTP_REQUEST_TOO_LARGE, "Too much post data");
		rq->finishRequest();

	} else {
		//
		//	If we have all the post data, store it in form[] and 
		//	call the run method.
		//
		if (rq->getRemainingContent() <= 0) {
			mprLog(4, "esp: %d: Post Data: length %d\n< %s\n", rq->getFd(), 
				postBuf->getLength(), postBuf->getStart());
			header = rq->getHeader();
			if (mprStrCmpAnyCase(header->contentMimeType, 
					"application/x-www-form-urlencoded") == 0) {
				rq->createQueryVars(postBuf->getStart(), postBuf->getLength());
			} else {
				//
				//	FUTURE -- provide access to raw post data
				//
				// 		rq->requestError(MPR_HTTP_UNSUPPORTED_MEDIA_TYPE, 
				// 			"Post data is not urlencoded");
			}
			mprLog(4, "esp: Data: %s\n", postBuf->getStart());
			run(rq);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

MprBuf *MaEspHandler::getPostBuf()
{
	return postBuf;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Process the ESP page. The run method is called before the postData has
//	been received. If there is still post data to be read, this routine
//	returns and waits for postData to recall it once all the post data has 
//	been read.
//

int MaEspHandler::run(MaRequest *rq)
{
	MaDataStream	*dynBuf;
	MprFileInfo		*info;
	char			*fileName, *docBuf, *errMsg;
	int				size, requestFlags;

	requestFlags = rq->getFlags();

	if (requestFlags & MPR_HTTP_POST_REQUEST && rq->getRemainingContent() > 0) {
		//
		//	When all the post data is received the run method will be recalled
		//	by the postData method.
		//
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}

	hitCount++;
	dynBuf = rq->getDynBuf();

	rq->setResponseCode(200);
	rq->setResponseMimeType("text/html");
	rq->setHeaderFlags(MPR_HTTP_DONT_CACHE, 0);

#if BLD_FEATURE_SESSION
	//
	//	Auto create a session if required
	//
	if (rq->getSession() == 0 && rq->host->getSessionAutoCreate()) {
		rq->createSession(0);
	}
#endif

	//
 	//	We now have all the information we need to create the esp request.
	//	Pass in the "rq" handle to be used as the openHandle on all callbacks. 
	//	This call will create the global and local variables. It will also 
	//	define all the rq->variables[] as globals of the corresponding names. 
	//	If BLD_FEATURE_LEGACY_API is defined, it will also copy all properties 
	//	as global variables.
	//	
	espRequest = espCreateRequest((EspHandle) rq, rq->getUri(),
		rq->getVariables());
	if (espRequest == 0) {
		rq->requestError(404, "Can't create ESP request for %s", rq->getUri());
		rq->finishRequest();
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}

	fileName = rq->getFileName();
	mprAssert(fileName);

	if (rq->openDoc(fileName) < 0) {
		rq->requestError(404, "Can't open document: %s", fileName);
		rq->finishRequest();
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	} 
	mprLog(4, "%d: esp: serving: %s\n", rq->getFd(), fileName);

	if (requestFlags & (MPR_HTTP_GET_REQUEST | MPR_HTTP_HEAD_REQUEST | 
			MPR_HTTP_POST_REQUEST)) {
		rq->insertDataStream(dynBuf);

		info = rq->getFileInfo();
		size = info->size * sizeof(char);
		docBuf = (char*) mprMalloc(size + 1);
		docBuf[size] = '\0';

		if (rq->readDoc(docBuf, size) != size) {
			rq->requestError(404, "Can't read document");
			rq->finishRequest();
			mprFree(docBuf);
			return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
		}

#if FUTURE
#if BLD_FEATURE_SESSION
		if (rq->getSession()) {
			mprAddVarTrigger(sessionObj, sessionLock);
		}
#endif
		mprAddVarTrigger(appObj, appLock);

		session[], app[] access

		Lock order
			rq, session, app
			if (app access)
				always lock session first
		Issues
			- What if user wants to create another super global
#endif

		if (espProcessRequest(espRequest, fileName, docBuf, &errMsg) < 0) {
			if (espHandlerService->getFlags() & ESP_FLAGS_ERRORS_TO_BROWSER) {
				rq->writeFmt("<h2>ESP Error in \"%s\"</h2>\n", rq->getUri());
				rq->writeFmt("<p>In file: \"%s\"</p>\n", fileName);
				rq->writeFmt("<h3><pre>%s</pre></h3>\n", errMsg);
				rq->writeFmt("<p>To prevent errors being displayed in the "
					"browser, Put <b>\"EspErrors log\"</b> in the "
					"config file.</p>");
				mprFree(errMsg);
			} else {
				rq->requestError(404, 
					"Error processing ESP request %s\n: %s", fileName, 
					errMsg ? errMsg : "");
				rq->finishRequest();
				if (errMsg) {
					mprFree(errMsg);
				}
				mprFree(docBuf);
				return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
			}
		}
		mprFree(docBuf);
	}

	rq->flushOutput(MPR_HTTP_BACKGROUND_FLUSH, MPR_HTTP_FINISH_REQUEST);
	return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// ESP Callbacks /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static void createSession(EspHandle handle, int timeout)
{
#if BLD_FEATURE_SESSION
	MaRequest	*rq = (MaRequest*) handle;

	rq->createSession(timeout);
#endif
}

////////////////////////////////////////////////////////////////////////////////

static void	destroySession(EspHandle handle)
{
#if BLD_FEATURE_SESSION
	MaRequest	*rq = (MaRequest*) handle;

	rq->destroySession();
#endif
}

////////////////////////////////////////////////////////////////////////////////

static char *getSessionId(EspHandle handle)
{
#if BLD_FEATURE_SESSION
	MaRequest	*rq = (MaRequest*) handle;

	return rq->getSessionId();
#else
	return 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Used to process include files only. Not for URIs that come over the web.
//

static int mapToStorage(EspHandle handle, char *path, int len, char *uri,
	int flags)
{
	MaRequest	*rq = (MaRequest*) handle;

	return rq->host->mapToStorage(rq, path, len, uri, flags);
}

////////////////////////////////////////////////////////////////////////////////

static int readFile(EspHandle handle, char **buf, int *len, char *path)
{
	MaRequest		*rq;
	MprFileSystem	*fs;
	MprFileInfo		info;
	MprFile			*file;
	char			*docBuf;
	int				size;

	mprAssert(path && *path);
	mprAssert(buf);

	rq = (MaRequest*) handle;
	fs = rq->host->server->fileSystem;

	if (fs->stat(path, &info) < 0) {
		return MPR_ERR_CANT_ACCESS;
	}
	file = fs->newFile();
	if (file == 0 || file->open(path, O_RDONLY | O_BINARY, 0666) < 0) {
		delete file;
		return MPR_ERR_CANT_OPEN;
	}

	size = info.size * sizeof(char);
	docBuf = (char*) mprMalloc(size + 1);
	docBuf[size] = '\0';

	if (file->read(docBuf, size) != size) {
		mprFree(docBuf);
		file->close();
		delete file;
		return MPR_ERR_CANT_READ;
	}
	file->close();
	delete file;

	*buf = docBuf;
	if (len) {
		*len = size;
	}
	return size;
}

////////////////////////////////////////////////////////////////////////////////

static void	redirect(EspHandle handle, int code, char *url)
{
	MaRequest	*rq = (MaRequest*) handle;

	rq->redirect(code, url);
}

////////////////////////////////////////////////////////////////////////////////

static void setCookie(EspHandle handle, char *name, char *value, int lifetime, 
	char *path, bool secure)
{
#if BLD_FEATURE_COOKIE
	MaRequest	*rq = (MaRequest*) handle;

	rq->setCookie(name, value, lifetime, path, secure);
#endif
}

////////////////////////////////////////////////////////////////////////////////

static void setHeader(EspHandle handle, char *value, bool allowMultiple)
{
	MaRequest	*rq = (MaRequest*) handle;

	rq->setHeader(value, allowMultiple);
}

////////////////////////////////////////////////////////////////////////////////

static void setResponseCode(EspHandle handle, int code)
{
	MaRequest	*rq = (MaRequest*) handle;

	rq->setResponseCode(code);
}

////////////////////////////////////////////////////////////////////////////////

static int writeBlock(EspHandle handle, char *buf, int size)
{
	MaRequest	*rq = (MaRequest*) handle;

	return rq->write(buf, size);
}

////////////////////////////////////////////////////////////////////////////////

static int writeFmt(EspHandle handle, char *fmt, ...)
{
	MaRequest	*rq = (MaRequest*) handle;
	va_list		args;
	char		*buf;
	int			len, rc;

	va_start(args, fmt);
	len = mprAllocVsprintf(&buf, MPR_MAX_HEAP_SIZE, fmt, args);
	rc = rq->write(buf, len);
	mprFree(buf);
	va_end(args);
	return rc;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MULTITHREAD

static void espLock(void *lockData)
{
	MprMutex	*mutex = (MprMutex*) lockData;

	mprAssert(mutex);

	mutex->lock();
}

////////////////////////////////////////////////////////////////////////////////

static void espUnlock(void *lockData)
{
	MprMutex	*mutex = (MprMutex*) lockData;

	mprAssert(mutex);

	mutex->unlock();
}

#endif // BLD_FEATURE_MULTITHREAD
////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MaEspProc //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_LEGACY_API
//
//	Callback wrapper for EspProcs
//

static int espProcWrapper(EspRequest *ep, int argc, char **argv)
{
	MaEspProc	*proc;
	MaRequest	*rq;

	proc = (MaEspProc*) espGetThisPtr(ep);
	rq = (MaRequest*) espGetRequestHandle(ep);

	rq->setScriptHandle((void*) ep);
	return proc->run(rq, argc, argv);
}

////////////////////////////////////////////////////////////////////////////////

MaEspProc::MaEspProc(char *name, char *scriptName)
{
	this->name = mprStrdup(name);
	espDefineStringCFunction(0, name, (EspStringCFunction) espProcWrapper, 
		(void*) this);
}

////////////////////////////////////////////////////////////////////////////////

MaEspProc::MaEspProc(MaServer *server, MaHost *host, char *name)
{
	this->name = mprStrdup(name);
}

////////////////////////////////////////////////////////////////////////////////

MaEspProc::~MaEspProc()
{
	mprFree(name);
}

////////////////////////////////////////////////////////////////////////////////

char *MaEspProc::getName()
{
	return name;
}

////////////////////////////////////////////////////////////////////////////////

void MaEspProc::setErrorMsg(MaRequest *rq, char *fmt, ...)
{
	EspRequest	*ep;
	va_list		args;
	char		*buf, *escapeBuf;
	int			len;

	va_start(args, fmt);
	mprAllocVsprintf(&buf, MPR_MAX_HEAP_SIZE, fmt, args);

	//
	//	Allow plenty of room for escaping HTML characters
	//
	len = strlen(buf) * 3;
	escapeBuf = (char*) mprMalloc(len);
	maEscapeHtml(escapeBuf, len, buf);

	ep = (EspRequest*) rq->getScriptHandle();
	espError(ep, escapeBuf);

	mprFree(escapeBuf);
	mprFree(buf);
	va_end(args);
}

////////////////////////////////////////////////////////////////////////////////

void MaEspProc::setReturnValue(MaRequest *rq, MprVar result)
{
	EspRequest	*ep;

	ep = (EspRequest*) rq->getScriptHandle();
	espSetReturn(ep, result);
}

////////////////////////////////////////////////////////////////////////////////

void maEspSetResult(MaRequest *rq, char *str)
{
	EspRequest	*ep;

	ep = (EspRequest*) rq->getScriptHandle();

	espSetReturnString(ep, str);
}

#endif // BLD_FEATURE_LEGACY_API

////////////////////////////////////////////////////////////////////////////////

#else
void mprEspHandlerDummy() {}

#endif // BLD_FEATURE_ESP_MODULE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
