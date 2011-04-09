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
// A RTSP server
// Implementation

#include "RTSPServer.hh"
#include "RTSPCommon.hh"
#include <GroupsockHelper.hh>

#if defined(__WIN32__) || defined(_WIN32) || defined(_QNX4)
#else
#include <signal.h>
#define USE_SIGNALS 1
#endif
#include <time.h> // for "strftime()" and "gmtime()"

////////// RTSPServer implementation //////////

RTSPServer*
RTSPServer::createNew(UsageEnvironment& env, Port ourPort,
		      UserAuthenticationDatabase* authDatabase,
		      unsigned reclamationTestSeconds) {
  int ourSocket = -1;

  do {
    ourSocket = setUpOurSocket(env, ourPort);
    if (ourSocket == -1) break;

    return new RTSPServer(env, ourSocket, ourPort, authDatabase,
			  reclamationTestSeconds);
  } while (0);

  if (ourSocket != -1) ::closeSocket(ourSocket);
  return NULL;
}

Boolean RTSPServer::lookupByName(UsageEnvironment& env,
				 char const* name,
				 RTSPServer*& resultServer) {
  resultServer = NULL; // unless we succeed

  Medium* medium;
  if (!Medium::lookupByName(env, name, medium)) return False;

  if (!medium->isRTSPServer()) {
    env.setResultMsg(name, " is not a RTSP server");
    return False;
  }

  resultServer = (RTSPServer*)medium;
  return True;
}

void RTSPServer::addServerMediaSession(ServerMediaSession* serverMediaSession) {
  if (serverMediaSession == NULL) return;

  char const* sessionName = serverMediaSession->streamName();
  if (sessionName == NULL) sessionName = "";
  ServerMediaSession* existingSession
    = (ServerMediaSession*)
    (fServerMediaSessions->Add(sessionName,
			       (void*)serverMediaSession));
  removeServerMediaSession(existingSession); // if any
}

ServerMediaSession* RTSPServer::lookupServerMediaSession(char const* streamName) {
  return (ServerMediaSession*)(fServerMediaSessions->Lookup(streamName));
}

void RTSPServer::removeServerMediaSession(ServerMediaSession* serverMediaSession) {
  if (serverMediaSession == NULL) return;

  fServerMediaSessions->Remove(serverMediaSession->streamName());
  if (serverMediaSession->referenceCount() == 0) {
    Medium::close(serverMediaSession);
  } else {
    serverMediaSession->deleteWhenUnreferenced() = True;
  }
}

void RTSPServer::removeServerMediaSession(char const* streamName) {
  removeServerMediaSession(lookupServerMediaSession(streamName));
}

char* RTSPServer::rtspURLPrefix(int clientSocket) const {
  struct sockaddr_in ourAddress;
  if (clientSocket < 0) {
    // Use our default IP address in the URL:
    ourAddress.sin_addr.s_addr = ReceivingInterfaceAddr != 0
      ? ReceivingInterfaceAddr
      : ourIPAddress(envir()); // hack
  } else {
    SOCKLEN_T namelen = sizeof ourAddress;
    getsockname(clientSocket, (struct sockaddr*)&ourAddress, &namelen);
  }

  char urlBuffer[100]; // more than big enough for "rtsp://<ip-address>:<port>/"

  portNumBits portNumHostOrder = ntohs(fServerPort.num());
  if (portNumHostOrder == 554 /* the default port number */) {
    sprintf(urlBuffer, "rtsp://%s/", our_inet_ntoa(ourAddress.sin_addr));
  } else {
    sprintf(urlBuffer, "rtsp://%s:%hu/",
	    our_inet_ntoa(ourAddress.sin_addr), portNumHostOrder);
  }

  return strDup(urlBuffer);
}

char* RTSPServer
::rtspURL(ServerMediaSession const* serverMediaSession, int clientSocket) const {
  char* urlPrefix = rtspURLPrefix(clientSocket);
  char const* sessionName = serverMediaSession->streamName();

  char* resultURL = new char[strlen(urlPrefix) + strlen(sessionName) + 1];
  sprintf(resultURL, "%s%s", urlPrefix, sessionName);

  delete[] urlPrefix;
  return resultURL;
}

#define LISTEN_BACKLOG_SIZE 20

int RTSPServer::setUpOurSocket(UsageEnvironment& env, Port& ourPort) {
  int ourSocket = -1;

  do {
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

Boolean RTSPServer
::specialClientAccessCheck(int /*clientSocket*/, struct sockaddr_in& /*clientAddr*/, char const* /*urlSuffix*/) {
  // default implementation
  return True;
}

RTSPServer::RTSPServer(UsageEnvironment& env,
		       int ourSocket, Port ourPort,
		       UserAuthenticationDatabase* authDatabase,
		       unsigned reclamationTestSeconds)
  : Medium(env),
    fServerSocket(ourSocket), fServerPort(ourPort),
    fAuthDB(authDatabase), fReclamationTestSeconds(reclamationTestSeconds),
    fServerMediaSessions(HashTable::create(STRING_HASH_KEYS)) {
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

RTSPServer::~RTSPServer() {
  // Turn off background read handling:
  envir().taskScheduler().turnOffBackgroundReadHandling(fServerSocket);

  ::closeSocket(fServerSocket);

  // Remove all server media sessions (they'll get deleted when they're finished):
  while (1) {
    ServerMediaSession* serverMediaSession
      = (ServerMediaSession*)fServerMediaSessions->RemoveNext();
    if (serverMediaSession == NULL) break;
    removeServerMediaSession(serverMediaSession);
  }

  // Finally, delete the session table itself:
  delete fServerMediaSessions;
}

Boolean RTSPServer::isRTSPServer() const {
  return True;
}

void RTSPServer::incomingConnectionHandler(void* instance, int /*mask*/) {
  RTSPServer* server = (RTSPServer*)instance;
  server->incomingConnectionHandler1();
}

void RTSPServer::incomingConnectionHandler1() {
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
  envir() << "accept()ed connection from " << our_inet_ntoa(clientAddr.sin_addr) << '\n';
#endif

  // Create a new object for this RTSP session.
  // (Choose a random 32-bit integer for the session id (it will be encoded as a 8-digit hex number).  We don't bother checking for
  //  a collision; the probability of two concurrent sessions getting the same session id is very low.)
  unsigned sessionId = (unsigned)our_random();
  (void)createNewClientSession(sessionId, clientSocket, clientAddr);
}


////////// RTSPServer::RTSPClientSession implementation //////////

RTSPServer::RTSPClientSession
::RTSPClientSession(RTSPServer& ourServer, unsigned sessionId,
	      int clientSocket, struct sockaddr_in clientAddr)
  : fOurServer(ourServer), fOurSessionId(sessionId),
    fOurServerMediaSession(NULL),
    fClientSocket(clientSocket), fClientAddr(clientAddr),
    fLivenessCheckTask(NULL),
    fIsMulticast(False), fSessionIsActive(True), fStreamAfterSETUP(False),
    fTCPStreamIdCount(0), fNumStreamStates(0), fStreamStates(NULL) {
  // Arrange to handle incoming requests:
  resetRequestBuffer();
  envir().taskScheduler().turnOnBackgroundReadHandling(fClientSocket,
     (TaskScheduler::BackgroundHandlerProc*)&incomingRequestHandler, this);
  noteLiveness();
}

RTSPServer::RTSPClientSession::~RTSPClientSession() {
  // Turn off any liveness checking:
  envir().taskScheduler().unscheduleDelayedTask(fLivenessCheckTask);

  // Turn off background read handling:
  envir().taskScheduler().turnOffBackgroundReadHandling(fClientSocket);

  ::closeSocket(fClientSocket);

  reclaimStreamStates();

  if (fOurServerMediaSession != NULL) {
    fOurServerMediaSession->decrementReferenceCount();
    if (fOurServerMediaSession->referenceCount() == 0
	&& fOurServerMediaSession->deleteWhenUnreferenced()) {
      fOurServer.removeServerMediaSession(fOurServerMediaSession);
    }
  }
}

void RTSPServer::RTSPClientSession::reclaimStreamStates() {
  for (unsigned i = 0; i < fNumStreamStates; ++i) {
    if (fStreamStates[i].subsession != NULL) {
      fStreamStates[i].subsession->deleteStream(fOurSessionId,
						fStreamStates[i].streamToken);
    }
  }
  delete[] fStreamStates; fStreamStates = NULL;
  fNumStreamStates = 0;
}

void RTSPServer::RTSPClientSession::resetRequestBuffer() {
  fRequestBytesAlreadySeen = 0;
  fRequestBufferBytesLeft = sizeof fRequestBuffer;
  fLastCRLF = &fRequestBuffer[-3]; // hack
}

void RTSPServer::RTSPClientSession::incomingRequestHandler(void* instance, int /*mask*/) {
  RTSPClientSession* session = (RTSPClientSession*)instance;
  session->incomingRequestHandler1();
}

void RTSPServer::RTSPClientSession::incomingRequestHandler1() {
  struct sockaddr_in dummy; // 'from' address, meaningless in this case

  int bytesRead = readSocket(envir(), fClientSocket, &fRequestBuffer[fRequestBytesAlreadySeen], fRequestBufferBytesLeft, dummy);
  handleRequestBytes(bytesRead);
}

void RTSPServer::RTSPClientSession::handleAlternativeRequestByte(void* instance, u_int8_t requestByte) {
  RTSPClientSession* session = (RTSPClientSession*)instance;
  session->handleAlternativeRequestByte1(requestByte);
}

void RTSPServer::RTSPClientSession::handleAlternativeRequestByte1(u_int8_t requestByte) {
  // Add this character to our buffer; then try to handle the data that we have buffered so far:
  if (fRequestBufferBytesLeft == 0) return;
  fRequestBuffer[fRequestBytesAlreadySeen] = requestByte;
  handleRequestBytes(1);
}

void RTSPServer::RTSPClientSession::handleRequestBytes(int newBytesRead) {
  noteLiveness();

  if (newBytesRead <= 0 || (unsigned)newBytesRead >= fRequestBufferBytesLeft) {
    // Either the client socket has died, or the request was too big for us.
    // Terminate this connection:
#ifdef DEBUG
    fprintf(stderr, "RTSPClientSession[%p]::handleRequestBytes() read %d new bytes (of %d); terminating connection!\n", this, newBytesRead, fRequestBufferBytesLeft);
#endif
    delete this;
    return;
  }

  Boolean endOfMsg = False;
  unsigned char* ptr = &fRequestBuffer[fRequestBytesAlreadySeen];
#ifdef DEBUG
  ptr[newBytesRead] = '\0';
  fprintf(stderr, "RTSPClientSession[%p]::handleRequestBytes() read %d new bytes:%s\n", this, newBytesRead, ptr);
#endif

  // Look for the end of the message: <CR><LF><CR><LF>
  unsigned char *tmpPtr = ptr;
  if (fRequestBytesAlreadySeen > 0) --tmpPtr;
      // in case the last read ended with a <CR>
  while (tmpPtr < &ptr[newBytesRead-1]) {
    if (*tmpPtr == '\r' && *(tmpPtr+1) == '\n') {
      if (tmpPtr - fLastCRLF == 2) { // This is it:
	endOfMsg = True;
	break;
      }
      fLastCRLF = tmpPtr;
    }
    ++tmpPtr;
  }

  fRequestBufferBytesLeft -= newBytesRead;
  fRequestBytesAlreadySeen += newBytesRead;

  if (!endOfMsg) return; // subsequent reads will be needed to complete the request

  // Parse the request string into command name and 'CSeq',
  // then handle the command:
  fRequestBuffer[fRequestBytesAlreadySeen] = '\0';
  char cmdName[RTSP_PARAM_STRING_MAX];
  char urlPreSuffix[RTSP_PARAM_STRING_MAX];
  char urlSuffix[RTSP_PARAM_STRING_MAX];
  char cseq[RTSP_PARAM_STRING_MAX];
  if (!parseRTSPRequestString((char*)fRequestBuffer, fRequestBytesAlreadySeen,
			      cmdName, sizeof cmdName,
			      urlPreSuffix, sizeof urlPreSuffix,
			      urlSuffix, sizeof urlSuffix,
			      cseq, sizeof cseq)) {
#ifdef DEBUG
    fprintf(stderr, "parseRTSPRequestString() failed!\n");
#endif
    handleCmd_bad(cseq);
  } else {
#ifdef DEBUG
    fprintf(stderr, "parseRTSPRequestString() returned cmdName \"%s\", urlPreSuffix \"%s\", urlSuffix \"%s\"\n", cmdName, urlPreSuffix, urlSuffix);
#endif
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
  }

#ifdef DEBUG
  fprintf(stderr, "sending response: %s", fResponseBuffer);
#endif
  send(fClientSocket, (char const*)fResponseBuffer, strlen((char*)fResponseBuffer), 0);

  if (strcmp(cmdName, "SETUP") == 0 && fStreamAfterSETUP) {
    // The client has asked for streaming to commence now, rather than after a
    // subsequent "PLAY" command.  So, simulate the effect of a "PLAY" command:
    handleCmd_withinSession("PLAY", urlPreSuffix, urlSuffix, cseq,
			    (char const*)fRequestBuffer);
  }

  resetRequestBuffer(); // to prepare for any subsequent request
  if (!fSessionIsActive) delete this;
}

// Handler routines for specific RTSP commands:

// Generate a "Date:" header for use in a RTSP response:
static char const* dateHeader() {
  static char buf[200];
#if !defined(_WIN32_WCE)
  time_t tt = time(NULL);
  strftime(buf, sizeof buf, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
#else
  // WinCE apparently doesn't have "time()", "strftime()", or "gmtime()",
  // so generate the "Date:" header a different, WinCE-specific way.
  // (Thanks to Pierre l'Hussiez for this code)
  SYSTEMTIME SystemTime;
  GetSystemTime(&SystemTime);
  WCHAR dateFormat[] = L"ddd, MMM dd yyyy";
  WCHAR timeFormat[] = L"HH:mm:ss GMT\r\n";
  WCHAR inBuf[200];
  DWORD locale = LOCALE_NEUTRAL;

  int ret = GetDateFormat(locale, 0, &SystemTime,
			  (LPTSTR)dateFormat, (LPTSTR)inBuf, sizeof inBuf);
  inBuf[ret - 1] = ' ';
  ret = GetTimeFormat(locale, 0, &SystemTime,
		      (LPTSTR)timeFormat,
		      (LPTSTR)inBuf + ret, (sizeof inBuf) - ret);
  wcstombs(buf, inBuf, wcslen(inBuf));
#endif
  return buf;
}

static char const* allowedCommandNames
= "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER";

void RTSPServer::RTSPClientSession::handleCmd_bad(char const* /*cseq*/) {
  // Don't do anything with "cseq", because it might be nonsense
  snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	   "RTSP/1.0 400 Bad Request\r\n%sAllow: %s\r\n\r\n",
	   dateHeader(), allowedCommandNames);
}

void RTSPServer::RTSPClientSession::handleCmd_notSupported(char const* cseq) {
  snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	   "RTSP/1.0 405 Method Not Allowed\r\nCSeq: %s\r\n%sAllow: %s\r\n\r\n",
	   cseq, dateHeader(), allowedCommandNames);
}

void RTSPServer::RTSPClientSession::handleCmd_notFound(char const* cseq) {
  snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	   "RTSP/1.0 404 Stream Not Found\r\nCSeq: %s\r\n%s\r\n",
	   cseq, dateHeader());
  fSessionIsActive = False; // triggers deletion of ourself after responding
}

void RTSPServer::RTSPClientSession::handleCmd_unsupportedTransport(char const* cseq) {
  snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	   "RTSP/1.0 461 Unsupported Transport\r\nCSeq: %s\r\n%s\r\n",
	   cseq, dateHeader());
  fSessionIsActive = False; // triggers deletion of ourself after responding
}

void RTSPServer::RTSPClientSession::handleCmd_OPTIONS(char const* cseq) {
  snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	   "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sPublic: %s\r\n\r\n",
	   cseq, dateHeader(), allowedCommandNames);
}

void RTSPServer::RTSPClientSession
::handleCmd_DESCRIBE(char const* cseq, char const* urlSuffix,
		     char const* fullRequestStr) {
  char* sdpDescription = NULL;
  char* rtspURL = NULL;
  do {
      if (!authenticationOK("DESCRIBE", cseq, urlSuffix, fullRequestStr))
          break;

    // We should really check that the request contains an "Accept:" #####
    // for "application/sdp", because that's what we're sending back #####

    // Begin by looking up the "ServerMediaSession" object for the
    // specified "urlSuffix":
    ServerMediaSession* session = fOurServer.lookupServerMediaSession(urlSuffix);
    if (session == NULL) {
      handleCmd_notFound(cseq);
      break;
    }

    // Then, assemble a SDP description for this session:
    sdpDescription = session->generateSDPDescription();
    if (sdpDescription == NULL) {
      // This usually means that a file name that was specified for a
      // "ServerMediaSubsession" does not exist.
      snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	       "RTSP/1.0 404 File Not Found, Or In Incorrect Format\r\n"
	       "CSeq: %s\r\n"
	       "%s\r\n",
	       cseq,
	       dateHeader());
     break;
    }
    unsigned sdpDescriptionSize = strlen(sdpDescription);

    // Also, generate our RTSP URL, for the "Content-Base:" header
    // (which is necessary to ensure that the correct URL gets used in
    // subsequent "SETUP" requests).
    rtspURL = fOurServer.rtspURL(session, fClientSocket);

    snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	     "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
	     "%s"
	     "Content-Base: %s/\r\n"
	     "Content-Type: application/sdp\r\n"
	     "Content-Length: %d\r\n\r\n"
	     "%s",
	     cseq,
	     dateHeader(),
	     rtspURL,
	     sdpDescriptionSize,
	     sdpDescription);
  } while (0);

  delete[] sdpDescription;
  delete[] rtspURL;
}

typedef enum StreamingMode {
  RTP_UDP,
  RTP_TCP,
  RAW_UDP
} StreamingMode;

static void parseTransportHeader(char const* buf,
				 StreamingMode& streamingMode,
				 char*& streamingModeString,
				 char*& destinationAddressStr,
				 u_int8_t& destinationTTL,
				 portNumBits& clientRTPPortNum, // if UDP
				 portNumBits& clientRTCPPortNum, // if UDP
				 unsigned char& rtpChannelId, // if TCP
				 unsigned char& rtcpChannelId // if TCP
				 ) {
  // Initialize the result parameters to default values:
  streamingMode = RTP_UDP;
  streamingModeString = NULL;
  destinationAddressStr = NULL;
  destinationTTL = 255;
  clientRTPPortNum = 0;
  clientRTCPPortNum = 1;
  rtpChannelId = rtcpChannelId = 0xFF;

  portNumBits p1, p2;
  unsigned ttl, rtpCid, rtcpCid;

  // First, find "Transport:"
  while (1) {
    if (*buf == '\0') return; // not found
    if (_strncasecmp(buf, "Transport: ", 11) == 0) break;
    ++buf;
  }

  // Then, run through each of the fields, looking for ones we handle:
  char const* fields = buf + 11;
  char* field = strDupSize(fields);
  while (sscanf(fields, "%[^;]", field) == 1) {
    if (strcmp(field, "RTP/AVP/TCP") == 0) {
      streamingMode = RTP_TCP;
    } else if (strcmp(field, "RAW/RAW/UDP") == 0 ||
	       strcmp(field, "MP2T/H2221/UDP") == 0) {
      streamingMode = RAW_UDP;
      streamingModeString = strDup(field);
    } else if (_strncasecmp(field, "destination=", 12) == 0) {
      delete[] destinationAddressStr;
      destinationAddressStr = strDup(field+12);
    } else if (sscanf(field, "ttl%u", &ttl) == 1) {
      destinationTTL = (u_int8_t)ttl;
    } else if (sscanf(field, "client_port=%hu-%hu", &p1, &p2) == 2) {
	clientRTPPortNum = p1;
	clientRTCPPortNum = p2;
    } else if (sscanf(field, "client_port=%hu", &p1) == 1) {
	clientRTPPortNum = p1;
	clientRTCPPortNum = streamingMode == RAW_UDP ? 0 : p1 + 1;
    } else if (sscanf(field, "interleaved=%u-%u", &rtpCid, &rtcpCid) == 2) {
      rtpChannelId = (unsigned char)rtpCid;
      rtcpChannelId = (unsigned char)rtcpCid;
    }

    fields += strlen(field);
    while (*fields == ';') ++fields; // skip over separating ';' chars
    if (*fields == '\0' || *fields == '\r' || *fields == '\n') break;
  }
  delete[] field;
}

static Boolean parsePlayNowHeader(char const* buf) {
  // Find "x-playNow:" header, if present
  while (1) {
    if (*buf == '\0') return False; // not found
    if (_strncasecmp(buf, "x-playNow:", 10) == 0) break;
    ++buf;
  }

  return True;
}

void RTSPServer::RTSPClientSession
::handleCmd_SETUP(char const* cseq,
		  char const* urlPreSuffix, char const* urlSuffix,
		  char const* fullRequestStr) {
  // "urlPreSuffix" should be the session (stream) name, and
  // "urlSuffix" should be the subsession (track) name.
  char const* streamName = urlPreSuffix;
  char const* trackId = urlSuffix;

  // Check whether we have existing session state, and, if so, whether it's
  // for the session that's named in "streamName".  (Note that we don't
  // support more than one concurrent session on the same client connection.) #####
  if (fOurServerMediaSession != NULL
      && strcmp(streamName, fOurServerMediaSession->streamName()) != 0) {
    fOurServerMediaSession = NULL;
  }
  if (fOurServerMediaSession == NULL) {
    // Set up this session's state.

    // Look up the "ServerMediaSession" object for the specified stream:
    if (streamName[0] != '\0' ||
	fOurServer.lookupServerMediaSession("") != NULL) { // normal case
    } else { // weird case: there was no track id in the URL
      streamName = urlSuffix;
      trackId = NULL;
    }
    fOurServerMediaSession = fOurServer.lookupServerMediaSession(streamName);
    if (fOurServerMediaSession == NULL) {
      handleCmd_notFound(cseq);
      return;
    }

    fOurServerMediaSession->incrementReferenceCount();

    // Set up our array of states for this session's subsessions (tracks):
    reclaimStreamStates();
    ServerMediaSubsessionIterator iter(*fOurServerMediaSession);
    for (fNumStreamStates = 0; iter.next() != NULL; ++fNumStreamStates) {}
    fStreamStates = new struct streamState[fNumStreamStates];
    iter.reset();
    ServerMediaSubsession* subsession;
    for (unsigned i = 0; i < fNumStreamStates; ++i) {
      subsession = iter.next();
      fStreamStates[i].subsession = subsession;
      fStreamStates[i].streamToken = NULL; // for now; reset by SETUP later
    }
  }

  // Look up information for the specified subsession (track):
  ServerMediaSubsession* subsession = NULL;
  unsigned streamNum;
  if (trackId != NULL && trackId[0] != '\0') { // normal case
    for (streamNum = 0; streamNum < fNumStreamStates; ++streamNum) {
      subsession = fStreamStates[streamNum].subsession;
      if (subsession != NULL && strcmp(trackId, subsession->trackId()) == 0) break;
    }
    if (streamNum >= fNumStreamStates) {
      // The specified track id doesn't exist, so this request fails:
      handleCmd_notFound(cseq);
      return;
    }
  } else {
    // Weird case: there was no track id in the URL.
    // This works only if we have only one subsession:
    if (fNumStreamStates != 1) {
      handleCmd_bad(cseq);
      return;
    }
    streamNum = 0;
    subsession = fStreamStates[streamNum].subsession;
  }
  // ASSERT: subsession != NULL

  // Look for a "Transport:" header in the request string,
  // to extract client parameters:
  StreamingMode streamingMode;
  char* streamingModeString = NULL; // set when RAW_UDP streaming is specified
  char* clientsDestinationAddressStr;
  u_int8_t clientsDestinationTTL;
  portNumBits clientRTPPortNum, clientRTCPPortNum;
  unsigned char rtpChannelId, rtcpChannelId;
  parseTransportHeader(fullRequestStr, streamingMode, streamingModeString,
		       clientsDestinationAddressStr, clientsDestinationTTL,
		       clientRTPPortNum, clientRTCPPortNum,
		       rtpChannelId, rtcpChannelId);
  if (streamingMode == RTP_TCP && rtpChannelId == 0xFF) {
    // TCP streaming was requested, but with no "interleaving=" fields.
    // (QuickTime Player sometimes does this.)  Set the RTP and RTCP channel ids to
    // proper values:
    rtpChannelId = fTCPStreamIdCount; rtcpChannelId = fTCPStreamIdCount+1;
  }
  fTCPStreamIdCount += 2;

  Port clientRTPPort(clientRTPPortNum);
  Port clientRTCPPort(clientRTCPPortNum);

  // Next, check whether a "Range:" header is present in the request.
  // This isn't legal, but some clients do this to combine "SETUP" and "PLAY":
  double rangeStart = 0.0, rangeEnd = 0.0;
  fStreamAfterSETUP = parseRangeHeader(fullRequestStr, rangeStart, rangeEnd) ||
                      parsePlayNowHeader(fullRequestStr);

  // Then, get server parameters from the 'subsession':
  int tcpSocketNum = streamingMode == RTP_TCP ? fClientSocket : -1;
  netAddressBits destinationAddress = 0;
  u_int8_t destinationTTL = 255;
#ifdef RTSP_ALLOW_CLIENT_DESTINATION_SETTING
  if (clientsDestinationAddressStr != NULL) {
    // Use the client-provided "destination" address.
    // Note: This potentially allows the server to be used in denial-of-service
    // attacks, so don't enable this code unless you're sure that clients are
    // trusted.
    destinationAddress = our_inet_addr(clientsDestinationAddressStr);
  }
  // Also use the client-provided TTL.
  destinationTTL = clientsDestinationTTL;
#endif
  delete[] clientsDestinationAddressStr;
  Port serverRTPPort(0);
  Port serverRTCPPort(0);
  subsession->getStreamParameters(fOurSessionId, fClientAddr.sin_addr.s_addr,
				  clientRTPPort, clientRTCPPort,
				  tcpSocketNum, rtpChannelId, rtcpChannelId,
				  destinationAddress, destinationTTL, fIsMulticast,
				  serverRTPPort, serverRTCPPort,
				  fStreamStates[streamNum].streamToken);
  struct in_addr destinationAddr; destinationAddr.s_addr = destinationAddress;
  char* destAddrStr = strDup(our_inet_ntoa(destinationAddr));
  struct sockaddr_in sourceAddr; SOCKLEN_T namelen = sizeof sourceAddr;
  getsockname(fClientSocket, (struct sockaddr*)&sourceAddr, &namelen);
  char* sourceAddrStr = strDup(our_inet_ntoa(sourceAddr.sin_addr));
  if (fIsMulticast) {
    switch (streamingMode) {
    case RTP_UDP:
        snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "Transport: RTP/AVP;multicast;destination=%s;source=%s;port=%d-%d;ttl=%d\r\n"
                 "Session: %08X\r\n\r\n",
                 cseq,
                 dateHeader(),
                 destAddrStr, sourceAddrStr, ntohs(serverRTPPort.num()), ntohs(serverRTCPPort.num()), destinationTTL,
                 fOurSessionId);
        break;
    case RTP_TCP:
        // multicast streams can't be sent via TCP
        handleCmd_unsupportedTransport(cseq);
        break;
    case RAW_UDP:
        snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "Transport: %s;multicast;destination=%s;source=%s;port=%d;ttl=%d\r\n"
                 "Session: %08X\r\n\r\n",
                 cseq,
                 dateHeader(),
                 streamingModeString, destAddrStr, sourceAddrStr, ntohs(serverRTPPort.num()), destinationTTL,
                 fOurSessionId);
        break;
    }
  } else {
    switch (streamingMode) {
    case RTP_UDP: {
      snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	       "RTSP/1.0 200 OK\r\n"
	       "CSeq: %s\r\n"
	       "%s"
	       "Transport: RTP/AVP;unicast;destination=%s;source=%s;client_port=%d-%d;server_port=%d-%d\r\n"
	       "Session: %08X\r\n\r\n",
	       cseq,
	       dateHeader(),
	       destAddrStr, sourceAddrStr, ntohs(clientRTPPort.num()), ntohs(clientRTCPPort.num()), ntohs(serverRTPPort.num()), ntohs(serverRTCPPort.num()),
	       fOurSessionId);
      break;
    }
    case RTP_TCP: {
      snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	       "RTSP/1.0 200 OK\r\n"
	       "CSeq: %s\r\n"
	       "%s"
	       "Transport: RTP/AVP/TCP;unicast;destination=%s;source=%s;interleaved=%d-%d\r\n"
	       "Session: %08X\r\n\r\n",
	       cseq,
	       dateHeader(),
	       destAddrStr, sourceAddrStr, rtpChannelId, rtcpChannelId,
	       fOurSessionId);
      break;
    }
    case RAW_UDP: {
      snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	       "RTSP/1.0 200 OK\r\n"
	       "CSeq: %s\r\n"
	       "%s"
	       "Transport: %s;unicast;destination=%s;source=%s;client_port=%d;server_port=%d\r\n"
	       "Session: %08X\r\n\r\n",
	       cseq,
	       dateHeader(),
	       streamingModeString, destAddrStr, sourceAddrStr, ntohs(clientRTPPort.num()), ntohs(serverRTPPort.num()),
	       fOurSessionId);
      break;
    }
    }
  }
  delete[] destAddrStr; delete[] sourceAddrStr; delete[] streamingModeString;
}

void RTSPServer::RTSPClientSession
::handleCmd_withinSession(char const* cmdName,
			  char const* urlPreSuffix, char const* urlSuffix,
			  char const* cseq, char const* fullRequestStr) {
  // This will either be:
  // - a non-aggregated operation, if "urlPreSuffix" is the session (stream)
  //   name and "urlSuffix" is the subsession (track) name, or
  // - a aggregated operation, if "urlSuffix" is the session (stream) name,
  //   or "urlPreSuffix" is the session (stream) name, and "urlSuffix"
  //   is empty.
  // First, figure out which of these it is:
  if (fOurServerMediaSession == NULL) { // There wasn't a previous SETUP!
    handleCmd_notSupported(cseq);
    return;
  }
  ServerMediaSubsession* subsession;
  if (urlSuffix[0] != '\0' &&
      strcmp(fOurServerMediaSession->streamName(), urlPreSuffix) == 0) {
    // Non-aggregated operation.
    // Look up the media subsession whose track id is "urlSuffix":
    ServerMediaSubsessionIterator iter(*fOurServerMediaSession);
    while ((subsession = iter.next()) != NULL) {
      if (strcmp(subsession->trackId(), urlSuffix) == 0) break; // success
    }
    if (subsession == NULL) { // no such track!
      handleCmd_notFound(cseq);
      return;
    }
  } else if (strcmp(fOurServerMediaSession->streamName(), urlSuffix) == 0 ||
	     strcmp(fOurServerMediaSession->streamName(), urlPreSuffix) == 0) {
    // Aggregated operation
    subsession = NULL;
  } else { // the request doesn't match a known stream and/or track at all!
    handleCmd_notFound(cseq);
    return;
  }

  if (strcmp(cmdName, "TEARDOWN") == 0) {
    handleCmd_TEARDOWN(subsession, cseq);
  } else if (strcmp(cmdName, "PLAY") == 0) {
    handleCmd_PLAY(subsession, cseq, fullRequestStr);
  } else if (strcmp(cmdName, "PAUSE") == 0) {
    handleCmd_PAUSE(subsession, cseq);
  } else if (strcmp(cmdName, "GET_PARAMETER") == 0) {
    handleCmd_GET_PARAMETER(subsession, cseq, fullRequestStr);
  } else if (strcmp(cmdName, "SET_PARAMETER") == 0) {
    handleCmd_SET_PARAMETER(subsession, cseq, fullRequestStr);
  }
}

void RTSPServer::RTSPClientSession
::handleCmd_TEARDOWN(ServerMediaSubsession* /*subsession*/, char const* cseq) {
  snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	   "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%s\r\n",
	   cseq, dateHeader());
  fSessionIsActive = False; // triggers deletion of ourself after responding
}

static Boolean parseScaleHeader(char const* buf, float& scale) {
  // Initialize the result parameter to a default value:
  scale = 1.0;

  // First, find "Scale:"
  while (1) {
    if (*buf == '\0') return False; // not found
    if (_strncasecmp(buf, "Scale: ", 7) == 0) break;
    ++buf;
  }

  // Then, run through each of the fields, looking for ones we handle:
  char const* fields = buf + 7;
  while (*fields == ' ') ++fields;
  float sc;
  if (sscanf(fields, "%f", &sc) == 1) {
    scale = sc;
  } else {
    return False; // The header is malformed
  }

  return True;
}

void RTSPServer::RTSPClientSession
  ::handleCmd_PLAY(ServerMediaSubsession* subsession, char const* cseq,
		   char const* fullRequestStr) {
  char* rtspURL = fOurServer.rtspURL(fOurServerMediaSession, fClientSocket);
  unsigned rtspURLSize = strlen(rtspURL);

  // Parse the client's "Scale:" header, if any:
  float scale;
  Boolean sawScaleHeader = parseScaleHeader(fullRequestStr, scale);

  // Try to set the stream's scale factor to this value:
  if (subsession == NULL /*aggregate op*/) {
    fOurServerMediaSession->testScaleFactor(scale);
  } else {
    subsession->testScaleFactor(scale);
  }

  char buf[100];
  char* scaleHeader;
  if (!sawScaleHeader) {
    buf[0] = '\0'; // Because we didn't see a Scale: header, don't send one back
  } else {
    sprintf(buf, "Scale: %f\r\n", scale);
  }
  scaleHeader = strDup(buf);

  // Parse the client's "Range:" header, if any:
  double rangeStart = 0.0, rangeEnd = 0.0;
  Boolean sawRangeHeader = parseRangeHeader(fullRequestStr, rangeStart, rangeEnd);

  // Use this information, plus the stream's duration (if known), to create
  // our own "Range:" header, for the response:
  float duration = subsession == NULL /*aggregate op*/
    ? fOurServerMediaSession->duration() : subsession->duration();
  if (duration < 0.0) {
    // We're an aggregate PLAY, but the subsessions have different durations.
    // Use the largest of these durations in our header
    duration = -duration;
  }

  if (rangeEnd <= 0.0 || rangeEnd > duration) rangeEnd = duration;
  if (rangeStart < 0.0) {
    rangeStart = 0.0;
  } else if (rangeEnd > 0.0 && scale > 0.0 && rangeStart > rangeEnd) {
    rangeStart = rangeEnd;
  }

  char* rangeHeader;
  if (!sawRangeHeader) {
    buf[0] = '\0'; // Because we didn't see a Range: header, don't send one back
  } else if (rangeEnd == 0.0 && scale >= 0.0) {
    sprintf(buf, "Range: npt=%.3f-\r\n", rangeStart);
  } else {
    sprintf(buf, "Range: npt=%.3f-%.3f\r\n", rangeStart, rangeEnd);
  }
  rangeHeader = strDup(buf);

  // Create a "RTP-Info:" line.  It will get filled in from each subsession's state:
  char const* rtpInfoFmt =
    "%s" // "RTP-Info:", plus any preceding rtpInfo items
    "%s" // comma separator, if needed
    "url=%s/%s"
    ";seq=%d"
    ";rtptime=%u"
    ;
  unsigned rtpInfoFmtSize = strlen(rtpInfoFmt);
  char* rtpInfo = strDup("RTP-Info: ");
  unsigned i, numRTPInfoItems = 0;

  // Do any required seeking/scaling on each subsession, before starting streaming:
  for (i = 0; i < fNumStreamStates; ++i) {
    if (subsession == NULL /* means: aggregated operation */
	|| subsession == fStreamStates[i].subsession) {
      if (sawScaleHeader) {
	fStreamStates[i].subsession->setStreamScale(fOurSessionId,
						    fStreamStates[i].streamToken,
						    scale);
      }
      if (sawRangeHeader) {
	fStreamStates[i].subsession->seekStream(fOurSessionId,
						fStreamStates[i].streamToken,
						rangeStart);
      }
    }
  }

  // Now, start streaming:
  for (i = 0; i < fNumStreamStates; ++i) {
    if (subsession == NULL /* means: aggregated operation */
	|| subsession == fStreamStates[i].subsession) {
      unsigned short rtpSeqNum = 0;
      unsigned rtpTimestamp = 0;
      fStreamStates[i].subsession->startStream(fOurSessionId,
					       fStreamStates[i].streamToken,
					       (TaskFunc*)noteClientLiveness, this,
					       rtpSeqNum, rtpTimestamp,
					       handleAlternativeRequestByte, this);
      const char *urlSuffix = fStreamStates[i].subsession->trackId();
      char* prevRTPInfo = rtpInfo;
      unsigned rtpInfoSize = rtpInfoFmtSize
	+ strlen(prevRTPInfo)
	+ 1
	+ rtspURLSize + strlen(urlSuffix)
	+ 5 /*max unsigned short len*/
	+ 10 /*max unsigned (32-bit) len*/
	+ 2 /*allows for trailing \r\n at final end of string*/;
      rtpInfo = new char[rtpInfoSize];
      sprintf(rtpInfo, rtpInfoFmt,
	      prevRTPInfo,
	      numRTPInfoItems++ == 0 ? "" : ",",
	      rtspURL, urlSuffix,
	      rtpSeqNum,
	      rtpTimestamp
	      );
      delete[] prevRTPInfo;
    }
  }
  if (numRTPInfoItems == 0) {
    rtpInfo[0] = '\0';
  } else {
    unsigned rtpInfoLen = strlen(rtpInfo);
    rtpInfo[rtpInfoLen] = '\r';
    rtpInfo[rtpInfoLen+1] = '\n';
    rtpInfo[rtpInfoLen+2] = '\0';
  }

  // Fill in the response:
  snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	   "RTSP/1.0 200 OK\r\n"
	   "CSeq: %s\r\n"
	   "%s"
	   "%s"
	   "%s"
	   "Session: %08X\r\n"
	   "%s\r\n",
	   cseq,
	   dateHeader(),
	   scaleHeader,
	   rangeHeader,
	   fOurSessionId,
	   rtpInfo);
  delete[] rtpInfo; delete[] rangeHeader;
  delete[] scaleHeader; delete[] rtspURL;
}

void RTSPServer::RTSPClientSession
  ::handleCmd_PAUSE(ServerMediaSubsession* subsession, char const* cseq) {
  for (unsigned i = 0; i < fNumStreamStates; ++i) {
    if (subsession == NULL /* means: aggregated operation */
	|| subsession == fStreamStates[i].subsession) {
      fStreamStates[i].subsession->pauseStream(fOurSessionId,
					       fStreamStates[i].streamToken);
    }
  }
  snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	   "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sSession: %08X\r\n\r\n",
	   cseq, dateHeader(), fOurSessionId);
}

void RTSPServer::RTSPClientSession
::handleCmd_GET_PARAMETER(ServerMediaSubsession* subsession, char const* cseq,
			  char const* /*fullRequestStr*/) {
  // We implement "GET_PARAMETER" just as a 'keep alive',
  // and send back an empty response:
  snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	   "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sSession: %08X\r\n\r\n",
	   cseq, dateHeader(), fOurSessionId);
}

void RTSPServer::RTSPClientSession
::handleCmd_SET_PARAMETER(ServerMediaSubsession* /*subsession*/, char const* cseq,
			  char const* /*fullRequestStr*/) {
  // By default, we don't implement "SET_PARAMETER":
  handleCmd_notSupported(cseq);
}

static Boolean parseAuthorizationHeader(char const* buf,
					char const*& username,
					char const*& realm,
					char const*& nonce, char const*& uri,
					char const*& response) {
  // Initialize the result parameters to default values:
  username = realm = nonce = uri = response = NULL;

  // First, find "Authorization:"
  while (1) {
    if (*buf == '\0') return False; // not found
    if (_strncasecmp(buf, "Authorization: Digest ", 22) == 0) break;
    ++buf;
  }

  // Then, run through each of the fields, looking for ones we handle:
  char const* fields = buf + 22;
  while (*fields == ' ') ++fields;
  char* parameter = strDupSize(fields);
  char* value = strDupSize(fields);
  while (1) {
    value[0] = '\0';
    if (sscanf(fields, "%[^=]=\"%[^\"]\"", parameter, value) != 2 &&
	sscanf(fields, "%[^=]=\"\"", parameter) != 1) {
      break;
    }
    if (strcmp(parameter, "username") == 0) {
      username = strDup(value);
    } else if (strcmp(parameter, "realm") == 0) {
      realm = strDup(value);
    } else if (strcmp(parameter, "nonce") == 0) {
      nonce = strDup(value);
    } else if (strcmp(parameter, "uri") == 0) {
      uri = strDup(value);
    } else if (strcmp(parameter, "response") == 0) {
      response = strDup(value);
    }

    fields += strlen(parameter) + 2 /*="*/ + strlen(value) + 1 /*"*/;
    while (*fields == ',' || *fields == ' ') ++fields;
        // skip over any separating ',' and ' ' chars
    if (*fields == '\0' || *fields == '\r' || *fields == '\n') break;
  }
  delete[] parameter; delete[] value;
  return True;
}

Boolean RTSPServer::RTSPClientSession
::authenticationOK(char const* cmdName, char const* cseq,
		   char const* urlSuffix, char const* fullRequestStr) {

  if (!fOurServer.specialClientAccessCheck(fClientSocket, fClientAddr, urlSuffix)) {
    snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
             "RTSP/1.0 401 Unauthorized\r\n"
             "CSeq: %s\r\n"
             "%s"
             "\r\n",
             cseq, dateHeader());
    return False;
  }

  // If we weren't set up with an authentication database, we're OK:
  if (fOurServer.fAuthDB == NULL) return True;

  char const* username = NULL; char const* realm = NULL; char const* nonce = NULL;
  char const* uri = NULL; char const* response = NULL;
  Boolean success = False;

  do {
    // To authenticate, we first need to have a nonce set up
    // from a previous attempt:
    if (fCurrentAuthenticator.nonce() == NULL) break;

    // Next, the request needs to contain an "Authorization:" header,
    // containing a username, (our) realm, (our) nonce, uri,
    // and response string:
    if (!parseAuthorizationHeader(fullRequestStr,
				  username, realm, nonce, uri, response)
	|| username == NULL
	|| realm == NULL || strcmp(realm, fCurrentAuthenticator.realm()) != 0
	|| nonce == NULL || strcmp(nonce, fCurrentAuthenticator.nonce()) != 0
	|| uri == NULL || response == NULL) {
      break;
    }

    // Next, the username has to be known to us:
    char const* password = fOurServer.fAuthDB->lookupPassword(username);
#ifdef DEBUG
    fprintf(stderr, "lookupPassword(%s) returned password %s\n", username, password);
#endif
    if (password == NULL) break;
    fCurrentAuthenticator.
      setUsernameAndPassword(username, password,
			     fOurServer.fAuthDB->passwordsAreMD5());

    // Finally, compute a digest response from the information that we have,
    // and compare it to the one that we were given:
    char const* ourResponse
      = fCurrentAuthenticator.computeDigestResponse(cmdName, uri);
    success = (strcmp(ourResponse, response) == 0);
    fCurrentAuthenticator.reclaimDigestResponse(ourResponse);
  } while (0);

  delete[] (char*)username; delete[] (char*)realm; delete[] (char*)nonce;
  delete[] (char*)uri; delete[] (char*)response;
  if (success) return True;

  // If we get here, there was some kind of authentication failure.
  // Send back a "401 Unauthorized" response, with a new random nonce:
  fCurrentAuthenticator.setRealmAndRandomNonce(fOurServer.fAuthDB->realm());
  snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	   "RTSP/1.0 401 Unauthorized\r\n"
	   "CSeq: %s\r\n"
	   "%s"
	   "WWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\"\r\n\r\n",
	   cseq,
	   dateHeader(),
	   fCurrentAuthenticator.realm(), fCurrentAuthenticator.nonce());
  return False;
}

void RTSPServer::RTSPClientSession::noteLiveness() {
  if (fOurServer.fReclamationTestSeconds > 0) {
    envir().taskScheduler()
      .rescheduleDelayedTask(fLivenessCheckTask,
			     fOurServer.fReclamationTestSeconds*1000000,
			     (TaskFunc*)livenessTimeoutTask, this);
  }
}

void RTSPServer::RTSPClientSession
::noteClientLiveness(RTSPClientSession* clientSession) {
  clientSession->noteLiveness();
}

void RTSPServer::RTSPClientSession
::livenessTimeoutTask(RTSPClientSession* clientSession) {
  // If this gets called, the client session is assumed to have timed out,
  // so delete it:
#ifdef DEBUG
  fprintf(stderr, "RTSP client session from %s has timed out (due to inactivity)\n", our_inet_ntoa(clientSession->fClientAddr.sin_addr));
#endif
  delete clientSession;
}

RTSPServer::RTSPClientSession*
RTSPServer::createNewClientSession(unsigned sessionId, int clientSocket, struct sockaddr_in clientAddr) {
  return new RTSPClientSession(*this, sessionId, clientSocket, clientAddr);
}


////////// ServerMediaSessionIterator implementation //////////

RTSPServer::ServerMediaSessionIterator
::ServerMediaSessionIterator(RTSPServer& server)
  : fOurIterator((server.fServerMediaSessions == NULL)
		 ? NULL : HashTable::Iterator::create(*server.fServerMediaSessions)) {
}

RTSPServer::ServerMediaSessionIterator::~ServerMediaSessionIterator() {
  delete fOurIterator;
}

ServerMediaSession* RTSPServer::ServerMediaSessionIterator::next() {
  if (fOurIterator == NULL) return NULL;

  char const* key; // dummy
  return (ServerMediaSession*)(fOurIterator->next(key));
}


////////// UserAuthenticationDatabase implementation //////////

UserAuthenticationDatabase::UserAuthenticationDatabase(char const* realm,
						       Boolean passwordsAreMD5)
  : fTable(HashTable::create(STRING_HASH_KEYS)),
    fRealm(strDup(realm == NULL ? "LIVE555 Streaming Media" : realm)),
    fPasswordsAreMD5(passwordsAreMD5) {
}

UserAuthenticationDatabase::~UserAuthenticationDatabase() {
  delete[] fRealm;
  delete fTable;
}

void UserAuthenticationDatabase::addUserRecord(char const* username,
					       char const* password) {
  fTable->Add(username, (void*)(strDup(password)));
}

void UserAuthenticationDatabase::removeUserRecord(char const* username) {
  char* password = (char*)(fTable->Lookup(username));
  fTable->Remove(username);
  delete[] password;
}

char const* UserAuthenticationDatabase::lookupPassword(char const* username) {
  return (char const*)(fTable->Lookup(username));
}
