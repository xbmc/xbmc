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
// A generic RTSP client
// C++ header

#ifndef _RTSP_CLIENT_HH
#define _RTSP_CLIENT_HH

#ifndef _MEDIA_SESSION_HH
#include "MediaSession.hh"
#endif
#ifndef _NET_ADDRESS_HH
#include "NetAddress.hh"
#endif
#ifndef _DIGEST_AUTHENTICATION_HH
#include "DigestAuthentication.hh"
#endif

class RTSPClient: public Medium {
public:
  static RTSPClient* createNew(UsageEnvironment& env,
			       int verbosityLevel = 0,
			       char const* applicationName = NULL,
			       portNumBits tunnelOverHTTPPortNum = 0);
  // If "tunnelOverHTTPPortNum" is non-zero, we tunnel RTSP (and RTP)
  // over a HTTP connection with the given port number, using the technique
  // described in Apple's document <http://developer.apple.com/documentation/QuickTime/QTSS/Concepts/chapter_2_section_14.html>

  int socketNum() const { return fInputSocketNum; }

  static Boolean lookupByName(UsageEnvironment& env,
			      char const* sourceName,
			      RTSPClient*& resultClient);

  char* describeURL(char const* url, Authenticator* authenticator = NULL,
		    Boolean allowKasennaProtocol = False, int timeout = -1);
      // Issues a RTSP "DESCRIBE" command
      // Returns the SDP description of a session, or NULL if none
      // (This is dynamically allocated, and must later be freed
      //  by the caller - using "delete[]")
  char* describeWithPassword(char const* url,
			     char const* username, char const* password,
			     Boolean allowKasennaProtocol = False, 
			     int timeout = -1);
      // Uses "describeURL()" to do a "DESCRIBE" - first
      // without using "password", then (if we get an Unauthorized
      // response) with an authentication response computed from "password"

  Boolean announceSDPDescription(char const* url,
				 char const* sdpDescription,
				 Authenticator* authenticator = NULL,
				 int timeout = -1);
      // Issues a RTSP "ANNOUNCE" command
      // Returns True iff this command succeeds
  Boolean announceWithPassword(char const* url, char const* sdpDescription,
			       char const* username, char const* password, int timeout = -1);
      // Uses "announceSDPDescription()" to do an "ANNOUNCE" - first
      // without using "password", then (if we get an Unauthorized
      // response) with an authentication response computed from "password"

  char* sendOptionsCmd(char const* url,
		       char* username = NULL, char* password = NULL,
		       Authenticator* authenticator = NULL,
		       int timeout = -1);
      // Issues a RTSP "OPTIONS" command
      // Returns a string containing the list of options, or NULL

  Boolean setupMediaSubsession(MediaSubsession& subsession,
			       Boolean streamOutgoing = False,
			       Boolean streamUsingTCP = False,
			       Boolean forceMulticastOnUnspecified = False);
      // Issues a RTSP "SETUP" command on "subsession".
      // Returns True iff this command succeeds
      // If "forceMulticastOnUnspecified" is True (and "streamUsingTCP" is False),
      // then the client will request a multicast stream if the media address
      // in the original SDP response was unspecified (i.e., 0.0.0.0).
      // Note, however, that not all servers will support this.

  Boolean playMediaSession(MediaSession& session,
			   double start = 0.0f, double end = -1.0f,
			   float scale = 1.0f);
      // Issues an aggregate RTSP "PLAY" command on "session".
      // Returns True iff this command succeeds
      // (Note: start=-1 means 'resume'; end=-1 means 'play to end')
  Boolean playMediaSubsession(MediaSubsession& subsession,
			      double start = 0.0f, double end = -1.0f,
			      float scale = 1.0f,
			      Boolean hackForDSS = False);
      // Issues a RTSP "PLAY" command on "subsession".
      // Returns True iff this command succeeds
      // (Note: start=-1 means 'resume'; end=-1 means 'play to end')

  Boolean pauseMediaSession(MediaSession& session);
      // Issues an aggregate RTSP "PAUSE" command on "session".
      // Returns True iff this command succeeds
  Boolean pauseMediaSubsession(MediaSubsession& subsession);
      // Issues a RTSP "PAUSE" command on "subsession".
      // Returns True iff this command succeeds

  Boolean recordMediaSubsession(MediaSubsession& subsession);
      // Issues a RTSP "RECORD" command on "subsession".
      // Returns True iff this command succeeds

  Boolean setMediaSessionParameter(MediaSession& session,
				   char const* parameterName,
				   char const* parameterValue);
      // Issues a RTSP "SET_PARAMETER" command on "subsession".
      // Returns True iff this command succeeds

  Boolean getMediaSessionParameter(MediaSession& session,
				   char const* parameterName,
				   char*& parameterValue);
      // Issues a RTSP "GET_PARAMETER" command on "subsession".
      // Returns True iff this command succeeds

  Boolean teardownMediaSession(MediaSession& session);
      // Issues an aggregate RTSP "TEARDOWN" command on "session".
      // Returns True iff this command succeeds
  Boolean teardownMediaSubsession(MediaSubsession& subsession);
      // Issues a RTSP "TEARDOWN" command on "subsession".
      // Returns True iff this command succeeds

  static Boolean parseRTSPURL(UsageEnvironment& env, char const* url,
			      NetAddress& address, portNumBits& portNum,
			      char const** urlSuffix = NULL);
      // (ignores any "<username>[:<password>]@" in "url")
  static Boolean parseRTSPURLUsernamePassword(char const* url,
					      char*& username,
					      char*& password);

  unsigned describeStatus() const { return fDescribeStatusCode; }

  void setUserAgentString(char const* userAgentStr);
  // sets an alternative string to be used in RTSP "User-Agent:" headers

  unsigned sessionTimeoutParameter() const { return fSessionTimeoutParameter; }

#ifdef SUPPORT_REAL_RTSP
  Boolean usingRealNetworksChallengeResponse() const { return fRealChallengeStr != NULL; }
#endif

protected:
  RTSPClient(UsageEnvironment& env, int verbosityLevel,
	     char const* applicationName, portNumBits tunnelOverHTTPPortNum);
      // called only by createNew();
  virtual ~RTSPClient();

private: // redefined virtual functions
  virtual Boolean isRTSPClient() const;

private:
  void reset();
  void resetTCPSockets();

  Boolean openConnectionFromURL(char const* url, Authenticator* authenticator,
				int timeout = -1);
  char* createAuthenticatorString(Authenticator const* authenticator,
				  char const* cmd, char const* url);
  static void checkForAuthenticationFailure(unsigned responseCode,
					    char*& nextLineStart,
					    Authenticator* authenticator);
  Boolean sendRequest(char const* requestString, char const* tag,
		      Boolean base64EncodeIfOverHTTP = True);
  Boolean getResponse(char const* tag,
		      unsigned& bytesRead, unsigned& responseCode,
		      char*& firstLine, char*& nextLineStart,
		      Boolean checkFor200Response = True);
  unsigned getResponse1(char*& responseBuffer, unsigned responseBufferSize);
  Boolean parseResponseCode(char const* line, unsigned& responseCode);
  Boolean parseTransportResponse(char const* line,
				 char*& serverAddressStr,
				 portNumBits& serverPortNum,
				 unsigned char& rtpChannelId,
				 unsigned char& rtcpChannelId);
  Boolean parseRTPInfoHeader(char*& line, u_int16_t& seqNum, u_int32_t& timestamp);
  Boolean parseScaleHeader(char const* line, float& scale);
  Boolean parseGetParameterHeader(char const* line,
                                  const char* param,
                                  char*& value);
  char const* sessionURL(MediaSession const& session) const;
  void constructSubsessionURL(MediaSubsession const& subsession,
			      char const*& prefix,
			      char const*& separator,
			      char const*& suffix);
  Boolean setupHTTPTunneling(char const* urlSuffix, Authenticator* authenticator);

  // Support for handling requests sent back by a server:
  static void incomingRequestHandler(void*, int /*mask*/);
  void incomingRequestHandler1();
  void handleCmd_notSupported(char const* cseq);

private:
  int fVerbosityLevel;
  portNumBits fTunnelOverHTTPPortNum;
  char* fUserAgentHeaderStr;
      unsigned fUserAgentHeaderStrSize;
  int fInputSocketNum, fOutputSocketNum;
  unsigned fServerAddress;
  static unsigned fCSeq; // sequence number, used in consecutive requests
      // Note: it's static, to ensure that it differs if more than one
      // connection is made to the same server, using the same URL.
      // Some servers (e.g., DSS) may have problems with this otherwise.
  char* fBaseURL;
  Authenticator fCurrentAuthenticator;
  unsigned char fTCPStreamIdCount; // used for (optional) RTP/TCP
  char* fLastSessionId;
  unsigned fSessionTimeoutParameter; // optionally set in response "Session:" headers
#ifdef SUPPORT_REAL_RTSP
  char* fRealChallengeStr;
  char* fRealETagStr;
#endif
  unsigned fDescribeStatusCode;
  // 0: OK; 1: connection failed; 2: stream unavailable
  char* fResponseBuffer;
  unsigned fResponseBufferSize;

  // The following fields are used to implement the non-standard Kasenna protocol:
  Boolean fServerIsKasenna;
  char* fKasennaContentType;

  // The following is used to deal with Microsoft servers' non-standard use of RTSP:
  Boolean fServerIsMicrosoft;
};

#endif
