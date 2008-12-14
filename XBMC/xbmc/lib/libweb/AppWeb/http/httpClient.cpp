///
///	@file 	httpClient.cpp
/// @brief 	Http client program to fetch URLs
/// @overview The httpClient program functions well as a client to 
///		retrieve URLs from web servers and as a test client to verify 
///		the http server. It is multi-threaded and can be used to load 
///		and stress test the server. It will run without multi-threading 
///		if required.
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
////////////////////////////////// Includes ////////////////////////////////////

#define	IN_CLIENT_LIBRARY 1

#include	"client.h"

/////////////////////////////////// Defines ////////////////////////////////////

#define HTTP_DEFAULT_ITERATIONS		1
#define HTTP_DEFAULT_POOL_THREADS	10
#define HTTP_DEFAULT_LOAD_THREADS	1

//
//	Request types
//
#define HTTP_HEAD		1
#define HTTP_GET		2
#define HTTP_POST		3
#define HTTP_DELETE		4
#define HTTP_OPTIONS	5
#define HTTP_TRACE		6

//////////////////////////////////// Locals ////////////////////////////////////

static int		activeLoadThreads;	// Still running test threads
static int		benchmark;			// Output benchmarks
static int		continueOnErrors;
static char		*cmpDir;			// Dir of web content to compare results
static int		success;			// Total success flag
static int		fetchCount;			// Total count of fetches
static char		*fileList;			// File of URLs
static Mpr		*mpr;				// MPR object
static int		iterations;			// URLs to fetch
static int		httpVersion;		// HTTP/1.x
static char		*host;				// Host to connect to
static int		loadThreads;		// Number of threads to use for URL requests
static char		*method;			// HTTP method when URL on cmd line
static int		outputHeader;		// Write headers as well as content
static int		poolThreads;		// Pool threads. >0 if multi-threaded
static char		*postData;			// Post data
static int		postLen;			// Length of post data
static int		retries;
static int		quietMode;			// Suppress error messages
static int		saveArgc;
static char**	saveArgv;
static int		singleStep;			// Pause between requests
static int		timeout;
static MprLogModule 
				*tMod;
static int 		trace;				// Trace requests
static int		verbose;
static char		*writeDir;			// Save output to this directory

#if BLD_FEATURE_MULTITHREAD
static MprMutex	*mutex;
#endif

////////////////////////////// Forward Declarations ////////////////////////////

static int		fetch(MaClient *client, char *method, char *url, 
					char *data, int len);

#if BLD_FEATURE_MULTITHREAD
static void		doTests(void *data, MprThread *tp);
static void		lock();
static void		unlock();
#else
static void		doTests(void *data, void *tp);
inline void		lock() {};
inline void		unlock() {};
#endif

//////////////////////////////////// Code //////////////////////////////////////
//
//	Normal main
//

int main(int argc, char *argv[])
{
	double			elapsed;
	char			*programName, *argp, *logSpec;
	int				c, errflg, start;
#if BLD_FEATURE_LOG
	MprLogToFile	*logger;
#endif
#if BLD_FEATURE_MULTITHREAD
	MprThread		*threadp;
#endif

	programName = mprGetBaseName(argv[0]);
	method = "GET";
	fileList = cmpDir = writeDir = 0;
	verbose = continueOnErrors = outputHeader = errflg = 0;
	poolThreads = 4;			// Need at least one to run efficiently
	httpVersion = 1;			// HTTP/1.1
	success = 1;
	trace = 0;
	host = "localhost";
	logSpec = "stdout:1";
	postData = 0;
	postLen = 0;
	retries = MPR_HTTP_CLIENT_RETRIES;
	iterations = HTTP_DEFAULT_ITERATIONS;
	loadThreads = HTTP_DEFAULT_LOAD_THREADS;
	timeout = MPR_HTTP_CLIENT_TIMEOUT;

#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
#endif

	//
	//	FUTURE: switch to GNU style --args with a better usage message
	//
	MprCmdLine	cmdLine(argc, argv, "bC:cDd:f:Hh:i:l:M:mqo:r:st:T:vV:w:");
	while ((c = cmdLine.next(&argp)) != EOF) {
		switch(c) {
		case 'b':
			benchmark++;
			break;

		case 'c':
			continueOnErrors++;
			break;

		case 'C':
			cmpDir = argp;
			break;

		case 'd':
			postData = argp;
			postLen = strlen(postData);
			break;

		case 'D':
			mprSetDebugMode(1);
			break;

		case 'f':
			fileList = argp;
			break;

		case 'h':
			host = argp;
			break;

		case 'H':
			outputHeader++;
			break;

		case 'i':
			iterations = atoi(argp);
			break;

		case 'l':
			logSpec = argp;
			break;

		case 'm':
			mprRequestMemStats(1);
			break;

		case 'M':
			method = argp;
			break;

		case 'o':
			timeout = atoi(argp);
			break;

		case 'q':
			quietMode++;
			break;

		case 'r':
			retries = atoi(argp);
			break;

		case 's':
			singleStep++;
			break;

		case 't':
			loadThreads = atoi(argp);
			break;

		case 'T':
			poolThreads = atoi(argp);
			break;

		case 'v':
			verbose++;
			trace++;
			break;

		case 'V':
			httpVersion = atoi(argp);
			break;

		case 'w':
			writeDir = argp;
			break;

		default:
			errflg++;
			break;
		}
	}
	if (writeDir && (loadThreads > 1)) {
		errflg++;
	}

	if (errflg) {
		mprFprintf(MPR_STDERR, 
			"usage: %s [-bcHMmqsTv] [-C cmpDir] [-d postData] [-f fileList]\n"
			"	[-i iterations] [-l logSpec] [-M method] [-o timeout]\n"
			"	[-h host] [-r retries] [-t threads] [-T poolThreads]\n"
			"	[-V httpVersion] [-w writeDir] [urls...]\n", programName);
		exit(2);
	}
	saveArgc = argc - cmdLine.firstArg();
	saveArgv = &argv[cmdLine.firstArg()];

	mpr = new Mpr(programName);

#if BLD_FEATURE_LOG
	tMod = new MprLogModule("httpClient");
	logger = new MprLogToFile();
	mpr->addListener(logger);
	if (mpr->setLogSpec(logSpec) < 0) {
		mprFprintf(MPR_STDERR, "Can't open log file %s\n", logSpec);
		exit(2);
	}
#endif

	//
	//	Alternatively, set the configuration manually
	//
	mpr->setAppTitle("Mbedthis HTTP Client");
#if BLD_FEATURE_MULTITHREAD
	mpr->setMaxPoolThreads(poolThreads);
#endif

	//
	//	Start the Timer, Socket and Pool services
	//
	if (mpr->start(MPR_SERVICE_THREAD) < 0) {
		mprError(MPR_L, MPR_USER, "Can't start MPR for %s", mpr->getAppTitle());
		delete mpr;
		exit(2);
	}

	//
	//	Create extra test threads to run the tests as required. We use
	//	the main thread also (so start with j==1)
	//
	start = mprGetTime(0);
#if BLD_FEATURE_MULTITHREAD
	activeLoadThreads = loadThreads;
	for (int j = 1; j < loadThreads; j++) {
		char name[64];
		mprSprintf(name, sizeof(name), "t.%d", j - 1);
		threadp = new MprThread(doTests, MPR_NORMAL_PRIORITY, (void*) j, name); 
		threadp->start();
	}
#endif

	doTests(0, 0);

	//
	//	Wait for all the threads to complete (simple but effective). Keep 
	//	servicing events as we wind down.
	//
	while (activeLoadThreads > 1) {
		mprSleep(100);
	}

	if (benchmark && success) {
		elapsed = (mprGetTime(0) - start);
		if (fetchCount == 0) {
			elapsed = 0;
			fetchCount = 1;
		}
		mprPrintf("\tThreads %d, Pool Threads %d   \t%13.2f\t%12.2f\t%6d\n", 
			loadThreads, poolThreads, elapsed * 1000.0 / fetchCount, 
			elapsed / 1000.0, fetchCount);

		mprPrintf("\nTime elapsed:        %13.4f sec\n", elapsed / 1000.0);
		mprPrintf("Time per request:    %13.4f sec\n", elapsed / 1000.0 
			/ fetchCount);
		mprPrintf("Requests per second: %13.4f\n", fetchCount * 1.0 / 
			(elapsed / 1000.0));
	}
	if (! quietMode) {
		mprPrintf("\n");
	}

	mpr->stop(0);

#if BLD_FEATURE_MULTITHREAD
	delete mutex;
#endif
#if BLD_FEATURE_LOG
	delete tMod;
#endif

	delete mpr;
#if BLD_FEATURE_LOG
	delete logger;
#endif
	mprMemClose();
	return (success) ? 0 : 255;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Do the real work here
// 

#if BLD_FEATURE_MULTITHREAD
static void doTests(void *data, MprThread *threadp)
#else
static void doTests(void *data, void *threadp)
#endif
{
	MprBuf		post;
	FILE		*fp;
	MaClient	*client;
	char		urlBuf[4096], urlBuf2[4096];
	char		postBuf[4096];
	char		*cp, *operation, *url, *tok;
	int			rc, j, line;

	fp = 0;

	client = new MaClient();
	client->setTimeout(timeout);
	client->setRetries(retries);
	if (httpVersion == 0) {
		client->setKeepAlive(0);
	}

	post.setBuf(MPR_HTTP_CLIENT_BUFSIZE, MPR_HTTP_MAX_BODY);

	while (!mpr->isExiting() && success) {
		lock();
		if (fetchCount >= iterations) {
			unlock();
			break;
		}
		unlock();
		rc = 0;

		for (j = 0; j < saveArgc; j++) {
			url = saveArgv[j];
			if (*url == '/') {
				mprSprintf(urlBuf2, sizeof(urlBuf2), "%s%s", host, url);
				url = urlBuf2;
			}
			rc = fetch(client, method, url, postData, postLen);
			if (rc < 0 && !continueOnErrors) {
				success = 0;
				goto commonExit;
			}
		}
		if (fileList == 0) {
			continue;
		}
		fp = fopen(fileList, "rt");
		if (fp == 0) {
			mprError(MPR_L, MPR_USER, "Can't open %s", fileList);
			goto commonExit;
		}

		line = 0;
		while (fgets(urlBuf, sizeof(urlBuf), fp) != NULL && !mpr->isExiting()) {
			lock();
			if (fetchCount >= iterations) {
				unlock();
				break;
			}
			unlock();
			if ((cp = strchr(urlBuf, '\n')) != 0) {
				*cp = '\0';
			}
			operation = mprStrTok(urlBuf, " \t\n", &tok);
			url = mprStrTok(0, " \t\n", &tok);
	
			if (*url == '/') {
				mprSprintf(urlBuf2, sizeof(urlBuf2), "%s%s", host, url);
				url = urlBuf2;
			}

			if (strcmp(operation, "GET") == 0) {
				rc = fetch(client, operation, url, postData, postLen);

			} else if (strcmp(operation, "HEAD") == 0) {
				rc = fetch(client, operation, url, postData, postLen);

			} else if (strcmp(operation, "POST") == 0) {
				while (fgets(postBuf, sizeof(postBuf), fp) != NULL) {
					if (postBuf[0] != '\t') {
						break;
					}
					if (strlen(postBuf) == 1) {
						break;
					}
					post.put((uchar*) &postBuf[1], strlen(postBuf) - 2);
					post.addNull();
					if (post.getLength() >= (MPR_HTTP_MAX_BODY - 1)) {
						mprError(MPR_L, MPR_USER, "Bad post data on line %d",
							line);
						if (!continueOnErrors) {
							success = 0;
							fclose(fp);
							fp = 0;
							goto commonExit;
						}
					}
				}
				if (post.getLength() > 0) {
					rc = fetch(client, operation, url, post.getStart(), 
						post.getLength());
				} else {
					rc = fetch(client, operation, url, postBuf, postLen);
				}
				post.flush();

			} else {
				rc = -1;
				mprError(MPR_L, MPR_USER, "Bad operation on line %d", line);
			}
			if (rc < 0 && !continueOnErrors) {
				success = 0;
				fclose(fp);
				fp = 0;
				goto commonExit;
			}
		}
		fclose(fp);
		fp = 0;
	}

commonExit:

#if BLD_FEATURE_MULTITHREAD
	if (threadp) {
		lock();
		activeLoadThreads--;
		unlock();
	}
#endif

	delete client;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Issue the HTTP request
//

static int fetch(MaClient *client, char *method, char *url, char *data, int len)
{
	struct stat	sbuf;
	FILE		*fp;
	MaUrl		*parsedUrl;
	char		*header, *content, *diffBuf, *msg;
	char		path[MPR_MAX_PATH], dir[MPR_MAX_PATH], urlBuf[MPR_HTTP_MAX_URL];
	char		tmp[MPR_MAX_PATH];
	int			i, code, rc, contentLen, mark, elapsed, c;

	fp = 0;

	lock();
	fetchCount++;
	unlock();

	if (url == 0) {
		return MPR_ERR_BAD_ARGS;
	}
	if (*url == '/') {
		mprSprintf(urlBuf, sizeof(urlBuf), "http://127.0.0.1%s", url);
		url = urlBuf;
	} else if ((strstr(url, "http://")) == 0) {
		mprSprintf(urlBuf, sizeof(urlBuf), "http://%s", url);
		url = urlBuf;
	}
	mprLog(MPR_DEBUG, tMod, "fetch: %s %s\n", method, url);
	mark = mprGetTime(0);

	if (mprStrCmpAnyCase(method, "GET") == 0) {
		rc = client->getRequest(url);

	} else if (mprStrCmpAnyCase(method, "HEAD") == 0) {
		rc = client->headRequest(url);

	} else if (mprStrCmpAnyCase(method, "OPTIONS") == 0) {
		rc = client->optionsRequest(url);

	} else if (mprStrCmpAnyCase(method, "POST") == 0) {
		rc = client->postRequest(url, data, len);

	} else if (mprStrCmpAnyCase(method, "TRACE") == 0) {
		rc = client->traceRequest(url);
	}

	if (rc < 0) {
		return MPR_ERR_CANT_OPEN;
	}

	code = client->getResponseCode();
	content = client->getResponseContent(&contentLen);
	header = client->getResponseHeader();
	msg = client->getResponseMessage();

	elapsed = mprGetTime(0) - mark;

	if (code == 200 || code == 302) {
		mprLog(6, tMod, "Response code %d, content len %d, header: \n%s\n", 
			code, contentLen, header);
	} else {
		mprLog(2, tMod, "Response code %d, content len %d, header: \n%s\n%s", 
			code, contentLen, header, content);
		mprError(MPR_L, MPR_USER, "Can't retrieve \"%s\" (%d), %s",
			url, code, msg);
		return MPR_ERR_CANT_READ;
	}

	if (cmpDir) {
		client->getParsedUrl(&parsedUrl);
		mprSprintf(path, sizeof(path), "%s%s", cmpDir, parsedUrl->uri);
		if (path[strlen(path) - 1] == '/') {
			path[strlen(path) - 1] = '\0';
		}
		if (stat(path, &sbuf) < 0) {
			mprError(MPR_L, MPR_USER, "Can't access %s", path);
			return MPR_ERR_CANT_ACCESS;
		}
		if (sbuf.st_mode & S_IFDIR) {
			strcpy(tmp, path);
			mprSprintf(path, sizeof(path), "%s/_DEFAULT_.html", tmp);
		}
		if (stat(path, &sbuf) < 0) {
			mprError(MPR_L, MPR_USER, "Can't access %s", path);
			return MPR_ERR_CANT_ACCESS;
		}
		if ((int) sbuf.st_size != contentLen) {
			mprError(MPR_L, MPR_USER, "Failed comparison for %s"
				"ContentLen %d, size %d\n", url, contentLen, sbuf.st_size);
			return MPR_ERR_CANT_ACCESS;
		}
		if ((fp = fopen(path, "r" MPR_BINARY)) == 0) {
			mprError(MPR_L, MPR_USER, "Can't open %s", path);
			return MPR_ERR_CANT_OPEN;
		}
		diffBuf = (char*) mprMalloc(contentLen);
		if ((int) fread(diffBuf, 1, contentLen, fp) != contentLen) {
			mprError(MPR_L, MPR_USER, "Can't read content from %s", path);
			return MPR_ERR_CANT_READ;
		}
		for (i = 0; i < contentLen; i++) {
			if (diffBuf[i] != content[i]) {
				mprError(MPR_L, MPR_USER, "Failed comparison for %s"
					"At byte %d: %x vs %x\n", i, 
					(uchar) diffBuf[i], (uchar) content[i]);
				return MPR_ERR_GENERAL;
			}
		}
		fclose(fp);
		mprFree(diffBuf);
	}

	if (writeDir) {
		client->getParsedUrl(&parsedUrl);
		mprSprintf(path, sizeof(path), "%s%s", writeDir, parsedUrl->uri);
		if (path[strlen(path) - 1] == '/') {
			path[strlen(path) - 1] = '\0';
		}
		mprGetDirName(dir, sizeof(dir), path);
		mprMakeDir(dir);
		if (stat(path, &sbuf) == 0 && sbuf.st_mode & S_IFDIR) {
			strcpy(tmp, path);
			mprSprintf(path, sizeof(path), "%s/_DEFAULT_.html", tmp);
		}

		if ((fp = fopen(path, "w" MPR_BINARY)) == 0) {
			mprError(MPR_L, MPR_USER, "Can't open %s", path);
			return MPR_ERR_CANT_OPEN;
		}
		
		if (outputHeader) {
			if (fputs(header, fp) != EOF) {
				mprError(MPR_L, MPR_USER, "Can't write header to %s", path);
				return MPR_ERR_CANT_WRITE;
			}
			fputc('\n', fp);
		}
		if ((int) fwrite(content, 1, contentLen, fp) != contentLen) {
			mprError(MPR_L, MPR_USER, "Can't write content to %s", path);
			return MPR_ERR_CANT_WRITE;
		}
		fclose(fp);
	}

	lock();
	if (trace) {
		if (strstr(url, "http://") != 0) {
			url += 7;
		}
		if ((fetchCount % 100) == 1) {
			if (fetchCount == 1 || (fetchCount % 2500) == 1) {
				if (fetchCount > 1) {
					mprPrintf("\n");
				}
				mprPrintf(
					"   Count   Fd  Thread   Op  Code   Bytes  Time Url\n");
			}
			mprPrintf("%8d %4d %7s %4s %5d %7d %5.2f %s\n", fetchCount - 1,
				client->getFd(), 
#if BLD_FEATURE_MULTITHREAD
				mprGetCurrentThreadName(),
#else
				"",
#endif
				method, code, contentLen, elapsed / 1000.0, url);
		}
	}
	if (outputHeader) {
		mprPrintf("%s\n", header);
	}
	if (!quietMode) {
		for (i = 0; i < contentLen; i++) {
			if (!isprint(content[i]) && content[i] != '\n' && 
					content[i] != '\r' && content[i] != '\t') {
				break;
			}
		}
		if (contentLen > 0) {
			if (i != contentLen && 0) {
				mprPrintf("Content has non-printable data\n");
			} else {
				// mprPrintf("Length of content %d\n", contentLen);
				for (i = 0; i < contentLen; i++) {
					c = (uchar) content[i];
					if (isprint(c) || isspace(c)) {
						putchar(content[i]);
					} else {
						mprPrintf("0x%x", c);
					}
				}
				fflush(stdout);
			}
		}
	}
	unlock();

	if (singleStep) {
		mprPrintf("Pause: ");
		read(0, (char*) &rc, 1);
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MULTITHREAD
static void lock()
{
	mutex->lock();
}
	
////////////////////////////////////////////////////////////////////////////////

static void unlock()
{
	mutex->unlock();
}
#endif

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
