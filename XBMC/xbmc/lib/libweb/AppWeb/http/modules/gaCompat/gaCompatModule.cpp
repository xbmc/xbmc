///
///	@file 	gaCompatModule.cpp
/// @brief 	Compatibility module for the GoAhead Web Server
///	@overview Provide a reasonable measure of compatibility with the 
///		GoAhead WebServer APIs.
///	@remarks This module only supports single threaded operation without 
///	any support for virtual hosts.
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

#include	"gaCompatModule.h"

//////////////////////////////////// Locals ////////////////////////////////////
#if BLD_FEATURE_GACOMPAT_MODULE

#define MPR_HTTP_MAX_GO_FORM		100

//
//	hAlloc list defines.
//
//	The handle list stores the length of the list and the number of used
//	handles in the first two words.  These are hidden from the caller by
//	returning a pointer to the third word to the caller
//

#define H_LEN		0		// First entry holds length of list
#define H_USED		1		// Second entry holds number of used
#define H_OFFSET	2		// Offset to real start of list 
#define H_INCR		16		// Grow handle list in chunks this size


class SymHashEntry : public MprHashEntry {
  public:
	sym_t			sym;
					SymHashEntry(char *key, value_t *vp);
	virtual			~SymHashEntry();
	value_t			*getValue() { return &sym.content; };
};


class WebsForm : public MaEgiForm {
  private:
	WebsFormCb		goFormCallback;

  public:
					WebsForm(char *formName, WebsFormCb fn);
					~WebsForm();
	void			run(MaRequest *rq, char *script, char *path, 
						char *query, char *postData, int postLen);
};

//
//	Single threaded
//
static MprFile 		*file;

static MaServer		*defaultServer;
static MaHost		*defaultHost;
static WebsForm		*websForms[MPR_HTTP_MAX_GO_FORM];
static int 			maxForm;

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// CompatModule /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int mprCompatInit(void *handle)
{
	if (maGetHttp() == 0) {
		return MPR_ERR_NOT_INITIALIZED;
	}
	new MaCompatModule(handle);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MaCompatModule::MaCompatModule(void *handle) : MaModule("gaCompat", handle)
{
	file = new MprFile();
}

////////////////////////////////////////////////////////////////////////////////

MaCompatModule::~MaCompatModule()
{
	//
	//	No need to delete websForms[]. EgiHandler will delete these for us
	//
	delete file;
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// WebsForm ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

WebsForm::WebsForm(char *formName, WebsFormCb fn) : MaEgiForm(formName)
{
	goFormCallback = fn;
}

////////////////////////////////////////////////////////////////////////////////

WebsForm::~WebsForm()
{
}

////////////////////////////////////////////////////////////////////////////////

void WebsForm::run(MaRequest *rq, char *script, char *uri, 
	char *query, char *postData, int postLen)
{
	int		code;

	//
	//	GoAhead GoForms write their own headers
	//
	rq->setHeaderFlags(MPR_HTTP_HEADER_WRITTEN, 0);
#if BLD_FEATURE_KEEP_ALIVE
	rq->setNoKeepAlive();
#endif

	(*goFormCallback)((webs_t) rq, (char*) uri, (char*) query);

	//
	//	User must call websDone or websRedirect in the goform to complete
	//	the request. However, websRedirect does not close the request in
	//	Appweb. If websRedirect has not been called, then we set a flag
	//	to cause EGI to keep the request open.
	//
	code = rq->getResponseCode();
	if (300 < code && code <= 399) {
		//
		//	We also need to output headers if redirect is called.
		//

		rq->setHeaderFlags(0, MPR_HTTP_HEADER_WRITTEN);

	} else {
		//
		//	This will stop EGI from closing the socket
		//
		rq->setFlags(MPR_HTTP_DONT_FINISH, -1);
	}
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// C APIs ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
extern "C" {

char *strlower(char *string)
{
	char	*s;

	a_assert(string);

	if (string == NULL) {
		return NULL;
	}

	s = string;
	while (*s) {
		if (isupper(*s)) {
			*s = (char) tolower(*s);
		}
		s++;
	}
	*s = '\0';
	return string;
}

////////////////////////////////////////////////////////////////////////////////

char *strupper(char *string)
{
	char	*s;

	a_assert(string);
	if (string == NULL) {
		return NULL;
	}

	s = string;
	while (*s) {
		if (islower(*s)) {
			*s = (char) toupper(*s);
		}
		s++;
	}
	*s = '\0';
	return string;
}

////////////////////////////////////////////////////////////////////////////////

value_t valueInteger(long value)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = integer;
	v.value.integer = value;
	return v;
}

////////////////////////////////////////////////////////////////////////////////

value_t valueString(char* value, int flags)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = string;
	if (flags & VALUE_ALLOCATE) {
		v.allocated = 1;
		v.value.string = bstrdup(B_L, value);
	} else {
		v.allocated = 0;
		v.value.string = value;
	}
	return v;
}

////////////////////////////////////////////////////////////////////////////////

int emfSchedCallback(int delay, emfSchedProc *proc, void *arg)
{
	new MprTimer(delay, (MprTimerProc) proc, (void*) arg, 
		MPR_TIMER_AUTO_RESCHED);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void emfUnschedCallback(int id)
{
	MprTimer	*timer;

	timer = (MprTimer*) id;
	timer->stop(1000);
	timer->dispose();
}

////////////////////////////////////////////////////////////////////////////////

void emfReschedCallback(int id, int delay)
{
	MprTimer	*timer;

	timer = (MprTimer*) id;
	timer->reschedule(delay);
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// SymHashEntry /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

SymHashEntry::SymHashEntry(char *key, value_t *vp) : MprHashEntry(key)
{
	sym.forw = 0;
	sym.name.value.string = mprStrdup(key);
	sym.name.type = string;
	sym.arg = 0;

	sym.content = *vp;
	if (vp->allocated) {
		sym.content.value.string = mprStrdup(vp->value.string);
	}
}

////////////////////////////////////////////////////////////////////////////////

SymHashEntry::~SymHashEntry()
{
	if (sym.content.allocated) {
		mprFree(sym.content.value.string);
	}
	mprFree(sym.name.value.string);
}

////////////////////////////////////////////////////////////////////////////////

sym_fd_t symOpen(int tableSize)
{
	return (sym_fd_t) new MprHashTable();
}

////////////////////////////////////////////////////////////////////////////////

void symClose(sym_fd_t sd)
{
	MprHashTable	*table = (MprHashTable*) sd;

	delete table;
}

////////////////////////////////////////////////////////////////////////////////

sym_t *symLookup(sym_fd_t sd, char *name)
{
	MprHashTable	*table = (MprHashTable*) sd;
	SymHashEntry	*sp;

	sp = (SymHashEntry*) table->lookup(name);
	if (sp == 0) {
		return 0;
	}
	return &sp->sym;
}

////////////////////////////////////////////////////////////////////////////////

sym_t *symEnter(sym_fd_t sd, char *name, value_t v, int arg)
{
	MprHashTable	*table = (MprHashTable*) sd;
	SymHashEntry	*sp;

	sp = new SymHashEntry(name, &v);
	table->insert(sp);
	return &sp->sym;
}

////////////////////////////////////////////////////////////////////////////////

int symDelete(sym_fd_t sd, char *name)
{
	MprHashTable	*table = (MprHashTable*) sd;
	SymHashEntry	*sp;

	sp = (SymHashEntry*) table->lookup(name);
	if (sp) {
		table->remove(sp);
		delete sp;
		return 0;
	} else {
		return -1;
	}
}

////////////////////////////////////////////////////////////////////////////////

sym_t *symFirstEx(sym_fd_t sd, void **current)
{
	MprHashTable	*table = (MprHashTable*) sd;
	SymHashEntry	*sp;

	sp = (SymHashEntry*) table->getFirst();
	if (sp == 0) {
		return 0;
	}
	* ((SymHashEntry**) current) = sp;
	return &sp->sym;
}

////////////////////////////////////////////////////////////////////////////////

sym_t *symNextEx(sym_fd_t sd, void **current)
{
	MprHashTable	*table = (MprHashTable*) sd;
	SymHashEntry	*sp;

	sp = (SymHashEntry*) table->getNext((SymHashEntry*) *current);
	if (sp == 0) {
		return 0;
	}
	* ((SymHashEntry**) current) = sp;
	return &sp->sym;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////  User Management //////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	At the momemnt we are using the GoAhead WebServer UM module. This code
//	replaces that with a wrapper over the Appweb security model.
//
#if UNUSED

int umOpen() 
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void umClose() 
{
}

////////////////////////////////////////////////////////////////////////////////

int umRestore(char *filename)
{
	//	FUTURE -- can now implement
#if FUTURE
	auth->resetUserGroup();
	auth->readGroupFile(server, auth, filename);
	auth->readUserFile(server, auth, filename);
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int umCommit(char *filename)
{
#if FUTURE
	auth->save(filename);
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int umAddGroup(char *group, short privilege, accessMeth_t am, 
	bool_t protect, bool_t disabled)
{
#if FUTURE
	//
	//	protected == delete protected. This should be done in the UI anyway.
	//	accessMethod == noAuth, basic, digest
	//	disabled == enable the user
	//
	auth->createGroup(group);
	
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int umAddUser(char *user, char *password, char *group, bool_t protect, 
	bool_t disabled)
{
#if FUTURE
	//	not supporting disabled, protect
	auth->addUserPassword(user, password);
	auth->addUsersToGroup(group, user);
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int umDeleteGroup(char *group)
{
#if FUTURE
	auth->removeGroup(group);
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int umDeleteUser(char *user)
{
#if FUTURE
	auth->removeUser(user);
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

char *umGetFirstGroup()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

char *umGetNextGroup(char *lastUser)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

char *umGetFirstUser()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

char *umGetNextUser(char *lastUser)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

accessMeth_t umGetGroupAccessMethod(char *group)
{
	return AM_NONE;
}

////////////////////////////////////////////////////////////////////////////////

bool_t umGetGroupEnabled(char *group)
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

short umGetGroupPrivilege(char *group)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool_t umGetUserEnabled(char *user)
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

char *umGetUserGroup(char *user)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

char *umGetUserPassword(char *user)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool_t umGroupExists(char *group)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int umSetGroupAccessMethod(char *group, accessMeth_t am)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int umSetGroupEnabled(char *group, bool_t enabled)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int umSetGroupPrivilege(char *group, short privileges)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int umSetUserEnabled(char *user, bool_t enabled)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int umSetUserGroup(char *user, char *password)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int umSetUserPassword(char *user, char *password)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool_t umUserExists(char *user)
{
	return 0;
}

#endif // UNUSED 

////////////////////////////////////////////////////////////////////////////////

int websAspDefine(char *name, WebsAspCb fn)
{
	ejsDefineStringCFunction(-1, name, (MprStringCFunction) fn, 0, 
		MPR_VAR_ALT_HANDLE);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void websDecodeUrl(char *decoded, char *token, int len)
{
	maUrlDecode(decoded, len, token, 1, 1);
}

////////////////////////////////////////////////////////////////////////////////

void websDone(webs_t wp, int code)
{
	bool	closeSocket;

	closeSocket = 1;
	((MaRequest*) wp)->finishRequest(code, closeSocket);
}

////////////////////////////////////////////////////////////////////////////////

void websError(webs_t wp, int code, char *msg, ...)
{
	va_list		ap;
	char		buf[MPR_MAX_STRING];

	va_start(ap, msg);
	mprVsprintf(buf, sizeof(buf), msg, ap);
	((MaRequest*) wp)->requestError(code, buf);
	va_end(ap);
}

////////////////////////////////////////////////////////////////////////////////

char *websErrorMsg(int code)
{
	return (char*) maGetHttpErrorMsg(code);
}

////////////////////////////////////////////////////////////////////////////////

void websFooter(webs_t wp)
{
	((MaRequest*) wp)->write("</html>\n");
}

////////////////////////////////////////////////////////////////////////////////

int websFormDefine(char *name, WebsFormCb fn)
{
	char	nameBuf[MPR_HTTP_MAX_URL];

	if (maxForm >= MPR_HTTP_MAX_GO_FORM) {
		mprError(MPR_L, MPR_LOG, "Too many goForms");
		return -1;
	}
	mprSprintf(nameBuf, sizeof(nameBuf), "/goform/%s", name);
	websForms[maxForm++] = new WebsForm(nameBuf, fn);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

char *websGetDateString(websStatType *sbuf)
{
	if (sbuf) {
		MprFileInfo		info;
		info.mtime = sbuf->mtime;
		return (char*) maGetDateString(&info);
	} else {
		return (char*) maGetDateString(0);
	}
}

////////////////////////////////////////////////////////////////////////////////

char *websGetRequestLpath(webs_t wp)
{
	return (char*) ((MaRequest*) wp)->getFileName();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Returns the decoded Uri path portion without query
//
char *websGetPath(webs_t wp)
{
	return (char*) ((MaRequest*) wp)->getUri();
}

////////////////////////////////////////////////////////////////////////////////

char *websGetUserName(webs_t wp)
{
	return (char*) ((MaRequest*) wp)->getUser();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Returns a raw url un-decoded with query
//

char *websGetUrl(webs_t wp)
{
	return (char*) ((MaRequest*) wp)->getOriginalUri();
}

////////////////////////////////////////////////////////////////////////////////

char *websGetVar(webs_t wp, char *var, char *def)
{
	MaRequest	*rq;
	char		*value;

	rq = (MaRequest*) wp;

	if ((value = rq->getVar(MA_FORM_OBJ, var, 0))  != 0) {
		return value;
	}
	if ((value = rq->getVar(MA_HEADERS_OBJ, var, 0))  != 0) {
		return value;
	}
	if ((value = rq->getVar(MA_REQUEST_OBJ, var, 0))  != 0) {
		return value;
	}
	if ((value = rq->getVar(MA_SERVER_OBJ, var, 0))  != 0) {
		return value;
	}
	if ((value = rq->getVar(MA_GLOBAL_OBJ, var, 0))  != 0) {
		return value;
	}
	return def;
}

////////////////////////////////////////////////////////////////////////////////

void websSetVar(webs_t wp, char *var, char *value)
{
	MaRequest	*rq;

	rq = (MaRequest*) wp;
	rq->setVar(MA_GLOBAL_OBJ, var, value);
}

////////////////////////////////////////////////////////////////////////////////

char_t *websGetRequestIpaddr(webs_t wp)
{
	MaRequest	*rq;

	rq = (MaRequest*) wp;
	return rq->getRemoteIpAddr();
}

////////////////////////////////////////////////////////////////////////////////

void websHeader(webs_t wp)
{
	MaRequest	*rq = (MaRequest*) wp;

	rq->write("HTTP/1.0 200 OK\r\n");
	rq->writeFmt("Server: %s\r\n", MPR_HTTP_SERVER_NAME);
	rq->write("Pragma: no-cache\r\n");
	rq->write("Cache-control: no-cache\r\n");
	rq->write("Content-Type: text/html\r\n");
	rq->write("\r\n");
	rq->write("<html>\r\n");
}

////////////////////////////////////////////////////////////////////////////////

int websPageOpen(webs_t wp, char *fileName, char *uri, int mode, int perm)
{
	return file->open(uri, mode, perm);
}

////////////////////////////////////////////////////////////////////////////////

int websPageClose(webs_t wp)
{
	file->close();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int websPageStat(webs_t wp, char *fileName, char *uri, websStatType* sbuf)
{
	MaRequest		*rq;
	MprFileInfo		info;
 
	rq = (MaRequest*) wp;
	if (rq->host->server->fileSystem->stat(uri, &info) < 0) {
		return MPR_ERR_CANT_ACCESS;
	}
	sbuf->size = info.size;
	sbuf->isDir = info.isDir;
	sbuf->mtime = (time_t) info.mtime;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void websRedirect(webs_t wp, char *url)
{
	((MaRequest*) wp)->redirect(301, url);
	// ((MaRequest*) wp)->flushOutput(MPR_HTTP_FOREGROUND_FLUSH, 0);
}

////////////////////////////////////////////////////////////////////////////////

void websSetRealm(char *realmName)
{
	MaDir		*dir;
	MaAuth		*auth;

	dir = defaultHost->findBestDir(defaultServer->getServerRoot());

	if (dir == 0) {
		mprError(MPR_L, MPR_LOG,
			"websSetRealm Error: Server not yet configured");
		return;
	}

	auth = dir->getAuth();
	mprAssert(auth);
	if (auth == 0) {
		mprError(MPR_L, MPR_LOG,
			"webSetRealm Error: Server not yet configured");
		return;
	}

	auth->setRealm(realmName);
}

////////////////////////////////////////////////////////////////////////////////

void websSetRequestLpath(webs_t wp, char *fileName)
{
	((MaRequest*) wp)->setFileName(fileName);
}

////////////////////////////////////////////////////////////////////////////////

int websUrlHandlerDefine(char *urlPrefix, char *webDir, int arg, 
	int (*fn)(webs_t wp, char *urlPrefix, char *webDir, int arg, 
	char *url, char *path, char *query), int flags)
{
	mprError(MPR_L, MPR_LOG, "websUrlHandlerDefine is not supported");
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int websValid(webs_t wp)
{
	//
	//	Always return valid. Hope this is sufficient
	//
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

int websValidateUrl(webs_t wp, char *path)
{
	if (maValidateUri(path) == 0) {
		return -1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int websWrite(webs_t wp, char* fmt, ...)
{
	va_list		ap;
	char		buf[MPR_MAX_STRING];

	va_start(ap, fmt);
	mprVsprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	return ((MaRequest*) wp)->write(buf);
}

////////////////////////////////////////////////////////////////////////////////

int websWriteBlock(webs_t wp, char *buf, int nChars)
{
	return ((MaRequest*) wp)->write(buf, nChars);
}

////////////////////////////////////////////////////////////////////////////////
#if UNUSED_AND_INCOMPLETE

int websAspRequest(webs_t wp, char *localPath)
{
	MaRequest	*rq = (MaRequest*) wp;
	MprFileInfo	*info;
	char		*fileName, *errMsg;
	char		*docBuf;
	int			size;

	info = maGetRequestDocumentInfo(rq);

	size = info->size * sizeof(char);
	docBuf = (char*) mprMalloc(size + 1);
	docBuf[size] = '\0';

	if (maReadDoc(rq, docBuf, size) != size) {
		rq->requestError(404, "Can't read error document");
		mprFree(docBuf);
		return -1;
	}

	fileName = maGetRequestFileName();

	if (espProcessRequest(espRequest, fileName, docBuf, &errMsg) < 0) {
		if (espHandlerService->getFlags() & ESP_FLAGS_ERRORS_TO_BROWSER) {
			rq->writeFmt("<h2>ESP Error in \"%s\"</h2>\n", rq->getUriPath());
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
			if (errMsg) {
				mprFree(errMsg);
			}
			mprFree(docBuf);
			return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
		}
	}
}

#endif // UNUSED
////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Socket APIs ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static void socketAcceptWrapper(void *data, MprSocket *sock, char *ip, 
	int port, MprSocket *listenSock, int isMprPoolThread)
{
	socketAccept_t	callback;

	mprAssert(sock);

	//
	//	We stored the user's accept callback function in the socket accept data.
	//
	sock->getAcceptCallback(0, (void**) &callback);

	(callback)((int) sock, ip, port, (int) listenSock);
}

////////////////////////////////////////////////////////////////////////////////

int socketOpenConnection(char *host, int port, socketAccept_t accept, int flags)
{
	MprSocket	*sock;
	int			rc, socketFlags;

	socketFlags = 0;
	if (flags & SOCKET_BROADCAST) {
		socketFlags |= MPR_SOCKET_BROADCAST;
	} else if (flags & SOCKET_DATAGRAM) {
		socketFlags |= MPR_SOCKET_DATAGRAM;
	} else if (flags & SOCKET_BLOCK) {
		socketFlags |= MPR_SOCKET_BLOCK;
	}

	sock = new MprSocket();
	mprAssert(sock);

	if (host == 0) {
		//
		//	We store the user's accept function in the socket accept data.
		//
		rc = sock->openServer(host, port, socketAcceptWrapper, (void*) accept, 
			socketFlags);
	} else {
		rc = sock->openClient(host, port, flags);
	}
	if (rc < 0) {
		return -1;
	}
	return (int) sock;
}

////////////////////////////////////////////////////////////////////////////////

void socketCloseConnection(int sid)
{
	MprSocket	*sock;

	sock = (MprSocket*) sid;
	mprAssert(sock);

	sock->close(MPR_SHUTDOWN_BOTH);
	sock->dispose();
}

////////////////////////////////////////////////////////////////////////////////

static void socketIoWrapper(void *data, MprSocket *sock, int socketMask, int
	isMprPoolThread)
{
	socketHandler_t		handler;
	int					mask, arg;

	mprAssert(sock);

	sock->getCallback(0, (void**) &handler, (void**) &arg, &mask);
	(handler)((int) sock, mask, (int) data);
}

////////////////////////////////////////////////////////////////////////////////

void socketCreateHandler(int sid, int mask, socketHandler_t handler, int arg)
{
	MprSocket		*sock;
	int			socketMask;

	sock = (MprSocket*) sid;
	mprAssert(sock);

	socketMask = 0;
	if (mask & SOCKET_READABLE) {
		socketMask |= MPR_SOCKET_READABLE;
	}
	if (mask & SOCKET_WRITABLE) {
		socketMask |= MPR_SOCKET_WRITABLE;
	}
	if (mask & SOCKET_EXCEPTION) {
		socketMask |= MPR_SOCKET_WRITABLE;
	}
	sock->setCallback(socketIoWrapper, (void*) handler, (void*) arg, 
		socketMask);
}

////////////////////////////////////////////////////////////////////////////////

void socketDeleteHandler(int sid)
{
	MprSocket		*sock;

	sock = (MprSocket*) sid;
	mprAssert(sock);

	sock->setCallback(0, 0, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////

int socketEof(int sid)
{
	MprSocket		*sock;

	sock = (MprSocket*) sid;
	mprAssert(sock);

	return sock->getEof() ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////

int socketFlush(int sid)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int socketGetHandle(int sid)
{
	MprSocket		*sock;

	sock = (MprSocket*) sid;
	mprAssert(sock);

	return sock->getFd();
}

////////////////////////////////////////////////////////////////////////////////

int socketRead(int sid, char *buf, int len)
{
	MprSocket		*sock;

	mprAssert(buf);
	mprAssert(len > 0);

	sock = (MprSocket*) sid;
	mprAssert(sock);

	return sock->read(buf, len);
}

////////////////////////////////////////////////////////////////////////////////

int socketWrite(int sid, char *buf, int len)
{
	MprSocket		*sock;

	mprAssert(buf);
	mprAssert(len > 0);

	sock = (MprSocket*) sid;
	mprAssert(sock);

	return sock->write(buf, len);
}

////////////////////////////////////////////////////////////////////////////////

int socketWriteString(int sid, char *buf)
{
	int		len;

	len = strlen(buf);
	return socketWrite(sid, buf, len);
}

////////////////////////////////////////////////////////////////////////////////

int socketSetBlock(int sid, int flags)
{
	MprSocket		*sock;
	int				old;

	sock = (MprSocket*) sid;
	mprAssert(sock);

	old = sock->getBlockingMode() ? 1 : 0;
	sock->setBlockingMode(flags != 0 ? true : false);
	return old;
}

////////////////////////////////////////////////////////////////////////////////

int socketGetBlock(int sid)
{
	MprSocket		*sock;

	sock = (MprSocket*) sid;
	mprAssert(sock);

	return sock->getBlockingMode() ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Misc ///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Trace formats normally do not have new lines. 
//
void trace(int level, char *fmt, ...) 
{
	char	buf[MPR_MAX_LOG_STRING];
	char	*args;
	int		len;

	va_start(args, fmt);

	//
	//	mprVsprintf always ensures there is a null terminator. Subtract one
	//	for a new line
	//
	len = mprVsprintf(buf, sizeof(buf) - 1, fmt, args);
	buf[len++] = '\n';
	buf[len] = '\0';

	mprLog(level, buf);
	va_end(args);
}

////////////////////////////////////////////////////////////////////////////////

void traceRaw(char *buf) 
{
	mprLog(0, buf);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Allocate a new file handle.  On the first call, the caller must set the
//	handle map to be a pointer to a null pointer.  *map points to the second
//	element in the handle array.
//

int hAlloc(void ***map)
{
	int		*mp;
	int		handle, len, memsize, incr;

	a_assert(map);

	if (*map == NULL) {
		incr = H_INCR;
		memsize = (incr + H_OFFSET) * sizeof(void**);
		if ((mp = (int*) balloc(B_L, memsize)) == NULL) {
			return -1;
		}
		memset(mp, 0, memsize);
		mp[H_LEN] = incr;
		mp[H_USED] = 0;
		*map = (void**) &mp[H_OFFSET];

	} else {
		mp = &((*(int**)map)[-H_OFFSET]);
	}

	len = mp[H_LEN];

	//
	//	Find the first null handle
	//
	if (mp[H_USED] < mp[H_LEN]) {
		for (handle = 0; handle < len; handle++) {
			if (mp[handle+H_OFFSET] == 0) {
				mp[H_USED]++;
				return handle;
			}
		}
	} else {
		handle = len;
	}

	//
	//	No free handle so grow the handle list. Grow list in chunks of H_INCR.
	//
	len += H_INCR;
	memsize = (len + H_OFFSET) * sizeof(void**);
	if ((mp = (int*) brealloc(B_L, (void*) mp, memsize)) == NULL) {
		return -1;
	}
	*map = (void**) &mp[H_OFFSET];
	mp[H_LEN] = len;
	memset(&mp[H_OFFSET + len - H_INCR], 0, sizeof(int*) * H_INCR);
	mp[H_USED]++;
	return handle;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Free a handle.  This function returns the value of the largest
//	handle in use plus 1, to be saved as a max value.
//

int hFree(void ***map, int handle)
{
	int		*mp;
	int		len;

	a_assert(map);
	mp = &((*(int**)map)[-H_OFFSET]);
	a_assert(mp[H_LEN] >= H_INCR);

	a_assert(mp[handle + H_OFFSET]);
	a_assert(mp[H_USED]);
	mp[handle + H_OFFSET] = 0;
	if (--(mp[H_USED]) == 0) {
		bfree(B_L, (void*) mp);
		*map = NULL;
	}

	//
	//	Find the greatest handle number in use.
	//
	if (*map == NULL) {
		handle = -1;
	} else {
		len = mp[H_LEN];
		if (mp[H_USED] < mp[H_LEN]) {
			for (handle = len - 1; handle >= 0; handle--) {
				if (mp[handle + H_OFFSET])
					break;
			}
		} else {
			handle = len;
		}
	}
	return handle + 1;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Allocate an entry in the halloc array.
//

int hAllocEntry(void ***list, int *max, int size)
{
	char	*cp;
	int		id;

	a_assert(list);
	a_assert(max);

	if ((id = hAlloc((void***) list)) < 0) {
		return -1;
	}

	if (size > 0) {
		if ((cp = (char*) balloc(B_L, size)) == NULL) {
			hFree(list, id);
			return -1;
		}
		a_assert(cp);
		memset(cp, 0, size);

		(*list)[id] = (void*) cp;
	}

	if (id >= *max) {
		*max = id + 1;
	}
	return id;
}

////////////////////////////////////////////////////////////////////////////////

int fmtAlloc(char **buf, int maxSize, char *fmt, ...)
{
	va_list		args;
	int			rc;

	va_start(args, fmt);	
	rc = mprAllocVsprintf(buf, maxSize, fmt, args);
	va_end(args);	
	return rc;
}

////////////////////////////////////////////////////////////////////////////////

int fmtValloc(char **buf, int maxSize, char *fmt, va_list arg)
{
	return mprAllocVsprintf(buf, maxSize, fmt, arg);
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Security Handler //////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	The GaCompatHandler is created for each incoming HTTP request. We tell
//	the Handler base class that we must run ALWAYS as a non-terminal
//	handler. Handlers be non-terminal where they can modify the request,
//	but not actually handle it.
//

GaCompatHandler::GaCompatHandler(char *extensions) :
	MaHandler("gaCompatHandler", extensions, MPR_HANDLER_ALWAYS)
{
	if (defaultServer == 0) {
		defaultServer = maGetHttp()->findServer("default");
		defaultHost = defaultServer->getDefaultHost();
	}
#if UNUSED
	if (defaultInterp == 0) {
		defaultInterp = ejsGetDefaultInterp();
	}
	if (defaultServer == 0 || defaultHost == 0 || defaultInterp == 0) {
		error(MPR_L, "Undefined server, host or EJS engine");
		return;
	}
#endif
}


////////////////////////////////////////////////////////////////////////////////
//
//	Destructor for the GaCompatHandler
//

GaCompatHandler::~GaCompatHandler()
{
}


////////////////////////////////////////////////////////////////////////////////
//
//	For maximum speed in servicing requests, we clone a pre-existing
//	handler when an incoming request arrives.
//

MaHandler *GaCompatHandler::cloneHandler()
{
	return new GaCompatHandler(extensions);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Called to see if this handler should process this request.
//	Return TRUE if you want this handler to match this request
//

int GaCompatHandler::matchRequest(MaRequest *rq, char *uri, int uriLen)
{
	//	Always match
	return 1;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Run the handler to service the request
//

int GaCompatHandler::run(MaRequest *rq)
{
	char  *uri;

	uri = rq->getUri();

	if (securityChecks(rq) < 0) {
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Not yet debugged
//

int GaCompatHandler::securityChecks(MaRequest *rq)
{
#if FUTURE
	accessMeth_t	am;
	MaAuth			*auth;
	char			*type, *accessLimit, *requiredPassword;
	char			*uriPath, *ext, *mimeType, *query, *password, *user;
	char			*realm;
	int				flags, nRet, isSecure, authType;

	accessLimit = 0;
	realm = "UNKNOWN";

	auth = rq->getAuth();
	uriPath = maGetRequestUriPath(rq);
	ext = maGetRequestUriExt(rq);
	mimeType = maGetRequestMimeType(rq);
	query = maGetRequestUriQuery(rq);
	isSecure = maIsRequestSecure(rq);

	//
 	//	Get the critical request details
 	//
	password = maGetRequestPassword(rq);
	user = maGetRequestUserName(rq);

	//
	//	Get the access limit for the URL.  Exit if none found.
	//
	accessLimit = umGetAccessLimit((char*) uriPath);
	if (accessLimit == NULL) {
		return 0;
	}
		 
	//
	//	Check to see if URL must be encrypted
	//
#if BLD_FEATURE_SSL_MODULE
	nRet = umGetAccessLimitSecure(accessLimit);
	if (nRet && isSecure) {
		maSetResponseError(rq, 405, 
			"Access Denied\nSecure access is required.");
		trace(3, "SEC: Non-secure access attempted on <%s>\n", uriPath);
		bfree(B_L, accessLimit);
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}
#endif

	//
	//	Get the access limit for the URL
	//
	am = umGetAccessMethodForURL(accessLimit);
	authType = (am == AM_DIGEST) ? MPR_HTTP_AUTH_DIGEST : MPR_HTTP_AUTH_BASIC;

	if (strcmp("127.0.0.1", maGetRequestIpAddr(rq)) == 0) {
		//
		//	Local access is always allowed
		//
		return 0;
	}

	if (am == AM_NONE) {
		//
		//	URL is supposed to be hidden!  Make like it wasn't found.
		//
		maSetResponseError(rq, 404, "Page Not Found");
		bfreeSafe(B_L, accessLimit);
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	} 

	requiredPassword = umGetUserPassword((char*) user);
	auth->setRequiredPassword();

	if (user && *user) {

		if (! umUserExists((char*) user)) {
			formatAuthResponse(rq, auth, realm, authType, 401, 
				"Access Denied\nUnknown User");
			trace(3, "SEC: Unknown user <%s> attempted to access <%s>\n", 
				user, uriPath);
			bfreeSafe(B_L, accessLimit);
			return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
		}
		if (! umUserCanAccessURL((char*) user, accessLimit)) {
			formatAuthResponse(rq, auth, realm, authType, 403, 
				"Access Denied\nProhibited User");
			bfreeSafe(B_L, accessLimit);
			return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
		}
		if (password && * password) {

			auth->setRequiredPassword();
			if (requiredPassword) {
				if (gstrcmp(password, requiredPassword) != 0) {
					maSetResponseError(rq, 401, 
						"Access Denied\nWrong Password");
					trace(3, "SEC: Password fail for user <%s>"
								"attempt to access <%s>\n", user, uriPath);
					bfreeSafe(B_L, accessLimit);
					return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
				}
				bfree (B_L, requiredPassword);
			}

#if BLD_FEATURE_DIGEST
		} else if (mprStrcmpAnyCase(maGetRequestAuthType(rq), "digest") == 0) {

			char *digestCalc;

			//
			//	Check digest for equivalence
			//
			requiredPassword = umGetUserPassword((char*) user);
			maSetRequestPassword(requiredPassword);
#if USING_AUTH_HANDLER_FOR_THIS
			a_assert(wp->digest);
			a_assert(wp->nonce);
							 
			digestCalc = websCalcDigest(wp);
			maCalcDigest(&requiredDigest, userName, requiredPassword, realm, 
				uri, nonce, qop, nc, 
				cnonce, rq->getMethod());
			mprFree(requiredPassword);
			a_assert(digestCalc);

			if (gstrcmp(wp->digest, digestCalc) != 0) {
				formatAuthResponse(rq, auth, realm, authType, 401, 
					"Access Denied\nWrong Password");
				bfreeSafe(B_L, accessLimit);
				return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
			}
			bfree (B_L, digestCalc);
#endif
#endif
		} else {
			//
			//	No password has been specified
			//
			formatAuthResponse(rq, auth, realm, authType, 401, 
				"Access to this document requires a password");
			bfreeSafe(B_L, accessLimit);
			return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
		}

	} else if (am != AM_FULL) {
		//
		//	This will cause the browser to display a password / username dialog
		//
		formatAuthResponse(rq, auth, realm, authType, 401, 
			"Access to this document requires a User ID");
		bfreeSafe(B_L, accessLimit);
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}
#endif
	return 0;
}


////////////////////////////////////////////////////////////////////////////////

void GaCompatHandler::formatAuthResponse(MaRequest *rq, MaAuth *auth, 
	char *realm, int authType, int code, char *userMsg)
{
#if BLD_FEATURE_DIGEST
	char		*nonceStr;
	char		*etag;
#endif
	char		*buf, *headers;

	rq->incAccessError();

	mprLog(3, "auth: formatAuthResponse: code %d, %s\n", code, userMsg);
	mprAllocSprintf(&buf, MPR_HTTP_BUFSIZE, 
		"<HTML><HEAD>\n<TITLE>Authentication Error: %s</TITLE>\n</HEAD>\r\n"
		"<BODY><H2>Authentication Error: %s</H2></BODY>\n</HTML>\r\n",
		userMsg, userMsg);

	if (authType == MPR_HTTP_AUTH_BASIC) {
		mprAllocSprintf(&headers, MPR_MAX_STRING,
			"WWW-Authenticate: Basic realm=\"%s\"", realm);

#if BLD_FEATURE_DIGEST
	} else if (authType == MPR_HTTP_AUTH_DIGEST) {

		//
		//	Use the etag as our opaque string
		//
		etag = rq->getEtag();
		if (etag == 0) {
			etag = "";
		}
		maCalcNonce(&nonceStr, rq->host->getSecret(), etag, realm);

		mprAllocSprintf(&headers, MPR_MAX_STRING,
			"WWW-Authenticate: Digest realm=\"%s\", domain=\"%s\", "
			"qop=\"auth\", nonce=\"%s\", opaque=\"%s\", algorithm=\"MD5\", "
			"stale=\"FALSE\"", 
			realm, rq->host->getName(), nonceStr, etag);

		mprFree(nonceStr);
#endif
	}
	rq->setHeader(headers);
	rq->formatAltResponse(code, buf, MPR_HTTP_DONT_ESCAPE);
	rq->flushOutput(MPR_HTTP_FOREGROUND_FLUSH, MPR_HTTP_FINISH_REQUEST);
	mprFree(headers);
	mprFree(buf);
}

////////////////////////////////////////////////////////////////////////////////

}	// extern "C"
#else
void mprCompatModuleDummy() {}

#endif // BLD_FEATURE_GACOMPAT_MODULE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
