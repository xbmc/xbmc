///
///	@file   uploadHandler.cpp
///	@brief  Form-based file upload handler. 
/// @overview This upload module is a non-terminal handler. It intercepts 
///		post data according to RFC-1867 ("multipart/form-data" post data). 
///		It saves the uploaded files in a configured upload directory and 
///		creates files[] variables to describe the uploaded files.  
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
//	This module was initially developed with assistance from Guntermann & 
//	Drunck GmbH Systementwicklung, Germany.
//
//////////////////////////////////// Includes //////////////////////////////////

#include	"uploadHandler.h"

//////////////////////////////////// Locals ////////////////////////////////////
#if BLD_FEATURE_UPLOAD_MODULE

static MaUploadHandlerService *uploadHandlerService;

static char *findBoundary(void *buf, int bufLen, void *boundary, 
				int boundaryLen);

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MaUploadModule ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int mprUploadInit(void *handle)
{
	(void) new MaUploadModule(handle);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MaUploadModule::MaUploadModule(void *handle) : MaModule("upload", handle)
{
	uploadHandlerService = new MaUploadHandlerService();
}

////////////////////////////////////////////////////////////////////////////////

MaUploadModule::~MaUploadModule()
{
	delete uploadHandlerService;
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////// MaUploadHandlerService ////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaUploadHandlerService::MaUploadHandlerService(): 
	MaHandlerService("uploadHandler")
{
	callback = 0;
	callbackData = 0;

#if BLD_FEATURE_LOG
	log = new MprLogModule("upload");
#endif
#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
#endif

#if WIN
	uploadDir = mprStrdup(getenv("TEMP"));
	for (char *cp = uploadDir; *cp; cp++) {
		if (*cp == '\\') {
			*cp = '/';
		}
	}
#else
	uploadDir = mprStrdup("/tmp");
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaUploadHandlerService::~MaUploadHandlerService()
{
	mprFree(uploadDir);

#if BLD_FEATURE_LOG
	delete log;
#endif

#if BLD_FEATURE_MULTITHREAD
	mutex->lock();
	delete mutex;
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaUploadHandlerService::newHandler(MaServer *server, MaHost *host,
		char *ext)
{
	MaUploadHandler *ep;

	ep = new MaUploadHandler(ext, log);
	return ep;
}

////////////////////////////////////////////////////////////////////////////////

void MaUploadHandlerService::setUploadDir(char *dir)
{
	mprFree(uploadDir);
	uploadDir = mprStrdup(dir);
}

////////////////////////////////////////////////////////////////////////////////

MaUploadCallback 
	MaUploadHandlerService::setUserCallback(MaUploadCallback userCallback,
		void *data)
{
	MaUploadCallback last = callback;

	callback = userCallback;
	callbackData = data;

	return last;
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MaUploadHandler ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaUploadHandler::MaUploadHandler(char *ext, MprLogModule *serviceLog):
	MaHandler("uploadHandler", 0,
		MPR_HANDLER_POST | MPR_HANDLER_MAP_VIRTUAL |
		MPR_HANDLER_NEED_ENV)
{
	log = serviceLog;

	contentState = MPR_UPLOAD_REQUEST_HEADER;
	boundary = 0;
	boundaryLen = 0;
	postBuf = 0;
	fileName = 0;
	filePath = 0;
	fileSize = 0;
	nameField = 0;

	upfile = new MprFile();
	currentFile = mprCreateUndefinedVar();
	uploadDir = mprStrdup(uploadHandlerService->getUploadDir());
	location = 0;

	callback = uploadHandlerService->getCallback();
	callbackData = uploadHandlerService->getCallbackData();
}

////////////////////////////////////////////////////////////////////////////////

MaUploadHandler::~MaUploadHandler()
{
	if (filePath && *filePath) {
		unlink(filePath);
	}
	mprFree(fileName);
	mprFree(filePath);
	mprFree(boundary);
	mprFree(uploadDir);

	if (postBuf) {
		delete postBuf;
	}
	if (upfile) {
		delete upfile;
	}

	if (currentFile.type == MPR_TYPE_OBJECT) {
		mprLog(5, log, "Destroy files[%s]\n", currentFile.name);
		mprDestroyVar(&currentFile);
	}
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaUploadHandler::cloneHandler()
{
	MaUploadHandler	*ep;

	ep = new MaUploadHandler(extensions, log);

	//	FUTURE -- could be optimized. Is always duped in constructor above.
	mprFree(ep->uploadDir);
	ep->uploadDir = mprStrdup(uploadDir);
	ep->location = location;
	return ep;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_CONFIG_PARSE

int MaUploadHandler::parseConfig(char *key, char *value, MaServer * server,
		MaHost * host, MaAuth * auth, MaDir * dir, MaLocation *lp)
{
	char	pathBuf[MPR_MAX_FNAME], pathBuf2[MPR_MAX_FNAME];

	if (mprStrCmpAnyCase(key, "FileUploadDir") == 0) {
		value = mprStrTrim(value, '\"');

		host->replaceReferences(pathBuf2, sizeof(pathBuf2), value);
		if (host->makePath(pathBuf, sizeof(pathBuf), pathBuf2) == 0) {
			mprError(MPR_L, MPR_LOG, "FormUploadDir path is too long");
			return -1;
		}
		mprAssert(pathBuf2[0]);

		mprFree(uploadDir);
		uploadDir = mprStrdup(pathBuf);
		location = lp;

		mprLog(MPR_CONFIG, log, "Upload directory: %s\n", uploadDir);
		return 1;
	}
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Override default matchRequest
//

int MaUploadHandler::matchRequest(MaRequest *rq, char *uri, int uriLen)
{
	return 1;
	if (location == 0) {
		return 0;
	}

	return (strncmp(location->getPrefix(), uri, location->getPrefixLen()) == 0);
}

////////////////////////////////////////////////////////////////////////////////

int MaUploadHandler::setup(MaRequest * rq)
{
	MaLimits *limits;

	mprAssert(rq);

	limits = rq->host->getLimits();
	mprAssert(postBuf == 0);
	postBuf = new MprBuf(MPR_HTTP_IN_BUFSIZE, limits->maxBody);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Read some post data. Called by our run method. Since the handler is 
//	non-terminal, this function never gets called by the server.
//

void MaUploadHandler::postData(MaRequest * rq, char *postData, int postLen)
{
	char 	*line, *fileData, *nextTok;
	int 	fileDataLen;

	mprAssert(rq);

	fileData = 0;
	fileDataLen = 0;
	line = 0;

	mprLog(5, log, "%d: postData %d bytes\n", rq->getFd(), postLen);
	if (postLen < 0 && rq->getRemainingContent() > 0) {
		return;
	}

	//
	//	Copy the data to our buffer. We need to be able to parse various 
	//	headers and need to aggregate data until we have the complete header
	//	(terminated by a '\n'). Note we always add a silent null terminator.
	//
	postBuf->copyDown();
	if (postBuf->put((uchar*) postData, postLen) < 0) {
		rq->requestError(MPR_HTTP_BAD_REQUEST, "Upload file too big\n");
		rq->finishRequest();
		return;
	}
	postBuf->addNull();

	while (1) {
		//
		//	Read the next line
		//
		switch (contentState) {
		case MPR_UPLOAD_BOUNDARY:
		case MPR_UPLOAD_CONTENT_HEADER:
			line = postBuf->getStart();
			mprStrTok(line, "\n", &nextTok);
			if (nextTok == 0) {
				return;							// Incomplete line
			}
			postBuf->adjustStart(nextTok - line);
			mprStrTrim(line, '\r');
			break;

		case MPR_UPLOAD_CONTENT_DATA:
		default:
			// Data read below
			;
		}

		//
		//	Do per state processing
		//
		switch (contentState) {
		case MPR_UPLOAD_BOUNDARY:
			if (processContentBoundary(rq, line) < 0) {
				return;
			}
			break;

		case MPR_UPLOAD_CONTENT_HEADER:
			if (processContentHeader(rq, line) < 0) {
				return;
			}
			break;

		case MPR_UPLOAD_CONTENT_DATA:
			if (processContentData(rq) < 0) {
				return;
			}
			if (postBuf->getLength() < boundaryLen) {
				//	Incomplete boundary. Return and get more data
				return;
			}
			break;

		case MPR_UPLOAD_CONTENT_END:
			if (callback) {
				MprVar		*file, *filesVar, *variables;

				variables = rq->getVariables();
			
				filesVar = &variables[MA_FILES_OBJ];
				file = mprGetFirstProperty(filesVar, 0);
				while (file) {
					(*callback)(rq, (void*) this, callbackData, file);
					file = mprGetNextProperty(filesVar, file, 0);
				}
			}
			return;
		}
	}

	if (rq->getRemainingContent() <= 0) {
		//
		//	All done
		//
		contentState = MPR_UPLOAD_CONTENT_END;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Process the mime boundary division
//
//	Returns  < 0 on a request or state error
//			== 0 if successful
//

int MaUploadHandler::processContentBoundary(MaRequest * rq, char *line)
{
	//
	//	Expecting a multipart boundary string
	//
	if (strncmp(boundary, line, boundaryLen) != 0) {
		rq->requestError(MPR_HTTP_BAD_REQUEST, 
			"Bad upload state. Incomplete boundary\n");
		rq->finishRequest();
		return MPR_ERR_BAD_STATE;
	}
	if (line[boundaryLen] && 
			strcmp(&line[boundaryLen], "--") == 0) {
		mprLog(7, log, "End of content.\n");
		contentState = MPR_UPLOAD_CONTENT_END;

	} else {
		mprLog(7, log, "Starting new header.\n");
		contentState = MPR_UPLOAD_CONTENT_HEADER;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Expecting content headers. A blank line indicates the start of
//	the data.
//
//	Returns	 < 0  Request or state error
//	Returns	== 0  Successfully parsed the input line.
//

int MaUploadHandler::processContentHeader(MaRequest * rq, char *line)
{
	MprVar 	*variables;
	char	tmpFile[MPR_MAX_FNAME];
	char 	*key, *headerTok;
	char	*rest, *nextPair, *value;

	if (line[0] == '\0') {
		contentState = MPR_UPLOAD_CONTENT_DATA;
		return 0;
	}
	mprLog(5, log, "Header line: %s\n", line);

	headerTok = line;
	mprStrTok(line, ": ", &rest);

	if (mprStrCmpAnyCase(headerTok, "Content-Disposition") == 0) {
		//
		//	The content disposition header describes either a form
		//	variable or an uploaded file.
		//
		//		Content-Disposition: form-data; name="field1"
		//		>>blank line
		//		Field Data
		//		---boundary
		//
		//		Content-Disposition: form-data; name="field1" ->
		//			filename="user.file"
		//		>>blank line
		//		File data
		//		---boundary
		//
		key = rest;
		nameField = fileName = 0;
		while (key && mprStrTok(key, ";\r\n", &nextPair)) {

			key = mprStrTrim(key, ' ');
			mprStrTok(key, "= ", &value);
			value = mprStrTrim(value, '\"');

			if (mprStrCmpAnyCase(key, "form-data") == 0) {
				// Nothing to do

			} else if (mprStrCmpAnyCase(key, "name") == 0) {
				mprFree(nameField);
				nameField = mprStrdup(value);

			} else if (mprStrCmpAnyCase(key, "filename") == 0) {
				if (nameField == 0) {
					rq->requestError(MPR_HTTP_BAD_REQUEST, 
						"Bad upload state. Missing name field.\n");
					rq->finishRequest();
					return MPR_ERR_BAD_STATE;
				}
				mprFree(fileName);
				fileName = mprStrdup(value);

				//
				//	Create the file to hold the uploaded data
				//
				mprMakeTempFileName(tmpFile, sizeof(tmpFile), 
					uploadDir);
				filePath = mprStrdup(tmpFile);

				mprLog(5, log, "File upload of: %s stored as %s\n", 
					fileName, filePath);

				if (upfile->open(filePath, 
					O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,
						0600) < 0) {
					rq->requestError(MPR_HTTP_INTERNAL_SERVER_ERROR, 
						"Can't open upload temp file %s\n", filePath);
					rq->finishRequest();
					return MPR_ERR_BAD_STATE;
				}

				//
				//	Create the files[] object. We use currentFile as
				//	a local variable. Destroying the old reference 
				//	does not destroy the object as there is still a 
				//	reference to the object via "files[entry]".
				//
				if (currentFile.type == MPR_TYPE_OBJECT) {
					mprLog(5, log, "Destroy old files[%s]\n", currentFile.name);
					mprDestroyVar(&currentFile);
				}
				currentFile = mprCreateObjVar(nameField, MA_HTTP_HASH_SIZE);

				variables = rq->getVariables();
				//
				//	Create the files[entry]
				//
				mprCreateProperty(&variables[MA_FILES_OBJ], nameField, 
					&currentFile);
				mprCreatePropertyValue(&currentFile, "CLIENT_FILENAME", 
					mprCreateStringVar(fileName, 0));

				mprCreatePropertyValue(&currentFile, "FILENAME", 
					mprCreateStringVar(filePath, 0));
			}
			key = nextPair;
		}

	} else if (mprStrCmpAnyCase(headerTok, "Content-Type") == 0) {
		if (fileName) {
			mprLog(5, log, "Set files[%s][CONTENT_TYPE] = %s\n", 
				currentFile.name, rest);
			mprCreatePropertyValue(&currentFile, "CONTENT_TYPE", 
				mprCreateStringVar(rest, 0));
		}
	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Process the content data.
//
//	Returns < 0 on error
//			== 0 when more data is needed
//			== 1 when data successfully written
//
int MaUploadHandler::processContentData(MaRequest * rq)
{
	char 	*fileData, *bp;
	int 	size, fileDataLen, rc;

	size = postBuf->getLength();
	if (size < boundaryLen) {
		//	Incomplete boundary. Return and get more data
		return 0;
	}

	bp = findBoundary(postBuf->getStart(), size, boundary, boundaryLen);
	if (bp == 0) {

		if (fileName) {
			//
			//	No signature found yet. probably more data to come.
			//	Since we did not match the boundary above we know there is no 
			//	boundary. But there could be most of the boundary with the 
			//	boundary tail in the next block of data. Must always preserve 
			//	boundary-1 bytes.
			//
			fileData = postBuf->getStart();
			fileDataLen = postBuf->getEnd() - fileData - (boundaryLen - 1);

			if (fileDataLen > 0) {
				postBuf->adjustStart(fileDataLen);

				//
				//	File upload. Write the file data.
				//
				rc = upfile->write(fileData, fileDataLen);
				if (rc != fileDataLen) {
					rq->requestError(MPR_HTTP_INTERNAL_SERVER_ERROR, 
						"Can't write to upload temp file %s, "
						"rc %d, errno %d\n", 
						filePath, rc, mprGetOsError());
					rq->finishRequest();
					return MPR_ERR_CANT_WRITE;
				}
				fileSize += fileDataLen;
				mprSetPropertyValue(&currentFile, "SIZE", 
					mprCreateIntegerVar(fileSize));
			}
		}
		return 0;		// Get more data
	}

	mprLog(7, log, "Boundary found\n");
	fileData = postBuf->getStart();
	fileDataLen = bp - fileData;

	if (fileDataLen > 0) {

		postBuf->adjustStart(fileDataLen);

		//
		//	This is the CRLF before the boundary
		//
		if (fileDataLen >= 2 && 
				fileData[fileDataLen - 2] == '\r' && 
				fileData[fileDataLen - 1] == '\n') {
			fileDataLen -= 2;
		}

		if (fileName) {
			//
			//	File upload. Write the file data
			//
			rc = upfile->write(fileData, fileDataLen);
			if (rc != fileDataLen) {
				rq->requestError(MPR_HTTP_INTERNAL_SERVER_ERROR, 
					"Can't write trailing data to upload temp file %s, "
					"rc %d, errno %d\n", 
					filePath, rc, mprGetOsError());
				rq->finishRequest();
				return MPR_ERR_CANT_WRITE;
			}
			fileSize += fileDataLen;
			mprSetPropertyValue(&currentFile, "SIZE", 
				mprCreateIntegerVar(fileSize));

			//
			//	Now have all the data (we've seen the boundary)
			//
			upfile->close();
			fileName = 0;

		} else {
			//
			//	Normal string form data variables
			//
			fileData[fileDataLen] = '\0';
			mprLog(5, log, "Set form[%s] = %s\n", nameField, fileData);
			rq->setVar(MA_FORM_OBJ, nameField, fileData);
		}
	}
	contentState = MPR_UPLOAD_BOUNDARY;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaUploadHandler::run(MaRequest * rq)
{
	MaHeader 	*header;
	char 		*type, *param, *postDataBuf;
	int 		len;

	mprAssert(rq);

	if (!(rq->getFlags() & MPR_HTTP_POST_REQUEST)) {
		return 0;
	}

	if (rq->getRemainingContent() <= 0) {
		return 0;
	}

	header = rq->getHeader();
	type = mprStrdup(header->contentMimeType);

	//
	//	See if this request has multipart form data. If so, we are active.
	//
	mprStrTrim(type, '\r');
	mprStrTok(type, "; \r\n", &param);

	if (mprStrCmpAnyCase(type, "multipart/form-data") != 0) {
		mprFree(type);
		return 0;
	}

	//
	//	The string to use as the multipart boundary divider is the next token
	//
	if (mprStrCmpAnyCaseCount(param, "boundary=", 9) == 0) {
		param += 9;
	}

	boundaryLen = mprAllocSprintf(&boundary, MPR_MAX_STRING, 
		"--%s", param);
	mprFree(type);
	if (boundaryLen == 0 || *boundary == '\0') {
		rq->requestError(MPR_HTTP_BAD_REQUEST, "Bad boundary\n");
		rq->finishRequest();
		mprFree(type);
		return 0;
	}

	rq->setVar(MA_SERVER_OBJ, "UPLOAD_DIR", uploadDir);

	//	
	//	Process all the POST data. 
	//	FUTURE: this will not work single threaded as it will block the 
	//	current thread while reading the uploaded data.
	//
	rq->setPullPost();

	postDataBuf = (char *) mprMalloc(UPLOAD_BUF_SIZE);
	if (!postDataBuf) {
		rq->requestError(MPR_HTTP_INTERNAL_SERVER_ERROR, 
			"Can't allocate %d bytes\n", UPLOAD_BUF_SIZE);
		rq->finishRequest();
		return 0;
	}
	contentState = MPR_UPLOAD_BOUNDARY;

	//
	//	Continually loop reading post data until we are done. len may be 
	//	zero for EOF and will be negative on errors.
	//
	len = rq->readPostData(postDataBuf, UPLOAD_BUF_SIZE);
	while (len > 0) {
		if (rq->getState() != MPR_HTTP_RUNNING) {
			break;
		}
		postData(rq, postDataBuf, len);
		if (rq->getState() != MPR_HTTP_RUNNING) {
			break;
		}
		len = rq->readPostData(postDataBuf, UPLOAD_BUF_SIZE);
	}

	if (rq->getState() == MPR_HTTP_RUNNING) {
		if (len < 0 || rq->getRemainingContent() > 0 || 
				contentState != MPR_UPLOAD_CONTENT_END) {
			rq->requestError(MPR_HTTP_BAD_REQUEST, 
				"Incomplete upload post data. Remaining %d, state %d\n",
				rq->getRemainingContent(), contentState);
			rq->finishRequest();
			mprFree(postDataBuf);
			return 0;
		}
	}

	mprFree(postDataBuf);
	rq->setFlags(0, ~MPR_HTTP_PULL_POST);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Find the boundary signature in memory. Returns pointer to the first match.
//	

static char *findBoundary(void *buf, int bufLen, void *boundary, 
	int boundaryLen)
{
	char 	*cp, *endp;
	char 	first;

	mprAssert(buf);
	mprAssert(boundary);
	mprAssert(boundaryLen > 0);

	first = *((char*) boundary);
	cp = (char*) buf;

	if (bufLen < boundaryLen) {
		return 0;
	}
	endp = cp + (bufLen - boundaryLen) + 1;
	while (cp < endp) {
		cp = (char *) memchr(cp, first, endp - cp);
		if (!cp) {
			return 0;
		}
		if (memcmp(cp, boundary, boundaryLen) == 0) {
			return cp;
		}
		cp++;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MaUploadCallback maUploadSetUserCallback(MaUploadCallback callback, 
		void *data)
{
	return uploadHandlerService->setUserCallback(callback, data);
}

////////////////////////////////////////////////////////////////////////////////
#else

void mprUploadHandlerDummy()
{
}

#endif // BLD_FEATURE_UPLOAD_MODULE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
