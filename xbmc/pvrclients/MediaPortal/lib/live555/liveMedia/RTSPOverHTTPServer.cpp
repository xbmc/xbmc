/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// A simple HTTP server that acts solely to implement RTSP-over-HTTP tunneling
// (to a separate RTSP server), as described in
// http://developer.apple.com/documentation/QuickTime/QTSS/Concepts/chapter_2_section_14.html
// Implementation

#include "RTSPOverHTTPServer.hh"
#include "RTSPCommon.hh"
#include <GroupsockHelper.hh>

#include <string.h>
#if defined(__WIN32__) || defined(_WIN32) || defined(_QNX4)
#define snprintf _snprintf
#else
#include <signal.h>
#define USE_SIGNALS 1
#endif


#define DEBUG 1 //#####@@@@@
///////// RTSPOverHTTPServer implementation //////////

#define HTTP_PARAM_STRING_MAX 100

RTSPOverHTTPServer*
RTSPOverHTTPServer::createNew(UsageEnvironment& env, Port ourHTTPPort,
			      Port rtspServerPort, char const* rtspServerHostName) {
  int ourSocket = -1;

  do {
    ourSocket = setUpOurSocket(env, ourHTTPPort);
    if (ourSocket == -1) break;

    return new RTSPOverHTTPServer(env, ourSocket, rtspServerPort, rtspServerHostName);
  } while (0);

  if (ourSocket != -1) ::closeSocket(ourSocket);
  return NULL;
}

#define LISTEN_BACKLOG_SIZE 20

int RTSPOverHTTPServer::setUpOurSocket(UsageEnvironment& env, Port& ourPort) {
  int ourSocket = -1;

  do {
    NoReuse dummy; // Don't use this socket if there's already a local server using it

    ourSocket = setupStreamSocket(env, ourPort);
    if (ourSocket < 0) break;

    // Make sure we have a big send buffer:
    if (!increaseSendBufferTo(env, ourSocket, 50*1024)) break;

    // Allow multiple simultaneous connections:
    if (listen(ourSocket, LISTEN_BACKLOG_SIZE) < 0) {
      env.setResultErrMsg("listen() failed: ");
      break;
    }

    if (ourPort.num() == 0) {
      // bind() will have chosen a port for us; return it also:
      if (!getSourcePort(env, ourSocket, ourPort)) break;
    }

    return ourSocket;
  } while (0);

  if (ourSocket != -1) ::closeSocket(ourSocket);
  return -1;
}

RTSPOverHTTPServer
::RTSPOverHTTPServer(UsageEnvironment& env, int ourSocket,
		     Port rtspServerPort, char const* rtspServerHostName)
  : Medium(env),
    fServerSocket(ourSocket),
    fRTSPServerPort(rtspServerPort), fRTSPServerHostName(strDup(rtspServerHostName)) {
#ifdef USE_SIGNALS
  // Ignore the SIGPIPE signal, so that clients on the same host that are killed
  // don't also kill us:
  signal(SIGPIPE, SIG_IGN);
#endif

  // Arrange to handle connections from others:
  env.taskScheduler().turnOnBackgroundReadHandling(fServerSocket,
	   (TaskScheduler::BackgroundHandlerProc*)&incomingConnectionHandler,
						   this);
}

RTSPOverHTTPServer::~RTSPOverHTTPServer() {
  delete[] fRTSPServerHostName;
}

void RTSPOverHTTPServer::incomingConnectionHandler(void* instance, int /*mask*/) {
  RTSPOverHTTPServer* server = (RTSPOverHTTPServer*)instance;
  server->incomingConnectionHandler1();
}

void RTSPOverHTTPServer::incomingConnectionHandler1() {
  struct sockaddr_in clientAddr;
  SOCKLEN_T clientAddrLen = sizeof clientAddr;
  int clientSocket = accept(fServerSocket, (struct sockaddr*)&clientAddr,
                            &clientAddrLen);
  if (clientSocket < 0) {
    int err = envir().getErrno();
    if (err != EWOULDBLOCK) {
      envir().setResultErrMsg("accept() failed: ");
    }
    return;
  }
  makeSocketNonBlocking(clientSocket);
  increaseSendBufferTo(envir(), clientSocket, 50*1024);
#if defined(DEBUG) || defined(DEBUG_CONNECTIONS)
  fprintf(stderr, "accept()ed connection from %s\n", our_inet_ntoa(clientAddr.sin_addr));
#endif

  // Create a new object for handling this HTTP connection:
  new HTTPClientConnection(*this, clientSocket);
}


////////// HTTPClientConnection implementation /////////

RTSPOverHTTPServer::HTTPClientConnection
::HTTPClientConnection(RTSPOverHTTPServer& ourServer, int clientSocket)
  : fOurServer(ourServer), fClientSocket(clientSocket), fSessionIsActive(True) {
  // Arrange to handle incoming requests:
  resetRequestBuffer();
  envir().taskScheduler().turnOnBackgroundReadHandling(fClientSocket,
	       (TaskScheduler::BackgroundHandlerProc*)&incomingRequestHandler, this);
}

RTSPOverHTTPServer::HTTPClientConnection
::~HTTPClientConnection() {
  // Turn off background read handling:
  envir().taskScheduler().turnOffBackgroundReadHandling(fClientSocket);

  ::closeSocket(fClientSocket);
}

void RTSPOverHTTPServer::HTTPClientConnection
::incomingRequestHandler(void* instance, int /*mask*/) {
  HTTPClientConnection* connection = (HTTPClientConnection*)instance;
  connection->incomingRequestHandler1();
}

void RTSPOverHTTPServer::HTTPClientConnection::incomingRequestHandler1() {
  struct sockaddr_in dummy; // 'from' address, meaningless in this case
  Boolean endOfMsg = False;
  unsigned char* ptr = &fRequestBuffer[fRequestBytesAlreadySeen];

  int bytesRead = readSocket(envir(), fClientSocket,
                             ptr, fRequestBufferBytesLeft, dummy);
  if (bytesRead <= 0 || (unsigned)bytesRead >= fRequestBufferBytesLeft) {
    // Either the client socket has died, or the request was too big for us.
    // Terminate this connection:
#ifdef DEBUG
    fprintf(stderr, "HTTPClientConnection[%p]::incomingRequestHandler1() read %d bytes (of %d); terminating connection!\n", this, bytesRead, fRequestBufferBytesLeft);
#endif
    delete this;
    return;
  }
#ifdef DEBUG
  ptr[bytesRead] = '\0';
  fprintf(stderr, "HTTPClientConnection[%p]::incomingRequestHandler1() read %d bytes:%s\n",
	  this, bytesRead, ptr);
#endif

  // Look for the end of the message: <CR><LF><CR><LF>
  unsigned char *tmpPtr = ptr;
  if (fRequestBytesAlreadySeen > 0) --tmpPtr;
  // in case the last read ended with a <CR>
  while (tmpPtr < &ptr[bytesRead-1]) {
    if (*tmpPtr == '\r' && *(tmpPtr+1) == '\n') {
      if (tmpPtr - fLastCRLF == 2) { // This is it:
        endOfMsg = 1;
        break;
      }
      fLastCRLF = tmpPtr;
    }
    ++tmpPtr;
  }

  fRequestBufferBytesLeft -= bytesRead;
  fRequestBytesAlreadySeen += bytesRead;

  if (!endOfMsg) return; // subsequent reads will be needed to complete the request

  // Parse the request string to get the (few) parameters that we care about,
  // then handle the command:
  fRequestBuffer[fRequestBytesAlreadySeen] = '\0';
  char cmdName[HTTP_PARAM_STRING_MAX];
  char sessionCookie[HTTP_PARAM_STRING_MAX];
  char acceptStr[HTTP_PARAM_STRING_MAX];
  char contentTypeStr[HTTP_PARAM_STRING_MAX];
  if (!parseHTTPRequestString(cmdName, sizeof cmdName,
			  sessionCookie, sizeof sessionCookie,
			  acceptStr, sizeof acceptStr,
			  contentTypeStr, sizeof contentTypeStr)) {
#ifdef DEBUG
    fprintf(stderr, "parseHTTPRTSPRequestString() failed!\n");
#endif
    handleCmd_bad();
  } else {
#ifdef DEBUG
    fprintf(stderr, "parseHTTPRTSPRequestString() returned cmdName \"%s\", sessionCookie \"%s\", acceptStr \"%s\", contentTypeStr \"%s\"\n", cmdName, sessionCookie, acceptStr, contentTypeStr);
#endif
#if 0
    if (strcmp(cmdName, "OPTIONS") == 0) {
      handleCmd_OPTIONS(cseq);
    } else if (strcmp(cmdName, "DESCRIBE") == 0) {
      handleCmd_DESCRIBE(cseq, urlSuffix, (char const*)fRequestBuffer);
    } else if (strcmp(cmdName, "SETUP") == 0) {
      handleCmd_SETUP(cseq, urlPreSuffix, urlSuffix, (char const*)fRequestBuffer);
    } else if (strcmp(cmdName, "TEARDOWN") == 0
               || strcmp(cmdName, "PLAY") == 0
               || strcmp(cmdName, "PAUSE") == 0
               || strcmp(cmdName, "GET_PARAMETER") == 0
               || strcmp(cmdName, "SET_PARAMETER") == 0) {
      handleCmd_withinSession(cmdName, urlPreSuffix, urlSuffix, cseq,
                              (char const*)fRequestBuffer);
    } else {
      handleCmd_notSupported(cseq);
    }
#endif
  }

#ifdef DEBUG
  fprintf(stderr, "sending response: %s", fResponseBuffer);
#endif
  send(fClientSocket, (char const*)fResponseBuffer, strlen((char*)fResponseBuffer), 0);

  resetRequestBuffer(); // to prepare for any subsequent request
  if (!fSessionIsActive) delete this;
}

void RTSPOverHTTPServer::HTTPClientConnection::resetRequestBuffer() {
  fRequestBytesAlreadySeen = 0;
  fRequestBufferBytesLeft = sizeof fRequestBuffer;
  fLastCRLF = &fRequestBuffer[-3]; // hack
}

Boolean RTSPOverHTTPServer::HTTPClientConnection::
parseHTTPRequestString(char* resultCmdName,
		   unsigned resultCmdNameMaxSize,
		   char* sessionCookie,
		   unsigned sessionCookieMaxSize,
		   char* acceptStr,
		   unsigned acceptStrMaxSize,
		   char* contentTypeStr,
		   unsigned contentTypeStrMaxSize) {
  return False; //#####@@@@@
#if 0
  // This parser is currently rather dumb; it should be made smarter #####

  // Read everything up to the first space as the command name:
  Boolean parseSucceeded = False;
  unsigned i;
  for (i = 0; i < resultCmdNameMaxSize-1 && i < reqStrSize; ++i) {
    char c = reqStr[i];
    if (c == ' ' || c == '\t') {
      parseSucceeded = True;
      break;
    }

    resultCmdName[i] = c;
  }
  resultCmdName[i] = '\0';
  if (!parseSucceeded) return False;

  // Skip over the prefix of any "rtsp://" or "rtsp:/" URL that follows:
  unsigned j = i+1;
  while (j < reqStrSize && (reqStr[j] == ' ' || reqStr[j] == '\t')) ++j; // skip over any additional white space
   for (j = i+1; j < reqStrSize-8; ++j) {
     if ((reqStr[j] == 'r' || reqStr[j] == 'R')
	 && (reqStr[j+1] == 't' || reqStr[j+1] == 'T')
	 && (reqStr[j+2] == 's' || reqStr[j+2] == 'S')
	 && (reqStr[j+3] == 'p' || reqStr[j+3] == 'P')
	 && reqStr[j+4] == ':' && reqStr[j+5] == '/') {
       j += 6;
       if (reqStr[j] == '/') {
	 // This is a "rtsp://" URL; skip over the host:port part that follows:
	 ++j;
	 while (j < reqStrSize && reqStr[j] != '/' && reqStr[j] != ' ') ++j;
       } else {
	 // This is a "rtsp:/" URL; back up to the "/":
	 --j;
       }
       i = j;
       break;
     }
   }

 // Look for the URL suffix (before the following "RTSP/"):
 parseSucceeded = False;
 for (unsigned k = i+1; k < reqStrSize-5; ++k) {
   if (reqStr[k] == 'R' && reqStr[k+1] == 'T' &&
       reqStr[k+2] == 'S' && reqStr[k+3] == 'P' && reqStr[k+4] == '/') {
     while (--k >= i && reqStr[k] == ' ') {} // go back over all spaces before "RTSP/"
      unsigned k1 = k;
      while (k1 > i && reqStr[k1] != '/' && reqStr[k1] != ' ') --k1;
      // the URL suffix comes from [k1+1,k]

      // Copy "resultURLSuffix":
      if (k - k1 + 1 > resultURLSuffixMaxSize) return False; // there's no room
      unsigned n = 0, k2 = k1+1;
      while (k2 <= k) resultURLSuffix[n++] = reqStr[k2++];
      resultURLSuffix[n] = '\0';

      // Also look for the URL 'pre-suffix' before this:
      unsigned k3 = --k1;
      while (k3 > i && reqStr[k3] != '/' && reqStr[k3] != ' ') --k3;
      // the URL pre-suffix comes from [k3+1,k1]

      // Copy "resultURLPreSuffix":
      if (k1 - k3 + 1 > resultURLPreSuffixMaxSize) return False; // there's no room
      n = 0; k2 = k3+1;
      while (k2 <= k1) resultURLPreSuffix[n++] = reqStr[k2++];
      resultURLPreSuffix[n] = '\0';

      i = k + 7; // to go past " RTSP/"
      parseSucceeded = True;
      break;
    }
  }
  if (!parseSucceeded) return False;

  // Look for "CSeq:", skip whitespace,
  // then read everything up to the next \r or \n as 'CSeq':
  parseSucceeded = False;
  for (j = i; j < reqStrSize-5; ++j) {
    if (reqStr[j] == 'C' && reqStr[j+1] == 'S' && reqStr[j+2] == 'e' &&
        reqStr[j+3] == 'q' && reqStr[j+4] == ':') {
      j += 5;
      unsigned n;
      while (j < reqStrSize && (reqStr[j] ==  ' ' || reqStr[j] == '\t')) ++j;
      for (n = 0; n < resultCSeqMaxSize-1 && j < reqStrSize; ++n,++j) {
        char c = reqStr[j];
        if (c == '\r' || c == '\n') {
          parseSucceeded = True;
          break;
        }

        resultCSeq[n] = c;
      }
      resultCSeq[n] = '\0';
      break;
    }
  }
  if (!parseSucceeded) return False;

  return True;
#endif
}

static char const* allowedCommandNames = "GET, PUT";

void RTSPOverHTTPServer::HTTPClientConnection::handleCmd_bad() {
  snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
           "HTTP/1.1 400 Bad Request\r\nAllow: %s\r\n\r\n",
           allowedCommandNames);
}
