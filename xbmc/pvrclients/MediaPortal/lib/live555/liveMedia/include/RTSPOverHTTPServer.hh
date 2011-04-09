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
// C++ header

#ifndef _RTSP_OVER_HTTP_SERVER_HH
#define _RTSP_OVER_HTTP_SERVER_HH

#include "Media.hh"
#include "NetInterface.hh"

#define HTTP_BUFFER_SIZE 10000 // for incoming requests, and outgoing responses

class RTSPOverHTTPServer: public Medium {
public:
  static RTSPOverHTTPServer* createNew(UsageEnvironment& env, Port ourHTTPPort = 80,
				       Port rtspServerPort = 554,
				       char const* rtspServerHostName = "localhost");

protected:
  RTSPOverHTTPServer(UsageEnvironment& env, int ourSocket,
		     Port rtspServerPort, char const* rtspServerHostName);
      // called only by createNew();
  virtual ~RTSPOverHTTPServer();

  static int setUpOurSocket(UsageEnvironment& env, Port& ourPort);

private:
  static void incomingConnectionHandler(void*, int /*mask*/);
  void incomingConnectionHandler1();

  // The state of each individual connection handled by a HTTP server:
  class HTTPClientConnection {
  public:
    HTTPClientConnection(RTSPOverHTTPServer& ourServer, int clientSocket);
    virtual ~HTTPClientConnection();
  private:
    static void incomingRequestHandler(void*, int /*mask*/);
    void incomingRequestHandler1();
    UsageEnvironment& envir() { return fOurServer.envir(); }
    void resetRequestBuffer();
    Boolean parseHTTPRequestString(char* resultCmdName,
				   unsigned resultCmdNameMaxSize,
				   char* sessionCookie,
				   unsigned sessionCookieMaxSize,
				   char* acceptStr,
				   unsigned acceptStrMaxSize,
				   char* contentTypeStr,
				   unsigned contentTypeStrMaxSize);
    void handleCmd_bad();
#if 0 //#####@@@@@
    void handleCmd_notSupported(char const* cseq);
    void handleCmd_notFound(char const* cseq);
    void handleCmd_unsupportedTransport(char const* cseq);
    void handleCmd_OPTIONS(char const* cseq);
    void handleCmd_DESCRIBE(char const* cseq, char const* urlSuffix,
			    char const* fullRequestStr);
    void handleCmd_SETUP(char const* cseq,
			 char const* urlPreSuffix, char const* urlSuffix,
			 char const* fullRequestStr);
    void handleCmd_withinSession(char const* cmdName,
				 char const* urlPreSuffix, char const* urlSuffix,
				 char const* cseq, char const* fullRequestStr);
    void handleCmd_TEARDOWN(ServerMediaSubsession* subsession,
			    char const* cseq);
    void handleCmd_PLAY(ServerMediaSubsession* subsession,
			char const* cseq, char const* fullRequestStr);
    void handleCmd_PAUSE(ServerMediaSubsession* subsession,
			 char const* cseq);
    void handleCmd_GET_PARAMETER(ServerMediaSubsession* subsession,
				 char const* cseq, char const* fullRequestStr);
    Boolean authenticationOK(char const* cmdName, char const* cseq,
			     char const* fullRequestStr);
    void noteLiveness();
    Boolean isMulticast() const { return fIsMulticast; }
    static void noteClientLiveness(HTTPClientConnection* clientConnection);
    static void livenessTimeoutTask(HTTPClientConnection* clientConnection);
#endif

  private:
    RTSPOverHTTPServer& fOurServer;
#if 0 //#####@@@@@
    unsigned fOurSessionId;
    ServerMediaSession* fOurServerMediaSession;
#endif
    int fClientSocket;
#if 0 //#####@@@@@
    struct sockaddr_in fClientAddr;
    TaskToken fLivenessCheckTask;
#endif
    unsigned char fRequestBuffer[HTTP_BUFFER_SIZE];
    unsigned fRequestBytesAlreadySeen, fRequestBufferBytesLeft;
    unsigned char* fLastCRLF;
    unsigned char fResponseBuffer[HTTP_BUFFER_SIZE];
    Boolean fSessionIsActive;
#if 0 //#####@@@@@
    Authenticator fCurrentAuthenticator; // used if access control is needed
    unsigned char fTCPStreamIdCount; // used for (optional) RTP/TCP
    unsigned fNumStreamStates;
    struct streamState {
      ServerMediaSubsession* subsession;
      void* streamToken;
    } * fStreamStates;
#endif
  };

private:
  friend class RTSPOverHTTPTunnel;
  int fServerSocket;
  Port fRTSPServerPort;
  char* fRTSPServerHostName;
};

#endif
