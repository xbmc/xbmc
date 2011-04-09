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
// Implementation

#include "RTSPClient.hh"
#include "RTSPCommon.hh"
#include "Base64.hh"
#include "Locale.hh"
#include <GroupsockHelper.hh>
#include "our_md5.h"
#ifdef SUPPORT_REAL_RTSP
#include "../RealRTSP/include/RealRTSP.hh"
#endif

////////// RTSPClient //////////

RTSPClient* RTSPClient::createNew(UsageEnvironment& env,
				  int verbosityLevel,
				  char const* applicationName,
				  portNumBits tunnelOverHTTPPortNum) {
  return new RTSPClient(env, verbosityLevel,
			applicationName, tunnelOverHTTPPortNum);
}

Boolean RTSPClient::lookupByName(UsageEnvironment& env,
				 char const* instanceName,
				 RTSPClient*& resultClient) {
  resultClient = NULL; // unless we succeed

  Medium* medium;
  if (!Medium::lookupByName(env, instanceName, medium)) return False;

  if (!medium->isRTSPClient()) {
    env.setResultMsg(instanceName, " is not a RTSP client");
    return False;
  }

  resultClient = (RTSPClient*)medium;
  return True;
}

unsigned RTSPClient::fCSeq = 0;

RTSPClient::RTSPClient(UsageEnvironment& env,
		       int verbosityLevel, char const* applicationName,
		       portNumBits tunnelOverHTTPPortNum)
  : Medium(env),
    fVerbosityLevel(verbosityLevel),
    fTunnelOverHTTPPortNum(tunnelOverHTTPPortNum),
    fInputSocketNum(-1), fOutputSocketNum(-1), fServerAddress(0),
    fBaseURL(NULL), fTCPStreamIdCount(0), fLastSessionId(NULL),
    fSessionTimeoutParameter(0),
#ifdef SUPPORT_REAL_RTSP
    fRealChallengeStr(NULL), fRealETagStr(NULL),
#endif
    fServerIsKasenna(False), fKasennaContentType(NULL),
    fServerIsMicrosoft(False)
{
  fResponseBufferSize = 20000;
  fResponseBuffer = new char[fResponseBufferSize+1];

  // Set the "User-Agent:" header to use in each request:
  char const* const libName = "LIVE555 Streaming Media v";
  char const* const libVersionStr = LIVEMEDIA_LIBRARY_VERSION_STRING;
  char const* libPrefix; char const* libSuffix;
  if (applicationName == NULL || applicationName[0] == '\0') {
    applicationName = libPrefix = libSuffix = "";
  } else {
    libPrefix = " (";
    libSuffix = ")";
  }
  char const* const formatStr = "User-Agent: %s%s%s%s%s\r\n";
  unsigned headerSize
    = strlen(formatStr) + strlen(applicationName) + strlen(libPrefix)
    + strlen(libName) + strlen(libVersionStr) + strlen(libSuffix);
  fUserAgentHeaderStr = new char[headerSize];
  sprintf(fUserAgentHeaderStr, formatStr,
	  applicationName, libPrefix, libName, libVersionStr, libSuffix);
  fUserAgentHeaderStrSize = strlen(fUserAgentHeaderStr);
}

void RTSPClient::setUserAgentString(char const* userAgentStr) {
  if (userAgentStr == NULL) return;

  // Change the existing user agent header string:
  char const* const formatStr = "User-Agent: %s\r\n";
  unsigned const headerSize = strlen(formatStr) + strlen(userAgentStr) + 1;
  delete[] fUserAgentHeaderStr;
  fUserAgentHeaderStr = new char[headerSize];
  sprintf(fUserAgentHeaderStr, formatStr, userAgentStr);
  fUserAgentHeaderStrSize = strlen(fUserAgentHeaderStr);
}

RTSPClient::~RTSPClient() {
  envir().taskScheduler().turnOffBackgroundReadHandling(fInputSocketNum); // must be called before:
  reset();

  delete[] fResponseBuffer;
  delete[] fUserAgentHeaderStr;
}

Boolean RTSPClient::isRTSPClient() const {
  return True;
}

void RTSPClient::resetTCPSockets() {
  if (fInputSocketNum >= 0) {
    ::closeSocket(fInputSocketNum);
    if (fOutputSocketNum != fInputSocketNum) ::closeSocket(fOutputSocketNum);
  }
  fInputSocketNum = fOutputSocketNum = -1;
}

void RTSPClient::reset() {
  resetTCPSockets();
  fServerAddress = 0;

  delete[] fBaseURL; fBaseURL = NULL;

  fCurrentAuthenticator.reset();

  delete[] fKasennaContentType; fKasennaContentType = NULL;
#ifdef SUPPORT_REAL_RTSP
  delete[] fRealChallengeStr; fRealChallengeStr = NULL;
  delete[] fRealETagStr; fRealETagStr = NULL;
#endif
  delete[] fLastSessionId; fLastSessionId = NULL;
}

static char* getLine(char* startOfLine) {
  // returns the start of the next line, or NULL if none
  for (char* ptr = startOfLine; *ptr != '\0'; ++ptr) {
    // Check for the end of line: \r\n (but also accept \r or \n by itself):
    if (*ptr == '\r' || *ptr == '\n') {
      // We found the end of the line
      if (*ptr == '\r') {
	*ptr++ = '\0';
	if (*ptr == '\n') ++ptr;
      } else {
        *ptr++ = '\0';
      }
      return ptr;
    }
  }

  return NULL;
}

char* RTSPClient::describeURL(char const* url, Authenticator* authenticator,
			      Boolean allowKasennaProtocol, int timeout) {
  char* cmd = NULL;
  fDescribeStatusCode = 0;
  do {
    // First, check whether "url" contains a username:password to be used:
    char* username; char* password;
    if (authenticator == NULL
	&& parseRTSPURLUsernamePassword(url, username, password)) {
      char* result = describeWithPassword(url, username, password, allowKasennaProtocol, timeout);
      delete[] username; delete[] password; // they were dynamically allocated
      return result;
    }

    if (!openConnectionFromURL(url, authenticator, timeout)) break;

    // Send the DESCRIBE command:

    // First, construct an authenticator string:
    fCurrentAuthenticator.reset();
    char* authenticatorStr
      = createAuthenticatorString(authenticator, "DESCRIBE", url);

    char const* acceptStr = allowKasennaProtocol
      ? "Accept: application/x-rtsp-mh, application/sdp\r\n"
      : "Accept: application/sdp\r\n";

    // (Later implement more, as specified in the RTSP spec, sec D.1 #####)
    char const* const cmdFmt =
      "DESCRIBE %s RTSP/1.0\r\n"
      "CSeq: %d\r\n"
      "%s"
      "%s"
      "%s"
#ifdef SUPPORT_REAL_RTSP
      REAL_DESCRIBE_HEADERS
#endif
      "\r\n";
    unsigned cmdSize = strlen(cmdFmt)
      + strlen(url)
      + 20 /* max int len */
      + strlen(acceptStr)
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrSize;
    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt,
	    url,
	    ++fCSeq,
            acceptStr,
	    authenticatorStr,
	    fUserAgentHeaderStr);
    delete[] authenticatorStr;

    if (!sendRequest(cmd, "DESCRIBE")) break;

    // Get the response from the server:
    unsigned bytesRead; unsigned responseCode;
    char* firstLine; char* nextLineStart;
    if (!getResponse("DESCRIBE", bytesRead, responseCode, firstLine, nextLineStart,
		     False /*don't check for response code 200*/)) break;

    // Inspect the first line to check whether it's a result code that
    // we can handle.
    Boolean wantRedirection = False;
    char* redirectionURL = NULL;
#ifdef SUPPORT_REAL_RTSP
    delete[] fRealETagStr; fRealETagStr = new char[fResponseBufferSize];
#endif
    if (responseCode == 301 || responseCode == 302) {
      wantRedirection = True;
      redirectionURL = new char[fResponseBufferSize]; // ensures enough space
    } else if (responseCode != 200) {
      checkForAuthenticationFailure(responseCode, nextLineStart, authenticator);
      envir().setResultMsg("cannot handle DESCRIBE response: ", firstLine);
      break;
    }

    // Skip over subsequent header lines, until we see a blank line.
    // The remaining data is assumed to be the SDP descriptor that we want.
    // While skipping over the header lines, we also check for certain headers
    // that we recognize.
    // (We should also check for "Content-type: application/sdp",
    // "Content-location", "CSeq", etc.) #####
    char* serverType = new char[fResponseBufferSize]; // ensures enough space
    int contentLength = -1;
    char* lineStart;
    while (1) {
      lineStart = nextLineStart;
      if (lineStart == NULL) break;

      nextLineStart = getLine(lineStart);
      if (lineStart[0] == '\0') break; // this is a blank line

      if (sscanf(lineStart, "Content-Length: %d", &contentLength) == 1
	  || sscanf(lineStart, "Content-length: %d", &contentLength) == 1) {
	if (contentLength < 0) {
	  envir().setResultMsg("Bad \"Content-length:\" header: \"",
			       lineStart, "\"");
	  break;
	}
      } else if (strncmp(lineStart, "Content-Base:", 13) == 0) {
	int cbIndex = 13;

	while (lineStart[cbIndex] == ' ' || lineStart[cbIndex] == '\t') ++cbIndex;
	if (lineStart[cbIndex] != '\0'/*sanity check*/) {
	  delete[] fBaseURL; fBaseURL = strDup(&lineStart[cbIndex]);
	}
      } else if (sscanf(lineStart, "Server: %s", serverType) == 1) {
	if (strncmp(serverType, "Kasenna", 7) == 0) fServerIsKasenna = True;
	if (strncmp(serverType, "WMServer", 8) == 0) fServerIsMicrosoft = True;
#ifdef SUPPORT_REAL_RTSP
      } else if (sscanf(lineStart, "ETag: %s", fRealETagStr) == 1) {
#endif
      } else if (wantRedirection) {
	if (sscanf(lineStart, "Location: %s", redirectionURL) == 1) {
	  // Try again with this URL
	  if (fVerbosityLevel >= 1) {
	    envir() << "Redirecting to the new URL \""
		    << redirectionURL << "\"\n";
	  }
	  reset();
	  char* result = describeURL(redirectionURL, authenticator, allowKasennaProtocol, timeout);
	  delete[] redirectionURL;
	  delete[] serverType;
	  delete[] cmd;
	  return result;
	}
      }
    }
    delete[] serverType;

    // We're now at the end of the response header lines
    if (wantRedirection) {
      envir().setResultMsg("Saw redirection response code, but not a \"Location:\" header");
      delete[] redirectionURL;
      break;
    }
    if (lineStart == NULL) {
      envir().setResultMsg("no content following header lines: ", fResponseBuffer);
      break;
    }

    // Use the remaining data as the SDP descr, but first, check
    // the "Content-length:" header (if any) that we saw.  We may need to
    // read more data, or we may have extraneous data in the buffer.
    char* bodyStart = nextLineStart;
    if (contentLength >= 0) {
      // We saw a "Content-length:" header
      unsigned numBodyBytes = &firstLine[bytesRead] - bodyStart;
      if (contentLength > (int)numBodyBytes) {
	// We need to read more data.  First, make sure we have enough
	// space for it:
	unsigned numExtraBytesNeeded = contentLength - numBodyBytes;
	unsigned remainingBufferSize
	  = fResponseBufferSize - (bytesRead + (firstLine - fResponseBuffer));
	if (numExtraBytesNeeded > remainingBufferSize) {
	  char tmpBuf[200];
	  sprintf(tmpBuf, "Read buffer size (%d) is too small for \"Content-length:\" %d (need a buffer size of >= %d bytes\n",
		  fResponseBufferSize, contentLength,
		  fResponseBufferSize + numExtraBytesNeeded - remainingBufferSize);
	  envir().setResultMsg(tmpBuf);
	  break;
	}

	// Keep reading more data until we have enough:
	if (fVerbosityLevel >= 1) {
	  envir() << "Need to read " << numExtraBytesNeeded
		  << " extra bytes\n";
	}
	while (numExtraBytesNeeded > 0) {
	  struct sockaddr_in fromAddress;
	  char* ptr = &firstLine[bytesRead];
	  int bytesRead2 = readSocket(envir(), fInputSocketNum, (unsigned char*)ptr,
				      numExtraBytesNeeded, fromAddress);
	  if (bytesRead2 <= 0) break;
	  ptr[bytesRead2] = '\0';
	  if (fVerbosityLevel >= 1) {
	    envir() << "Read " << bytesRead2 << " extra bytes: "
		    << ptr << "\n";
	  }

	  bytesRead += bytesRead2;
	  numExtraBytesNeeded -= bytesRead2;
	}
	if (numExtraBytesNeeded > 0) break; // one of the reads failed
      }

      // Remove any '\0' characters from inside the SDP description.
      // Any such characters would violate the SDP specification, but
      // some RTSP servers have been known to include them:
      int from, to = 0;
      for (from = 0; from < contentLength; ++from) {
	if (bodyStart[from] != '\0') {
	  if (to != from) bodyStart[to] = bodyStart[from];
	  ++to;
	}
      }
      if (from != to && fVerbosityLevel >= 1) {
	envir() << "Warning: " << from-to << " invalid 'NULL' bytes were found in (and removed from) the SDP description.\n";
      }
      bodyStart[to] = '\0'; // trims any extra data
    }

    ////////// BEGIN Kasenna BS //////////
    // If necessary, handle Kasenna's non-standard BS response:
    if (fServerIsKasenna && strncmp(bodyStart, "<MediaDescription>", 18) == 0) {
      // Translate from x-rtsp-mh to sdp
      int videoPid, audioPid;
      u_int64_t mh_duration;
      char* currentWord = new char[fResponseBufferSize]; // ensures enough space
      delete[] fKasennaContentType;
      fKasennaContentType = new char[fResponseBufferSize]; // ensures enough space
      char* currentPos = bodyStart;

      while (strcmp(currentWord, "</MediaDescription>") != 0) {
          sscanf(currentPos, "%s", currentWord);

          if (strcmp(currentWord, "VideoPid") == 0) {
	    currentPos += strlen(currentWord) + 1;
	    sscanf(currentPos, "%s", currentWord);
	    currentPos += strlen(currentWord) + 1;
	    sscanf(currentPos, "%d", &videoPid);
	    currentPos += 3;
          }

          if (strcmp(currentWord, "AudioPid") == 0) {
	    currentPos += strlen(currentWord) + 1;
	    sscanf(currentPos, "%s", currentWord);
	    currentPos += strlen(currentWord) + 1;
	    sscanf(currentPos, "%d", &audioPid);
	    currentPos += 3;
          }

          if (strcmp(currentWord, "Duration") == 0) {
	    currentPos += strlen(currentWord) + 1;
	    sscanf(currentPos, "%s", currentWord);
	    currentPos += strlen(currentWord) + 1;
	    sscanf(currentPos, "%llu", &mh_duration);
	    currentPos += 3;
          }

          if (strcmp(currentWord, "TypeSpecificData") == 0) {
	    currentPos += strlen(currentWord) + 1;
	    sscanf(currentPos, "%s", currentWord);
	    currentPos += strlen(currentWord) + 1;
	    sscanf(currentPos, "%s", fKasennaContentType);
	    currentPos += 3;
	    printf("Kasenna Content Type: %s\n", fKasennaContentType);
          }

          currentPos += strlen(currentWord) + 1;
	}

      if (fKasennaContentType != NULL
	  && strcmp(fKasennaContentType, "PARTNER_41_MPEG-4") == 0) {
          char* describeSDP = describeURL(url, authenticator, True, timeout);

	  delete[] currentWord;
          delete[] cmd;
          return describeSDP;
      }

      unsigned char byte1 = fServerAddress & 0x000000ff;
      unsigned char byte2 = (fServerAddress & 0x0000ff00) >>  8;
      unsigned char byte3 = (fServerAddress & 0x00ff0000) >> 16;
      unsigned char byte4 = (fServerAddress & 0xff000000) >> 24;

      char const* sdpFmt =
	"v=0\r\n"
	"o=NoSpacesAllowed 1 1 IN IP4 %u.%u.%u.%u\r\n"
	"s=%s\r\n"
	"c=IN IP4 %u.%u.%u.%u\r\n"
	"t=0 0\r\n"
	"a=control:*\r\n"
	"a=range:npt=0-%llu\r\n"
	"m=video 1554 RAW/RAW/UDP 33\r\n"
	"a=control:trackID=%d\r\n";
      unsigned sdpBufSize = strlen(sdpFmt)
	+ 4*3 // IP address
	+ strlen(url)
	+ 20 // max int length
	+ 20; // max int length
      char* sdpBuf = new char[sdpBufSize];
      sprintf(sdpBuf, sdpFmt,
	      byte1, byte2, byte3, byte4,
	      url,
	      byte1, byte2, byte3, byte4,
	      mh_duration/1000000,
	      videoPid);

      char* result = strDup(sdpBuf);
      delete[] sdpBuf; delete[] currentWord;
      delete[] cmd;
      return result;
    }
    ////////// END Kasenna BS //////////

    delete[] cmd;
    return strDup(bodyStart);
  } while (0);

  delete[] cmd;
  if (fDescribeStatusCode == 0) fDescribeStatusCode = 2;
  return NULL;
}

char* RTSPClient
::describeWithPassword(char const* url,
		       char const* username, char const* password,
		       Boolean allowKasennaProtocol, int timeout) {
  Authenticator authenticator;
  authenticator.setUsernameAndPassword(username, password);
  char* describeResult = describeURL(url, &authenticator, allowKasennaProtocol, timeout);
  if (describeResult != NULL) {
    // We are already authorized
    return describeResult;
  }

  // The "realm" field should have been filled in:
  if (authenticator.realm() == NULL) {
    // We haven't been given enough information to try again, so fail:
    return NULL;
  }

  // Try again:
  describeResult = describeURL(url, &authenticator, allowKasennaProtocol, timeout);
  if (describeResult != NULL) {
    // The authenticator worked, so use it in future requests:
    fCurrentAuthenticator = authenticator;
  }

  return describeResult;
}

char* RTSPClient::sendOptionsCmd(char const* url,
				 char* username, char* password,
				 Authenticator* authenticator,
				 int timeout) {
  char* result = NULL;
  char* cmd = NULL;
  Boolean haveAllocatedAuthenticator = False;
  do {
    if (authenticator == NULL) {
      // First, check whether "url" contains a username:password to be used
      // (and no username,password pair was supplied separately):
      if (username == NULL && password == NULL
	  && parseRTSPURLUsernamePassword(url, username, password)) {
	Authenticator newAuthenticator;
	newAuthenticator.setUsernameAndPassword(username, password);
	result = sendOptionsCmd(url, username, password, &newAuthenticator, timeout);
	delete[] username; delete[] password; // they were dynamically allocated
	break;
      } else if (username != NULL && password != NULL) {
	// Use the separately supplied username and password:
	authenticator = new Authenticator;
	haveAllocatedAuthenticator = True;
	authenticator->setUsernameAndPassword(username, password);

	result = sendOptionsCmd(url, username, password, authenticator, timeout);
	if (result != NULL) break; // We are already authorized

	// The "realm" field should have been filled in:
	if (authenticator->realm() == NULL) {
	  // We haven't been given enough information to try again, so fail:
	  break;
	}
	// Try again:
      }
    }

    if (!openConnectionFromURL(url, authenticator, timeout)) break;

    // Send the OPTIONS command:

    // First, construct an authenticator string:
    char* authenticatorStr
      = createAuthenticatorString(authenticator, "OPTIONS", url);

    char const* const cmdFmt =
      "OPTIONS %s RTSP/1.0\r\n"
      "CSeq: %d\r\n"
      "%s"
      "%s"
#ifdef SUPPORT_REAL_RTSP
      REAL_OPTIONS_HEADERS
#endif
      "\r\n";
    unsigned cmdSize = strlen(cmdFmt)
      + strlen(url)
      + 20 /* max int len */
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrSize;
    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt,
	    url,
	    ++fCSeq,
	    authenticatorStr,
	    fUserAgentHeaderStr);
    delete[] authenticatorStr;

    if (!sendRequest(cmd, "OPTIONS")) break;

    // Get the response from the server:
    unsigned bytesRead; unsigned responseCode;
    char* firstLine; char* nextLineStart;
    if (!getResponse("OPTIONS", bytesRead, responseCode, firstLine, nextLineStart,
		     False /*don't check for response code 200*/)) break;
    if (responseCode != 200) {
      checkForAuthenticationFailure(responseCode, nextLineStart, authenticator);
      envir().setResultMsg("cannot handle OPTIONS response: ", firstLine);
      break;
    }

    // Look for a "Public:" header (which will contain our result str):
    char* lineStart;
    while (1) {
      lineStart = nextLineStart;
      if (lineStart == NULL) break;

      nextLineStart = getLine(lineStart);

      if (_strncasecmp(lineStart, "Public: ", 8) == 0) {
	delete[] result; result = strDup(&lineStart[8]);
#ifdef SUPPORT_REAL_RTSP
      } else if (_strncasecmp(lineStart, "RealChallenge1: ", 16) == 0) {
	delete[] fRealChallengeStr; fRealChallengeStr = strDup(&lineStart[16]);
#endif
      }
    }
  } while (0);

  delete[] cmd;
  if (haveAllocatedAuthenticator) delete authenticator;
  return result;
}

static Boolean isAbsoluteURL(char const* url) {
  // Assumption: "url" is absolute if it contains a ':', before any
  // occurrence of '/'
  while (*url != '\0' && *url != '/') {
    if (*url == ':') return True;
    ++url;
  }

  return False;
}

char const* RTSPClient::sessionURL(MediaSession const& session) const {
  char const* url = session.controlPath();
  if (url == NULL || strcmp(url, "*") == 0) url = fBaseURL;

  return url;
}

void RTSPClient::constructSubsessionURL(MediaSubsession const& subsession,
					char const*& prefix,
					char const*& separator,
					char const*& suffix) {
  // Figure out what the URL describing "subsession" will look like.
  // The URL is returned in three parts: prefix; separator; suffix
  //##### NOTE: This code doesn't really do the right thing if "sessionURL()"
  // doesn't end with a "/", and "subsession.controlPath()" is relative.
  // The right thing would have been to truncate "sessionURL()" back to the
  // rightmost "/", and then add "subsession.controlPath()".
  // In practice, though, each "DESCRIBE" response typically contains
  // a "Content-Base:" header that consists of "sessionURL()" followed by
  // a "/", in which case this code ends up giving the correct result.
  // However, we should really fix this code to do the right thing, and
  // also check for and use the "Content-Base:" header appropriately. #####
  prefix = sessionURL(subsession.parentSession());
  if (prefix == NULL) prefix = "";

  suffix = subsession.controlPath();
  if (suffix == NULL) suffix = "";

  if (isAbsoluteURL(suffix)) {
    prefix = separator = "";
  } else {
    unsigned prefixLen = strlen(prefix);
    separator = (prefix[prefixLen-1] == '/' || suffix[0] == '/') ? "" : "/";
  }
}

Boolean RTSPClient::announceSDPDescription(char const* url,
					   char const* sdpDescription,
					   Authenticator* authenticator,
					   int timeout) {
  char* cmd = NULL;
  do {
    if (!openConnectionFromURL(url, authenticator, timeout)) break;

    // Send the ANNOUNCE command:

    // First, construct an authenticator string:
    fCurrentAuthenticator.reset();
    char* authenticatorStr
      = createAuthenticatorString(authenticator, "ANNOUNCE", url);

    char const* const cmdFmt =
      "ANNOUNCE %s RTSP/1.0\r\n"
      "CSeq: %d\r\n"
      "Content-Type: application/sdp\r\n"
      "%s"
      "Content-length: %d\r\n\r\n"
      "%s";
	    // Note: QTSS hangs if an "ANNOUNCE" contains a "User-Agent:" field (go figure), so don't include one here
    unsigned sdpSize = strlen(sdpDescription);
    unsigned cmdSize = strlen(cmdFmt)
      + strlen(url)
      + 20 /* max int len */
      + strlen(authenticatorStr)
      + 20 /* max int len */
      + sdpSize;
    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt,
	    url,
	    ++fCSeq,
	    authenticatorStr,
	    sdpSize,
	    sdpDescription);
    delete[] authenticatorStr;

    if (!sendRequest(cmd, "ANNOUNCE")) break;

    // Get the response from the server:
    unsigned bytesRead; unsigned responseCode;
    char* firstLine; char* nextLineStart;
    if (!getResponse("ANNOUNCE", bytesRead, responseCode, firstLine, nextLineStart,
		     False /*don't check for response code 200*/)) break;

    // Inspect the first line to check whether it's a result code 200
    if (responseCode != 200) {
      checkForAuthenticationFailure(responseCode, nextLineStart, authenticator);
      envir().setResultMsg("cannot handle ANNOUNCE response: ", firstLine);
      break;
    }

    delete[] cmd;
    return True;
  } while (0);

  delete[] cmd;
  return False;
}

Boolean RTSPClient
::announceWithPassword(char const* url, char const* sdpDescription,
		       char const* username, char const* password, int timeout) {
  Authenticator authenticator;
  authenticator.setUsernameAndPassword(username, password);
  if (announceSDPDescription(url, sdpDescription, &authenticator, timeout)) {
    // We are already authorized
    return True;
  }

  // The "realm" field should have been filled in:
  if (authenticator.realm() == NULL) {
    // We haven't been given enough information to try again, so fail:
    return False;
  }

  // Try again:
  Boolean secondTrySuccess
    = announceSDPDescription(url, sdpDescription, &authenticator, timeout);

  if (secondTrySuccess) {
    // The authenticator worked, so use it in future requests:
    fCurrentAuthenticator = authenticator;
  }

  return secondTrySuccess;
}

Boolean RTSPClient::setupMediaSubsession(MediaSubsession& subsession,
					 Boolean streamOutgoing,
					 Boolean streamUsingTCP,
					 Boolean forceMulticastOnUnspecified) {
  char* cmd = NULL;
  char* setupStr = NULL;

  if (fServerIsMicrosoft) {
    // Microsoft doesn't send the right endTime on live streams.  Correct this:
    char *tmpStr = subsession.parentSession().mediaSessionType();
    if (tmpStr != NULL && strncmp(tmpStr, "broadcast", 9) == 0) {
      subsession.parentSession().playEndTime() = 0.0;
    }
  }

  do {
    // Construct the SETUP command:

    // First, construct an authenticator string:
    char* authenticatorStr
      = createAuthenticatorString(&fCurrentAuthenticator,
				  "SETUP", fBaseURL);

    // When sending more than one "SETUP" request, include a "Session:"
    // header in the 2nd and later "SETUP"s.
    char* sessionStr;
    if (fLastSessionId != NULL) {
      sessionStr = new char[20+strlen(fLastSessionId)];
      sprintf(sessionStr, "Session: %s\r\n", fLastSessionId);
    } else {
      sessionStr = strDup("");
    }

    char* transportStr = NULL;
#ifdef SUPPORT_REAL_RTSP
    if (usingRealNetworksChallengeResponse()) {
      // Use a special "Transport:" header, and also add a 'challenge response'.
      char challenge2[64];
      char checksum[34];
      RealCalculateChallengeResponse(fRealChallengeStr, challenge2, checksum);

      char const* etag = fRealETagStr == NULL ? "" : fRealETagStr;

      char* transportHeader;
      if (subsession.parentSession().isRealNetworksRDT) {
	transportHeader = strDup("Transport: x-pn-tng/tcp;mode=play,rtp/avp/unicast;mode=play\r\n");
      } else {
	// Use a regular "Transport:" header:
	char const* transportHeaderFmt
	  = "Transport: RTP/AVP%s%s=%d-%d\r\n";
	char const* transportTypeStr;
	char const* portTypeStr;
	unsigned short rtpNumber, rtcpNumber;
	if (streamUsingTCP) { // streaming over the RTSP connection
	  transportTypeStr = "/TCP;unicast";
	  portTypeStr = ";interleaved";
	  rtpNumber = fTCPStreamIdCount++;
	  rtcpNumber = fTCPStreamIdCount++;
	} else { // normal RTP streaming
	  unsigned connectionAddress = subsession.connectionEndpointAddress();
	  Boolean requestMulticastStreaming = IsMulticastAddress(connectionAddress)
	    || (connectionAddress == 0 && forceMulticastOnUnspecified);
	  transportTypeStr = requestMulticastStreaming ? ";multicast" : ";unicast";
	  portTypeStr = ";client_port";
	  rtpNumber = subsession.clientPortNum();
	  if (rtpNumber == 0) {
	    envir().setResultMsg("Client port number unknown\n");
	    break;
	  }
	  rtcpNumber = rtpNumber + 1;
	}

	unsigned transportHeaderSize = strlen(transportHeaderFmt)
	  + strlen(transportTypeStr) + strlen(portTypeStr) + 2*5 /* max port len */;
	transportHeader = new char[transportHeaderSize];
	sprintf(transportHeader, transportHeaderFmt,
		transportTypeStr, portTypeStr, rtpNumber, rtcpNumber);
      }
      char const* transportFmt =
	"%s"
	"RealChallenge2: %s, sd=%s\r\n"
	"If-Match: %s\r\n";
      unsigned transportSize = strlen(transportFmt)
	+ strlen(transportHeader)
	+ sizeof challenge2 + sizeof checksum
	+ strlen(etag);
      transportStr = new char[transportSize];
      sprintf(transportStr, transportFmt,
	      transportHeader,
	      challenge2, checksum,
	      etag);
      delete[] transportHeader;

      if (subsession.parentSession().isRealNetworksRDT) {
	// Also, tell the RDT source to use the RTSP TCP socket:
	RealRDTSource* rdtSource
	  = (RealRDTSource*)(subsession.readSource());
	rdtSource->setInputSocket(fInputSocketNum);
      }
    }
#endif

    char const *prefix, *separator, *suffix;
    constructSubsessionURL(subsession, prefix, separator, suffix);
    char const* transportFmt;

    if (strcmp(subsession.protocolName(), "UDP") == 0) {
      char const* setupFmt = "SETUP %s%s RTSP/1.0\r\n";
      unsigned setupSize = strlen(setupFmt)
        + strlen(prefix) + strlen (separator);
      setupStr = new char[setupSize];
      sprintf(setupStr, setupFmt, prefix, separator);

      transportFmt = "Transport: RAW/RAW/UDP%s%s%s=%d-%d\r\n";
    } else {
      char const* setupFmt = "SETUP %s%s%s RTSP/1.0\r\n";
      unsigned setupSize = strlen(setupFmt)
        + strlen(prefix) + strlen (separator) + strlen(suffix);
      setupStr = new char[setupSize];
      sprintf(setupStr, setupFmt, prefix, separator, suffix);

      transportFmt = "Transport: RTP/AVP%s%s%s=%d-%d\r\n";
    }

    if (transportStr == NULL) {
      // Construct a "Transport:" header.
      char const* transportTypeStr;
      char const* modeStr = streamOutgoing ? ";mode=receive" : "";
          // Note: I think the above is nonstandard, but DSS wants it this way
      char const* portTypeStr;
      unsigned short rtpNumber, rtcpNumber;
      if (streamUsingTCP) { // streaming over the RTSP connection
	transportTypeStr = "/TCP;unicast";
	portTypeStr = ";interleaved";
	rtpNumber = fTCPStreamIdCount++;
	rtcpNumber = fTCPStreamIdCount++;
      } else { // normal RTP streaming
	unsigned connectionAddress = subsession.connectionEndpointAddress();
	Boolean requestMulticastStreaming = IsMulticastAddress(connectionAddress)
	  || (connectionAddress == 0 && forceMulticastOnUnspecified);
	transportTypeStr = requestMulticastStreaming ? ";multicast" : ";unicast";
	portTypeStr = ";client_port";
	rtpNumber = subsession.clientPortNum();
	if (rtpNumber == 0) {
	  envir().setResultMsg("Client port number unknown\n");
	  delete[] authenticatorStr; delete[] sessionStr; delete[] setupStr;
	  break;
	}
	rtcpNumber = rtpNumber + 1;
      }

      unsigned transportSize = strlen(transportFmt)
	+ strlen(transportTypeStr) + strlen(modeStr) + strlen(portTypeStr) + 2*5 /* max port len */;
      transportStr = new char[transportSize];
      sprintf(transportStr, transportFmt,
	      transportTypeStr, modeStr, portTypeStr, rtpNumber, rtcpNumber);
    }

    // (Later implement more, as specified in the RTSP spec, sec D.1 #####)
    char const* const cmdFmt =
      "%s"
      "CSeq: %d\r\n"
      "%s"
      "%s"
      "%s"
      "%s"
      "\r\n";

    unsigned cmdSize = strlen(cmdFmt)
      + strlen(setupStr)
      + 20 /* max int len */
      + strlen(transportStr)
      + strlen(sessionStr)
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrSize;
    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt,
	    setupStr,
	    ++fCSeq,
	    transportStr,
	    sessionStr,
	    authenticatorStr,
	    fUserAgentHeaderStr);
    delete[] authenticatorStr; delete[] sessionStr; delete[] setupStr; delete[] transportStr;

    // And then send it:
    if (!sendRequest(cmd, "SETUP")) break;

    // Get the response from the server:
    unsigned bytesRead; unsigned responseCode;
    char* firstLine; char* nextLineStart;
    if (!getResponse("SETUP", bytesRead, responseCode, firstLine, nextLineStart)) break;

    // Look for a "Session:" header (to set our session id), and
    // a "Transport: " header (to set the server address/port)
    // For now, ignore other headers.
    char* lineStart;
    char* sessionId = new char[fResponseBufferSize]; // ensures we have enough space
    unsigned cLength = 0;
    while (1) {
      lineStart = nextLineStart;
      if (lineStart == NULL) break;

      nextLineStart = getLine(lineStart);

      if (sscanf(lineStart, "Session: %[^;]", sessionId) == 1) {
	subsession.sessionId = strDup(sessionId);
	delete[] fLastSessionId; fLastSessionId = strDup(sessionId);

	// Also look for an optional "; timeout = " parameter following this:
	char* afterSessionId
	  = lineStart + strlen(sessionId) + strlen ("Session: ");;
	int timeoutVal;
	if (sscanf(afterSessionId, "; timeout = %d", &timeoutVal) == 1) {
	  fSessionTimeoutParameter = timeoutVal;
	}
	continue;
      }

      char* serverAddressStr;
      portNumBits serverPortNum;
      unsigned char rtpChannelId, rtcpChannelId;
      if (parseTransportResponse(lineStart,
				 serverAddressStr, serverPortNum,
				 rtpChannelId, rtcpChannelId)) {
	delete[] subsession.connectionEndpointName();
	subsession.connectionEndpointName() = serverAddressStr;
	subsession.serverPortNum = serverPortNum;
	subsession.rtpChannelId = rtpChannelId;
	subsession.rtcpChannelId = rtcpChannelId;
	continue;
      }

      // Also check for a "Content-Length:" header.  Some weird servers include this
      // in the RTSP "SETUP" response.
      if (sscanf(lineStart, "Content-Length: %d", &cLength) == 1) continue;
    }
    delete[] sessionId;

    if (subsession.sessionId == NULL) {
      envir().setResultMsg("\"Session:\" header is missing in the response");
      break;
    }

    // If we saw a "Content-Length:" header in the response, then discard whatever
    // included data it refers to:
    if (cLength > 0) {
      char* dummyBuf = new char[cLength+1]; // allow for a trailing '\0'
      getResponse1(dummyBuf, cLength);
      delete[] dummyBuf;
    }

    if (streamUsingTCP) {
      // Tell the subsession to receive RTP (and send/receive RTCP)
      // over the RTSP stream:
      if (subsession.rtpSource() != NULL)
	subsession.rtpSource()->setStreamSocket(fInputSocketNum,
						subsession.rtpChannelId);
      if (subsession.rtcpInstance() != NULL)
	subsession.rtcpInstance()->setStreamSocket(fInputSocketNum,
						   subsession.rtcpChannelId);
    } else {
      // Normal case.
      // Set the RTP and RTCP sockets' destination address and port
      // from the information in the SETUP response (if present):
      netAddressBits destAddress = subsession.connectionEndpointAddress();
      if (destAddress == 0) destAddress = fServerAddress;
      subsession.setDestinations(destAddress);
    }

    delete[] cmd;
    return True;
  } while (0);

  delete[] cmd;
  return False;
}

static char* createScaleString(float scale, float currentScale) {
  char buf[100];
  if (scale == 1.0f && currentScale == 1.0f) {
    // This is the default value; we don't need a "Scale:" header:
    buf[0] = '\0';
  } else {
    Locale l("C", LC_NUMERIC);
    sprintf(buf, "Scale: %f\r\n", scale);
  }

  return strDup(buf);
}

static char* createRangeString(double start, double end) {
  char buf[100];
  if (start < 0) {
    // We're resuming from a PAUSE; there's no "Range:" header at all
    buf[0] = '\0';
  } else if (end < 0) {
    // There's no end time:
    Locale l("C", LC_NUMERIC);
    sprintf(buf, "Range: npt=%.3f-\r\n", start);
  } else {
    // There's both a start and an end time; include them both in the "Range:" hdr
    Locale l("C", LC_NUMERIC);
    sprintf(buf, "Range: npt=%.3f-%.3f\r\n", start, end);
  }

  return strDup(buf);
}

static char const* NoSessionErr = "No RTSP session is currently in progress\n";

Boolean RTSPClient::playMediaSession(MediaSession& session,
				     double start, double end, float scale) {
#ifdef SUPPORT_REAL_RTSP
  if (session.isRealNetworksRDT) {
    // This is a RealNetworks stream; set the "Subscribe" parameter before proceeding:
    char* streamRuleString = RealGetSubscribeRuleString(&session);
    setMediaSessionParameter(session, "Subscribe", streamRuleString);
    delete[] streamRuleString;
  }
#endif
  char* cmd = NULL;
  do {
    // First, make sure that we have a RTSP session in progress
    if (fLastSessionId == NULL) {
      envir().setResultMsg(NoSessionErr);
      break;
    }

    // Send the PLAY command:

    // First, construct an authenticator string:
    char* authenticatorStr
      = createAuthenticatorString(&fCurrentAuthenticator, "PLAY", fBaseURL);
    // And then a "Scale:" string:
    char* scaleStr = createScaleString(scale, session.scale());
    // And then a "Range:" string:
    char* rangeStr = createRangeString(start, end);

    char const* const cmdFmt =
      "PLAY %s RTSP/1.0\r\n"
      "CSeq: %d\r\n"
      "Session: %s\r\n"
      "%s"
      "%s"
      "%s"
      "%s"
      "\r\n";

    char const* sessURL = sessionURL(session);
    unsigned cmdSize = strlen(cmdFmt)
      + strlen(sessURL)
      + 20 /* max int len */
      + strlen(fLastSessionId)
      + strlen(scaleStr)
      + strlen(rangeStr)
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrSize;
    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt,
	    sessURL,
	    ++fCSeq,
	    fLastSessionId,
	    scaleStr,
	    rangeStr,
	    authenticatorStr,
	    fUserAgentHeaderStr);
    delete[] scaleStr;
    delete[] rangeStr;
    delete[] authenticatorStr;

    if (!sendRequest(cmd, "PLAY")) break;

    // Get the response from the server:
    unsigned bytesRead; unsigned responseCode;
    char* firstLine; char* nextLineStart;
    if (!getResponse("PLAY", bytesRead, responseCode, firstLine, nextLineStart)) break;

    // Look for various headers that we understand:
    char* lineStart;
    while (1) {
      lineStart = nextLineStart;
      if (lineStart == NULL) break;

      nextLineStart = getLine(lineStart);

      if (parseScaleHeader(lineStart, session.scale())) continue;
      if (parseRangeHeader(lineStart, session.playStartTime(), session.playEndTime())) continue;

      u_int16_t seqNum; u_int32_t timestamp;
      if (parseRTPInfoHeader(lineStart, seqNum, timestamp)) {
	// This is data for our first subsession.  Fill it in, and do the same for our other subsessions:
	MediaSubsessionIterator iter(session);
	MediaSubsession* subsession;
	while ((subsession = iter.next()) != NULL) {
	  subsession->rtpInfo.seqNum = seqNum;
	  subsession->rtpInfo.timestamp = timestamp;
	  subsession->rtpInfo.infoIsNew = True;

	  if (!parseRTPInfoHeader(lineStart, seqNum, timestamp)) break;
	}
	continue;
      }
    }

    if (fTCPStreamIdCount == 0) { // we're not receiving RTP-over-TCP
      // Arrange to handle incoming requests sent by the server
      envir().taskScheduler().turnOnBackgroundReadHandling(fInputSocketNum,
	   (TaskScheduler::BackgroundHandlerProc*)&incomingRequestHandler, this);
    }

    delete[] cmd;
    return True;
  } while (0);

  delete[] cmd;
  return False;
}

Boolean RTSPClient::playMediaSubsession(MediaSubsession& subsession,
					double start, double end, float scale,
					Boolean hackForDSS) {
  char* cmd = NULL;
  do {
    // First, make sure that we have a RTSP session in progress
    if (subsession.sessionId == NULL) {
      envir().setResultMsg(NoSessionErr);
      break;
    }

    // Send the PLAY command:

    // First, construct an authenticator string:
    char* authenticatorStr
      = createAuthenticatorString(&fCurrentAuthenticator, "PLAY", fBaseURL);
    // And then a "Scale:" string:
    char* scaleStr = createScaleString(scale, subsession.scale());
    // And then a "Range:" string:
    char* rangeStr = createRangeString(start, end);

    char const* const cmdFmt =
      "PLAY %s%s%s RTSP/1.0\r\n"
      "CSeq: %d\r\n"
      "Session: %s\r\n"
      "%s"
      "%s"
      "%s"
      "%s"
      "\r\n";

    char const *prefix, *separator, *suffix;
    constructSubsessionURL(subsession, prefix, separator, suffix);
    if (hackForDSS || fServerIsKasenna) {
      // When "PLAY" is used to inject RTP packets into a DSS
      // (violating the RTSP spec, btw; "RECORD" should have been used)
      // the DSS can crash (or hang) if the '/trackid=...' portion of
      // the URL is present.
      separator = suffix = "";
    }

    unsigned cmdSize = strlen(cmdFmt)
      + strlen(prefix) + strlen(separator) + strlen(suffix)
      + 20 /* max int len */
      + strlen(subsession.sessionId)
      + strlen(scaleStr)
      + strlen(rangeStr)
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrSize;
    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt,
	    prefix, separator, suffix,
	    ++fCSeq,
	    subsession.sessionId,
	    scaleStr,
	    rangeStr,
	    authenticatorStr,
	    fUserAgentHeaderStr);
    delete[] scaleStr;
    delete[] rangeStr;
    delete[] authenticatorStr;

    if (!sendRequest(cmd, "PLAY")) break;

    // Get the response from the server:
    unsigned bytesRead; unsigned responseCode;
    char* firstLine; char* nextLineStart;
    if (!getResponse("PLAY", bytesRead, responseCode, firstLine, nextLineStart)) break;

    // Look for various headers that we understand:
    char* lineStart;
    while (1) {
      lineStart = nextLineStart;
      if (lineStart == NULL) break;

      nextLineStart = getLine(lineStart);

      if (parseScaleHeader(lineStart, subsession.scale())) continue;
      if (parseRangeHeader(lineStart, subsession._playStartTime(), subsession._playEndTime())) continue;

      u_int16_t seqNum; u_int32_t timestamp;
      if (parseRTPInfoHeader(lineStart, seqNum, timestamp)) {
	subsession.rtpInfo.seqNum = seqNum;
	subsession.rtpInfo.timestamp = timestamp;
	subsession.rtpInfo.infoIsNew = True;
	continue;
      }
    }

    delete[] cmd;
    return True;
  } while (0);

  delete[] cmd;
  return False;
}

Boolean RTSPClient::pauseMediaSession(MediaSession& session) {
  char* cmd = NULL;
  do {
    // First, make sure that we have a RTSP session in progress
    if (fLastSessionId == NULL) {
      envir().setResultMsg(NoSessionErr);
      break;
    }

    // Send the PAUSE command:

    // First, construct an authenticator string:
    char* authenticatorStr
      = createAuthenticatorString(&fCurrentAuthenticator, "PAUSE", fBaseURL);

    char const* sessURL = sessionURL(session);
    char const* const cmdFmt =
      "PAUSE %s RTSP/1.0\r\n"
      "CSeq: %d\r\n"
      "Session: %s\r\n"
      "%s"
      "%s"
      "\r\n";

    unsigned cmdSize = strlen(cmdFmt)
      + strlen(sessURL)
      + 20 /* max int len */
      + strlen(fLastSessionId)
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrSize;
    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt,
	    sessURL,
	    ++fCSeq,
	    fLastSessionId,
	    authenticatorStr,
	    fUserAgentHeaderStr);
    delete[] authenticatorStr;

    if (!sendRequest(cmd, "PAUSE")) break;

    if (fTCPStreamIdCount == 0) { // When TCP streaming, don't look for a response
      // Get the response from the server:
      unsigned bytesRead; unsigned responseCode;
      char* firstLine; char* nextLineStart;
      if (!getResponse("PAUSE", bytesRead, responseCode, firstLine, nextLineStart)) break;
    }

    delete[] cmd;
    return True;
  } while (0);

  delete[] cmd;
  return False;
}

Boolean RTSPClient::pauseMediaSubsession(MediaSubsession& subsession) {
  char* cmd = NULL;
  do {
    // First, make sure that we have a RTSP session in progress
    if (subsession.sessionId == NULL) {
      envir().setResultMsg(NoSessionErr);
      break;
    }

    // Send the PAUSE command:

    // First, construct an authenticator string:
    char* authenticatorStr
      = createAuthenticatorString(&fCurrentAuthenticator, "PAUSE", fBaseURL);

    char const* const cmdFmt =
      "PAUSE %s%s%s RTSP/1.0\r\n"
      "CSeq: %d\r\n"
      "Session: %s\r\n"
      "%s"
      "%s"
      "\r\n";

    char const *prefix, *separator, *suffix;
    constructSubsessionURL(subsession, prefix, separator, suffix);
    if (fServerIsKasenna) separator = suffix = "";

    unsigned cmdSize = strlen(cmdFmt)
      + strlen(prefix) + strlen(separator) + strlen(suffix)
      + 20 /* max int len */
      + strlen(subsession.sessionId)
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrSize;
    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt,
	    prefix, separator, suffix,
	    ++fCSeq,
	    subsession.sessionId,
	    authenticatorStr,
	    fUserAgentHeaderStr);
    delete[] authenticatorStr;

    if (!sendRequest(cmd, "PAUSE")) break;

    if (fTCPStreamIdCount == 0) { // When TCP streaming, don't look for a response
      // Get the response from the server:
      unsigned bytesRead; unsigned responseCode;
      char* firstLine; char* nextLineStart;
      if (!getResponse("PAUSE", bytesRead, responseCode, firstLine, nextLineStart)) break;
    }

    delete[] cmd;
    return True;
  } while (0);

  delete[] cmd;
  return False;
}

Boolean RTSPClient::recordMediaSubsession(MediaSubsession& subsession) {
  char* cmd = NULL;
  do {
    // First, make sure that we have a RTSP session in progress
    if (subsession.sessionId == NULL) {
      envir().setResultMsg(NoSessionErr);
      break;
    }

    // Send the RECORD command:

    // First, construct an authenticator string:
    char* authenticatorStr
      = createAuthenticatorString(&fCurrentAuthenticator,
				  "RECORD", fBaseURL);

    char const* const cmdFmt =
      "RECORD %s%s%s RTSP/1.0\r\n"
      "CSeq: %d\r\n"
      "Session: %s\r\n"
      "Range: npt=0-\r\n"
      "%s"
      "%s"
      "\r\n";

    char const *prefix, *separator, *suffix;
    constructSubsessionURL(subsession, prefix, separator, suffix);

    unsigned cmdSize = strlen(cmdFmt)
      + strlen(prefix) + strlen(separator) + strlen(suffix)
      + 20 /* max int len */
      + strlen(subsession.sessionId)
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrSize;
    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt,
	    prefix, separator, suffix,
	    ++fCSeq,
	    subsession.sessionId,
	    authenticatorStr,
	    fUserAgentHeaderStr);
    delete[] authenticatorStr;

    if (!sendRequest(cmd, "RECORD")) break;

    // Get the response from the server:
    unsigned bytesRead; unsigned responseCode;
    char* firstLine; char* nextLineStart;
    if (!getResponse("RECORD", bytesRead, responseCode, firstLine, nextLineStart)) break;

    delete[] cmd;
    return True;
  } while (0);

  delete[] cmd;
  return False;
}

Boolean RTSPClient::setMediaSessionParameter(MediaSession& /*session*/,
					     char const* parameterName,
					     char const* parameterValue) {
  char* cmd = NULL;
  do {
    // First, make sure that we have a RTSP session in progress
    if (fLastSessionId == NULL) {
      envir().setResultMsg(NoSessionErr);
      break;
    }

    // Send the SET_PARAMETER command:

    // First, construct an authenticator string:
    char* authenticatorStr
      = createAuthenticatorString(&fCurrentAuthenticator,
				  "SET_PARAMETER", fBaseURL);

    char const* const cmdFmt =
      "SET_PARAMETER %s RTSP/1.0\r\n"
      "CSeq: %d\r\n"
      "Session: %s\r\n"
      "%s"
      "%s"
      "Content-length: %d\r\n\r\n"
      "%s: %s\r\n";

    unsigned parameterNameLen = strlen(parameterName);
    unsigned parameterValueLen = strlen(parameterValue);
    unsigned cmdSize = strlen(cmdFmt)
      + strlen(fBaseURL)
      + 20 /* max int len */
      + strlen(fLastSessionId)
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrSize
      + parameterNameLen + parameterValueLen;
    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt,
	    fBaseURL,
	    ++fCSeq,
	    fLastSessionId,
	    authenticatorStr,
	    fUserAgentHeaderStr,
	    parameterNameLen + parameterValueLen + 2, // the "+ 2" is for the \r\n after the parameter "name: value"
	    parameterName, parameterValue);
    delete[] authenticatorStr;

    if (!sendRequest(cmd, "SET_PARAMETER")) break;

    // Get the response from the server:
    unsigned bytesRead; unsigned responseCode;
    char* firstLine; char* nextLineStart;
    if (!getResponse("SET_PARAMETER", bytesRead, responseCode, firstLine, nextLineStart)) break;

    delete[] cmd;
    return True;
  } while (0);

  delete[] cmd;
  return False;
}

Boolean RTSPClient::getMediaSessionParameter(MediaSession& /*session*/,
					     char const* parameterName,
					     char*& parameterValue) {
  parameterValue = NULL; // default result
  Boolean const haveParameterName = parameterName != NULL && parameterName[0] != '\0';
  char* cmd = NULL;
  do {
    // First, make sure that we have a RTSP session in progress
    if (fLastSessionId == NULL) {
      envir().setResultMsg(NoSessionErr);
      break;
    }

    // Send the GET_PARAMETER command:
    // First, construct an authenticator string:
    char* authenticatorStr
      = createAuthenticatorString(&fCurrentAuthenticator,
				  "GET_PARAMETER", fBaseURL);

    if (haveParameterName) {
      char const* const cmdFmt =
	"GET_PARAMETER %s RTSP/1.0\r\n"
	"CSeq: %d\r\n"
	"Session: %s\r\n"
	"%s"
	"%s"
	"Content-type: text/parameters\r\n"
	"Content-length: %d\r\n\r\n"
	"%s\r\n";

      unsigned parameterNameLen = strlen(parameterName);
      unsigned cmdSize = strlen(cmdFmt)
	+ strlen(fBaseURL)
	+ 20 /* max int len */
	+ strlen(fLastSessionId)
	+ strlen(authenticatorStr)
	+ fUserAgentHeaderStrSize
	+ parameterNameLen;
      cmd = new char[cmdSize];
      sprintf(cmd, cmdFmt,
	      fBaseURL,
	      ++fCSeq,
	      fLastSessionId,
	      authenticatorStr,
	      fUserAgentHeaderStr,
	      parameterNameLen + 2, // the "+ 2" is for the \r\n after the parameter name
	      parameterName);
    } else {
      char const* const cmdFmt =
	"GET_PARAMETER %s RTSP/1.0\r\n"
	"CSeq: %d\r\n"
	"Session: %s\r\n"
	"%s"
	"%s"
	"\r\n";

      unsigned cmdSize = strlen(cmdFmt)
	+ strlen(fBaseURL)
	+ 20 /* max int len */
	+ strlen(fLastSessionId)
	+ strlen(authenticatorStr)
	+ fUserAgentHeaderStrSize;
      cmd = new char[cmdSize];
      sprintf(cmd, cmdFmt,
	      fBaseURL,
	      ++fCSeq,
	      fLastSessionId,
	      authenticatorStr,
	      fUserAgentHeaderStr);
    }
    delete[] authenticatorStr;

    if (!sendRequest(cmd, "GET_PARAMETER")) break;

    // Get the response from the server:
    // This section was copied/modified from the RTSPClient::describeURL func
    unsigned bytesRead; unsigned responseCode;
    char* firstLine; char* nextLineStart;
    if (!getResponse("GET_PARAMETER", bytesRead, responseCode, firstLine,
            nextLineStart, False /*don't check for response code 200*/)) break;

    // Inspect the first line to check whether it's a result code that
    // we can handle.
    if (responseCode != 200) {
      envir().setResultMsg("cannot handle GET_PARAMETER response: ", firstLine);
      break;
    }

    // Skip every subsequent header line, until we see a blank line
    // The remaining data is assumed to be the parameter data that we want.
    char* serverType = new char[fResponseBufferSize]; // ensures enough space
    int contentLength = -1;
    char* lineStart;
    while (1) {
      lineStart = nextLineStart;
      if (lineStart == NULL) break;

      nextLineStart = getLine(lineStart);
      if (lineStart[0] == '\0') break; // this is a blank line

      if (sscanf(lineStart, "Content-Length: %d", &contentLength) == 1
	  || sscanf(lineStart, "Content-length: %d", &contentLength) == 1) {
	if (contentLength < 0) {
	  envir().setResultMsg("Bad \"Content-length:\" header: \"",
			       lineStart, "\"");
	  break;
	}
      }
    }
    delete[] serverType;

    // We're now at the end of the response header lines
    if (lineStart == NULL) {
      envir().setResultMsg("no content following header lines: ",
                            fResponseBuffer);
      break;
    }

    // Use the remaining data as the parameter data, but first, check
    // the "Content-length:" header (if any) that we saw.  We may need to
    // read more data, or we may have extraneous data in the buffer.
    char* bodyStart = nextLineStart;
    if (contentLength >= 0) {
      // We saw a "Content-length:" header
      unsigned numBodyBytes = &firstLine[bytesRead] - bodyStart;
      if (contentLength > (int)numBodyBytes) {
	// We need to read more data.  First, make sure we have enough
	// space for it:
	unsigned numExtraBytesNeeded = contentLength - numBodyBytes;
	unsigned remainingBufferSize
	  = fResponseBufferSize - (bytesRead + (firstLine - fResponseBuffer));
	if (numExtraBytesNeeded > remainingBufferSize) {
	  char tmpBuf[200];
	  sprintf(tmpBuf, "Read buffer size (%d) is too small for \"Content-length:\" %d (need a buffer size of >= %d bytes\n",
		  fResponseBufferSize, contentLength,
		  fResponseBufferSize + numExtraBytesNeeded - remainingBufferSize);
	  envir().setResultMsg(tmpBuf);
	  break;
	}

	// Keep reading more data until we have enough:
	if (fVerbosityLevel >= 1) {
	  envir() << "Need to read " << numExtraBytesNeeded
		  << " extra bytes\n";
	}
	while (numExtraBytesNeeded > 0) {
	  struct sockaddr_in fromAddress;
	  char* ptr = &firstLine[bytesRead];
	  int bytesRead2 = readSocket(envir(), fInputSocketNum, (unsigned char*)ptr,
				      numExtraBytesNeeded, fromAddress);
	  if (bytesRead2 <= 0) break;
	  ptr[bytesRead2] = '\0';
	  if (fVerbosityLevel >= 1) {
	    envir() << "Read " << bytesRead2 << " extra bytes: "
		    << ptr << "\n";
	  }

	  bytesRead += bytesRead2;
	  numExtraBytesNeeded -= bytesRead2;
	}
	if (numExtraBytesNeeded > 0) break; // one of the reads failed
      }
    }

    if (haveParameterName
	&& !parseGetParameterHeader(bodyStart, parameterName, parameterValue)) break;

    delete[] cmd;
    return True;
  } while (0);

  delete[] cmd;
  return False;
}

Boolean RTSPClient::teardownMediaSession(MediaSession& session) {
  char* cmd = NULL;
  do {
    // First, make sure that we have a RTSP session in progreee
    if (fLastSessionId == NULL) {
      envir().setResultMsg(NoSessionErr);
      break;
    }

    // Send the TEARDOWN command:

    // First, construct an authenticator string:
    char* authenticatorStr
      = createAuthenticatorString(&fCurrentAuthenticator,
				  "TEARDOWN", fBaseURL);

    char const* sessURL = sessionURL(session);
    char const* const cmdFmt =
      "TEARDOWN %s RTSP/1.0\r\n"
      "CSeq: %d\r\n"
      "Session: %s\r\n"
      "%s"
      "%s"
      "\r\n";

    unsigned cmdSize = strlen(cmdFmt)
      + strlen(sessURL)
      + 20 /* max int len */
      + strlen(fLastSessionId)
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrSize;
    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt,
	    sessURL,
	    ++fCSeq,
	    fLastSessionId,
	    authenticatorStr,
	    fUserAgentHeaderStr);
    delete[] authenticatorStr;

    if (!sendRequest(cmd, "TEARDOWN")) break;

    if (fTCPStreamIdCount == 0) { // When TCP streaming, don't look for a response
      // Get the response from the server:
      unsigned bytesRead; unsigned responseCode;
      char* firstLine; char* nextLineStart;
      getResponse("TEARDOWN", bytesRead, responseCode, firstLine, nextLineStart); // ignore the response; from our POV, we're done

      // Run through each subsession, deleting its "sessionId":
      MediaSubsessionIterator iter(session);
      MediaSubsession* subsession;
      while ((subsession = iter.next()) != NULL) {
	delete[] (char*)subsession->sessionId;
	subsession->sessionId = NULL;
      }

      delete[] fLastSessionId; fLastSessionId = NULL;
      // we're done with this session
    }

    delete[] cmd;
    return True;
  } while (0);

  delete[] cmd;
  return False;
}

Boolean RTSPClient::teardownMediaSubsession(MediaSubsession& subsession) {
  char* cmd = NULL;
  do {
    // First, make sure that we have a RTSP session in progreee
    if (subsession.sessionId == NULL) {
      envir().setResultMsg(NoSessionErr);
      break;
    }

    // Send the TEARDOWN command:

    // First, construct an authenticator string:
    char* authenticatorStr
      = createAuthenticatorString(&fCurrentAuthenticator,
				  "TEARDOWN", fBaseURL);

    char const* const cmdFmt =
      "TEARDOWN %s%s%s RTSP/1.0\r\n"
      "CSeq: %d\r\n"
      "Session: %s\r\n"
      "%s"
      "%s"
      "\r\n";

    char const *prefix, *separator, *suffix;
    constructSubsessionURL(subsession, prefix, separator, suffix);

    unsigned cmdSize = strlen(cmdFmt)
      + strlen(prefix) + strlen(separator) + strlen(suffix)
      + 20 /* max int len */
      + strlen(subsession.sessionId)
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrSize;
    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt,
	    prefix, separator, suffix,
	    ++fCSeq,
	    subsession.sessionId,
	    authenticatorStr,
	    fUserAgentHeaderStr);
    delete[] authenticatorStr;

    if (!sendRequest(cmd, "TEARDOWN")) break;

    if (fTCPStreamIdCount == 0) { // When TCP streaming, don't look for a response
      // Get the response from the server:
      unsigned bytesRead; unsigned responseCode;
      char* firstLine; char* nextLineStart;
      getResponse("TEARDOWN", bytesRead, responseCode, firstLine, nextLineStart); // ignore the response; from our POV, we're done
    }

    delete[] (char*)subsession.sessionId;
    subsession.sessionId = NULL;
    // we're done with this session

    delete[] cmd;
    return True;
  } while (0);

  delete[] cmd;
  return False;
}

Boolean RTSPClient
::openConnectionFromURL(char const* url, Authenticator* authenticator, int timeout) {
  do {
    // Set this as our base URL:
    delete[] fBaseURL; fBaseURL = strDup(url); if (fBaseURL == NULL) break;

    // Begin by parsing the URL:

    NetAddress destAddress;
    portNumBits urlPortNum;
    char const* urlSuffix;
    if (!parseRTSPURL(envir(), url, destAddress, urlPortNum, &urlSuffix)) break;
    portNumBits destPortNum
      = fTunnelOverHTTPPortNum == 0 ? urlPortNum : fTunnelOverHTTPPortNum;

    if (fInputSocketNum < 0) {
      // We don't yet have a TCP socket.  Set one up (blocking) now:
      fInputSocketNum = fOutputSocketNum
	= setupStreamSocket(envir(), 0, False /* =>blocking */);
      if (fInputSocketNum < 0) break;
      
      // Connect to the remote endpoint:
      fServerAddress = *(unsigned*)(destAddress.data());
      MAKE_SOCKADDR_IN(remoteName, fServerAddress, htons(destPortNum));
      fd_set set;
      FD_ZERO(&set);
      timeval tvout = {0,0};
      
      // If we were supplied with a timeout, make our socket temporarily non-blocking
      if (timeout > 0) {
	FD_SET((unsigned)fInputSocketNum, &set);
	tvout.tv_sec = timeout;
	tvout.tv_usec = 0;
	makeSocketNonBlocking(fInputSocketNum);
      }
      if (connect(fInputSocketNum, (struct sockaddr*) &remoteName, sizeof remoteName) != 0) {
	if (envir().getErrno() != EINPROGRESS && envir().getErrno() != EWOULDBLOCK) {
	  envir().setResultErrMsg("connect() failed: ");
	  break;
	}
	if (timeout > 0 && (select(fInputSocketNum + 1, NULL, &set, NULL, &tvout) <= 0)) {
	  envir().setResultErrMsg("select/connect() failed: ");
	  break;
	}
      }
      // If we set our socket to non-blocking, put it back in blocking mode now.
      if (timeout > 0) {
	makeSocketBlocking(fInputSocketNum);
      }
      
      if (fTunnelOverHTTPPortNum != 0 && !setupHTTPTunneling(urlSuffix, authenticator)) break;
    }
    
    return True;
  } while (0);
  
  fDescribeStatusCode = 1;
  resetTCPSockets();
  return False;
}

Boolean RTSPClient::parseRTSPURL(UsageEnvironment& env, char const* url,
				 NetAddress& address,
				 portNumBits& portNum,
				 char const** urlSuffix) {
  do {
    // Parse the URL as "rtsp://<address>:<port>/<etc>"
    // (with ":<port>" and "/<etc>" optional)
    // Also, skip over any "<username>[:<password>]@" preceding <address>
    char const* prefix = "rtsp://";
    unsigned const prefixLength = 7;
    if (_strncasecmp(url, prefix, prefixLength) != 0) {
      env.setResultMsg("URL is not of the form \"", prefix, "\"");
      break;
    }

    unsigned const parseBufferSize = 100;
    char parseBuffer[parseBufferSize];
    char const* from = &url[prefixLength];

    // Skip over any "<username>[:<password>]@"
    // (Note that this code fails if <password> contains '@' or '/', but
    // given that these characters can also appear in <etc>, there seems to
    // be no way of unambiguously parsing that situation.)
    char const* from1 = from;
    while (*from1 != '\0' && *from1 != '/') {
      if (*from1 == '@') {
	from = ++from1;
	break;
      }
      ++from1;
    }

    char* to = &parseBuffer[0];
    unsigned i;
    for (i = 0; i < parseBufferSize; ++i) {
      if (*from == '\0' || *from == ':' || *from == '/') {
	// We've completed parsing the address
	*to = '\0';
	break;
      }
      *to++ = *from++;
    }
    if (i == parseBufferSize) {
      env.setResultMsg("URL is too long");
      break;
    }

    NetAddressList addresses(parseBuffer);
    if (addresses.numAddresses() == 0) {
      env.setResultMsg("Failed to find network address for \"",
		       parseBuffer, "\"");
      break;
    }
    address = *(addresses.firstAddress());

    portNum = 554; // default value
    char nextChar = *from;
    if (nextChar == ':') {
      int portNumInt;
      if (sscanf(++from, "%d", &portNumInt) != 1) {
	env.setResultMsg("No port number follows ':'");
	break;
      }
      if (portNumInt < 1 || portNumInt > 65535) {
	env.setResultMsg("Bad port number");
	break;
      }
      portNum = (portNumBits)portNumInt;
      while (*from >= '0' && *from <= '9') ++from; // skip over port number
    }

    // The remainder of the URL is the suffix:
    if (urlSuffix != NULL) *urlSuffix = from;

    return True;
  } while (0);

  return False;
}

Boolean RTSPClient::parseRTSPURLUsernamePassword(char const* url,
						 char*& username,
						 char*& password) {
  username = password = NULL; // by default
  do {
    // Parse the URL as "rtsp://<username>[:<password>]@<whatever>"
    char const* prefix = "rtsp://";
    unsigned const prefixLength = 7;
    if (_strncasecmp(url, prefix, prefixLength) != 0) break;

    // Look for the ':' and '@':
    unsigned usernameIndex = prefixLength;
    unsigned colonIndex = 0, atIndex = 0;
    for (unsigned i = usernameIndex; url[i] != '\0' && url[i] != '/'; ++i) {
      if (url[i] == ':' && colonIndex == 0) {
	colonIndex = i;
      } else if (url[i] == '@') {
	atIndex = i;
	break; // we're done
      }
    }
    if (atIndex == 0) break; // no '@' found

    char* urlCopy = strDup(url);
    urlCopy[atIndex] = '\0';
    if (colonIndex > 0) {
      urlCopy[colonIndex] = '\0';
      password = strDup(&urlCopy[colonIndex+1]);
    } else {
      password = strDup("");
    }
    username = strDup(&urlCopy[usernameIndex]);
    delete[] urlCopy;

    return True;
  } while (0);

  return False;
}

char*
RTSPClient::createAuthenticatorString(Authenticator const* authenticator,
				      char const* cmd, char const* url) {
  if (authenticator != NULL && authenticator->realm() != NULL
      && authenticator->username() != NULL && authenticator->password() != NULL) {
    // We've been provided a filled-in authenticator, so use it:
    char* authenticatorStr;
    if (authenticator->nonce() != NULL) { // Digest authentication
      char const* const authFmt =
	"Authorization: Digest username=\"%s\", realm=\"%s\", "
	"nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n";
      char const* response = authenticator->computeDigestResponse(cmd, url);
      unsigned authBufSize = strlen(authFmt)
	+ strlen(authenticator->username()) + strlen(authenticator->realm())
	+ strlen(authenticator->nonce()) + strlen(url) + strlen(response);
      authenticatorStr = new char[authBufSize];
      sprintf(authenticatorStr, authFmt,
	      authenticator->username(), authenticator->realm(),
	      authenticator->nonce(), url, response);
      authenticator->reclaimDigestResponse(response);
    } else { // Basic authentication
      char const* const authFmt = "Authorization: Basic %s\r\n";

      unsigned usernamePasswordLength = strlen(authenticator->username()) + 1 + strlen(authenticator->password());
      char* usernamePassword = new char[usernamePasswordLength+1];
      sprintf(usernamePassword, "%s:%s", authenticator->username(), authenticator->password());

      char* response = base64Encode(usernamePassword, usernamePasswordLength);
      unsigned const authBufSize = strlen(authFmt) + strlen(response) + 1;
      authenticatorStr = new char[authBufSize];
      sprintf(authenticatorStr, authFmt, response);
      delete[] response; delete[] usernamePassword;
    }

    return authenticatorStr;
  }

  return strDup("");
}

void RTSPClient::checkForAuthenticationFailure(unsigned responseCode,
					       char*& nextLineStart,
					       Authenticator* authenticator) {
  if (responseCode == 401 && authenticator != NULL) {
    // We have an authentication failure, so fill in "authenticator"
    // using the contents of a following "WWW-Authenticate:" line.
    // (Once we compute a 'response' for "authenticator", it can be
    //  used in a subsequent request - that will hopefully succeed.)
    char* lineStart;
    while (1) {
      lineStart = nextLineStart;
      if (lineStart == NULL) break;

      nextLineStart = getLine(lineStart);
      if (lineStart[0] == '\0') break; // this is a blank line

      char* realm = strDupSize(lineStart);
      char* nonce = strDupSize(lineStart);
      Boolean foundAuthenticateHeader = False;
      if (sscanf(lineStart, "WWW-Authenticate: Digest realm=\"%[^\"]\", nonce=\"%[^\"]\"",
		 realm, nonce) == 2) {
	authenticator->setRealmAndNonce(realm, nonce);
	foundAuthenticateHeader = True;
      } else if (sscanf(lineStart, "WWW-Authenticate: Basic realm=\"%[^\"]\"",
		 realm) == 1) {
	authenticator->setRealmAndNonce(realm, NULL); // Basic authentication
	foundAuthenticateHeader = True;
      }
      delete[] realm; delete[] nonce;
      if (foundAuthenticateHeader) break;
    }
  }
}

Boolean RTSPClient::sendRequest(char const* requestString, char const* tag,
				Boolean base64EncodeIfOverHTTP) {
  if (fVerbosityLevel >= 1) {
    envir() << "Sending request: " << requestString << "\n";
  }

  char* newRequestString = NULL;
  if (fTunnelOverHTTPPortNum != 0 && base64EncodeIfOverHTTP) {
    requestString = newRequestString = base64Encode(requestString, strlen(requestString));
    if (fVerbosityLevel >= 1) {
      envir() << "\tThe request was base-64 encoded to: " << requestString << "\n\n";
    }
  }

  Boolean result
    = send(fOutputSocketNum, requestString, strlen(requestString), 0) >= 0;
  delete[] newRequestString;

  if (!result) {
    if (tag == NULL) tag = "";
    char const* errFmt = "%s send() failed: ";
    unsigned const errLength = strlen(errFmt) + strlen(tag);
    char* err = new char[errLength];
    sprintf(err, errFmt, tag);
    envir().setResultErrMsg(err);
    delete[] err;
  }
  return result;
}

Boolean RTSPClient::getResponse(char const* tag,
				unsigned& bytesRead, unsigned& responseCode,
				char*& firstLine, char*& nextLineStart,
				Boolean checkFor200Response) {
  do {
    char* readBuf = fResponseBuffer;
    bytesRead = getResponse1(readBuf, fResponseBufferSize);
    if (bytesRead == 0) {
      envir().setResultErrMsg("Failed to read response: ");
      break;
    }
    if (fVerbosityLevel >= 1) {
      envir() << "Received " << tag << " response: " << readBuf << "\n";
    }

    firstLine = readBuf;
    nextLineStart = getLine(firstLine);
    if (!parseResponseCode(firstLine, responseCode)) break;


    if (responseCode != 200 && checkFor200Response) {
      envir().setResultMsg(tag, ": cannot handle response: ", firstLine);
      break;
    }

    return True;
  } while (0);

  // An error occurred:
  return False;
}

unsigned RTSPClient::getResponse1(char*& responseBuffer,
				  unsigned responseBufferSize) {
  struct sockaddr_in fromAddress;

  if (responseBufferSize == 0) return 0; // just in case...
  responseBuffer[0] = '\0'; // ditto

  // Begin by reading and checking the first byte of the response.
  // If it's '$', then there's an interleaved RTP (or RTCP)-over-TCP
  // packet here.  We need to read and discard it first.
  Boolean success = False;
  while (1) {
    unsigned char firstByte;
    struct timeval timeout;
    timeout.tv_sec = 30; timeout.tv_usec = 0;
    if (readSocket(envir(), fInputSocketNum, &firstByte, 1, fromAddress, &timeout)
	!= 1) break;
    if (firstByte != '$') {
      // Normal case: This is the start of a regular response; use it:
      responseBuffer[0] = firstByte;
      success = True;
      break;
    } else {
      // This is an interleaved packet; read and discard it:
      unsigned char streamChannelId;
      if (readSocket(envir(), fInputSocketNum, &streamChannelId, 1, fromAddress)
	  != 1) break;

      unsigned short size;
      if (readSocketExact(envir(), fInputSocketNum, (unsigned char*)&size, 2,
		     fromAddress) != 2) break;
      size = ntohs(size);
      if (fVerbosityLevel >= 1) {
	envir() << "Discarding interleaved RTP or RTCP packet ("
		<< size << " bytes, channel id "
		<< streamChannelId << ")\n";
      }

      unsigned char* tmpBuffer = new unsigned char[size];
      if (tmpBuffer == NULL) break;
      unsigned bytesRead = 0;
      unsigned bytesToRead = size;
      int curBytesRead;
      while ((curBytesRead = readSocket(envir(), fInputSocketNum,
					&tmpBuffer[bytesRead], bytesToRead,
					fromAddress)) > 0) {
	bytesRead += curBytesRead;
	if (bytesRead >= size) break;
	bytesToRead -= curBytesRead;
      }
      delete[] tmpBuffer;
      if (bytesRead != size) break;

      success = True;
    }
  }
  if (!success) return 0;

  // Keep reading data from the socket until we see "\r\n\r\n" (except
  // at the start), or until we fill up our buffer.
  // Don't read any more than this.
  char* p = responseBuffer;
  Boolean haveSeenNonCRLF = False;
  int bytesRead = 1; // because we've already read the first byte
  while (bytesRead < (int)responseBufferSize) {
    int bytesReadNow
      = readSocket(envir(), fInputSocketNum,
		   (unsigned char*)(responseBuffer+bytesRead),
		   1, fromAddress);
    if (bytesReadNow <= 0) {
      envir().setResultMsg("RTSP response was truncated");
      break;
    }
    bytesRead += bytesReadNow;

    // Check whether we have "\r\n\r\n" (or "\r\r" or "\n\n"):
    char* lastToCheck = responseBuffer+bytesRead-4;
    if (lastToCheck < responseBuffer) continue;
    for (; p <= lastToCheck; ++p) {
      if (haveSeenNonCRLF) {
        if ((*p == '\r' && *(p+1) == '\n' && *(p+2) == '\r' && *(p+3) == '\n')
           || (*(p+2) == '\r' && *(p+3) == '\r')
           || (*(p+2) == '\n' && *(p+3) == '\n'))
        {
          responseBuffer[bytesRead] = '\0';

          // Before returning, trim any \r or \n from the start:
          while (*responseBuffer == '\r' || *responseBuffer == '\n') {
            ++responseBuffer;
            --bytesRead;
          }
          return bytesRead;
        }
      } else {
        if (*p != '\r' && *p != '\n') {
          haveSeenNonCRLF = True;
        }
      }
    }
  }

  envir().setResultMsg("We received a response not ending with <CR><LF><CR><LF>");
  return 0;
}

Boolean RTSPClient::parseResponseCode(char const* line,
				      unsigned& responseCode) {
  if (sscanf(line, "%*s%u", &responseCode) != 1) {
    envir().setResultMsg("no response code in line: \"", line, "\"");
    return False;
  }

  return True;
}

Boolean RTSPClient::parseTransportResponse(char const* line,
					   char*& serverAddressStr,
					   portNumBits& serverPortNum,
					   unsigned char& rtpChannelId,
					   unsigned char& rtcpChannelId) {
  // Initialize the return parameters to 'not found' values:
  serverAddressStr = NULL;
  serverPortNum = 0;
  rtpChannelId = rtcpChannelId = 0xFF;

  char* foundServerAddressStr = NULL;
  Boolean foundServerPortNum = False;
  Boolean foundChannelIds = False;
  unsigned rtpCid, rtcpCid;
  Boolean isMulticast = True; // by default
  char* foundDestinationStr = NULL;
  portNumBits multicastPortNumRTP, multicastPortNumRTCP;
  Boolean foundMulticastPortNum = False;

  // First, check for "Transport:"
  if (_strncasecmp(line, "Transport: ", 11) != 0) return False;
  line += 11;

  // Then, run through each of the fields, looking for ones we handle:
  char const* fields = line;
  char* field = strDupSize(fields);
  while (sscanf(fields, "%[^;]", field) == 1) {
    if (sscanf(field, "server_port=%hu", &serverPortNum) == 1) {
      foundServerPortNum = True;
    } else if (_strncasecmp(field, "source=", 7) == 0) {
      delete[] foundServerAddressStr;
      foundServerAddressStr = strDup(field+7);
    } else if (sscanf(field, "interleaved=%u-%u", &rtpCid, &rtcpCid) == 2) {
      rtpChannelId = (unsigned char)rtpCid;
      rtcpChannelId = (unsigned char)rtcpCid;
      foundChannelIds = True;
    } else if (strcmp(field, "unicast") == 0) {
      isMulticast = False;
    } else if (_strncasecmp(field, "destination=", 12) == 0) {
      delete[] foundDestinationStr;
      foundDestinationStr = strDup(field+12);
    } else if (sscanf(field, "port=%hu-%hu",
		      &multicastPortNumRTP, &multicastPortNumRTCP) == 2) {
      foundMulticastPortNum = True;
    }

    fields += strlen(field);
    while (fields[0] == ';') ++fields; // skip over all leading ';' chars
    if (fields[0] == '\0') break;
  }
  delete[] field;

  // If we're multicast, and have a "destination=" (multicast) address, then use this
  // as the 'server' address (because some weird servers don't specify the multicast
  // address earlier, in the "DESCRIBE" response's SDP:
  if (isMulticast && foundDestinationStr != NULL && foundMulticastPortNum) {
    delete[] foundServerAddressStr;
    serverAddressStr = foundDestinationStr;
    serverPortNum = multicastPortNumRTP;
    return True;
  }
  delete[] foundDestinationStr;

  if (foundServerPortNum || foundChannelIds) {
    serverAddressStr = foundServerAddressStr;
    return True;
  }

  delete[] foundServerAddressStr;
  return False;
}

Boolean RTSPClient::parseRTPInfoHeader(char*& line, u_int16_t& seqNum, u_int32_t& timestamp) {
  // At this point in the parsing, "line" should begin with either "RTP-Info: " (for the start of the header),
  // or ",", indicating the RTP-Info parameter list for the 2nd-through-nth subsessions:
  if (_strncasecmp(line, "RTP-Info: ", 10) == 0) {
    line += 10;
  } else if (line[0] == ',') {
    ++line;
  } else {
    return False;
  }

  // "line" now consists of a ';'-separated list of parameters, ending with ',' or '\0'.
  char* field = strDupSize(line);

  while (sscanf(line, "%[^;,]", field) == 1) {
    if (sscanf(field, "seq=%hu", &seqNum) == 1 ||
	sscanf(field, "rtptime=%u", &timestamp) == 1) {
    }

    line += strlen(field);
    if (line[0] == '\0' || line[0] == ',') break;
    // ASSERT: line[0] == ';'
    ++line; // skip over the ';'
  }

  delete[] field;
  return True;
}

Boolean RTSPClient::parseScaleHeader(char const* line, float& scale) {
  if (_strncasecmp(line, "Scale: ", 7) != 0) return False;
  line += 7;

  Locale l("C", LC_NUMERIC);
  return sscanf(line, "%f", &scale) == 1;
}

Boolean RTSPClient::parseGetParameterHeader(char const* line,
                                            const char* param,
                                            char*& value) {
  if ((param != NULL && param[0] != '\0') &&
      (line != NULL && line[0] != '\0')) {
    int param_len = strlen(param);
    int line_len = strlen(line);

    if (_strncasecmp(line, param, param_len) != 0) {
      if (fVerbosityLevel >= 1) {
        envir() << "Parsing for \""<< param << "\" and didn't find it, return False\n";
      }
      return False;
    }

    // Strip \r\n from the end if it's there.
    if (line[line_len-2] == '\r' && line[line_len-1] == '\n') {
      line_len -= 2;
    }

    // Look for ": " appended to our requested parameter
    if (line[param_len] == ':' && line[param_len+1] == ' ') {
      // But make sure ": " wasn't in our actual serach param before adjusting
      if (param[param_len-2] != ':' && param[param_len-1] != ' ') {
        if (fVerbosityLevel >= 1) {
          envir() << "Found \": \" appended to parameter\n";
        }
        param_len += 2;
      }
    }

    // Get the string we want out of the line:
    value = strDup(line+param_len);
    return True;
  }
  return False;
}

Boolean RTSPClient::setupHTTPTunneling(char const* urlSuffix,
				       Authenticator* authenticator) {
  // Set up RTSP-over-HTTP tunneling, as described in
  //     http://developer.apple.com/documentation/QuickTime/QTSS/Concepts/chapter_2_section_14.html
  if (fVerbosityLevel >= 1) {
    envir() << "Requesting RTSP-over-HTTP tunneling (on port "
	    << fTunnelOverHTTPPortNum << ")\n\n";
  }
  if (urlSuffix == NULL || urlSuffix[0] == '\0') urlSuffix = "/";
  char* cmd = NULL;

  do {
    // Create a 'session cookie' string, using MD5:
    struct {
      struct timeval timestamp;
      unsigned counter;
    } seedData;
    gettimeofday(&seedData.timestamp, NULL);
    static unsigned counter = 0;
    seedData.counter = ++counter;
    char sessionCookie[33];
    our_MD5Data((unsigned char*)(&seedData), sizeof seedData, sessionCookie);
    // DSS seems to require that the 'session cookie' string be 22 bytes long:
    sessionCookie[23] = '\0';

    // Construct an authenticator string:
    char* authenticatorStr
      = createAuthenticatorString(authenticator, "GET", urlSuffix);

    // Begin by sending a HTTP "GET", to set up the server->client link:
    char const* const getCmdFmt =
      "GET %s HTTP/1.0\r\n"
      "%s"
      "%s"
      "x-sessioncookie: %s\r\n"
      "Accept: application/x-rtsp-tunnelled\r\n"
      "Pragma: no-cache\r\n"
      "Cache-Control: no-cache\r\n"
      "\r\n";
    unsigned cmdSize = strlen(getCmdFmt)
      + strlen(urlSuffix)
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrSize
      + strlen(sessionCookie);
    cmd = new char[cmdSize];
    sprintf(cmd, getCmdFmt,
	    urlSuffix,
	    authenticatorStr,
	    fUserAgentHeaderStr,
	    sessionCookie);
    delete[] authenticatorStr;
    if (!sendRequest(cmd, "HTTP GET", False/*don't base64-encode*/)) break;

    // Get the response from the server:
    unsigned bytesRead; unsigned responseCode;
    char* firstLine; char* nextLineStart;
    if (!getResponse("HTTP GET", bytesRead, responseCode, firstLine, nextLineStart,
		     False /*don't check for response code 200*/)) break;
    if (responseCode != 200) {
      checkForAuthenticationFailure(responseCode, nextLineStart, authenticator);
      envir().setResultMsg("cannot handle HTTP GET response: ", firstLine);
      break;
    }

    // Next, set up a second TCP connection (to the same server & port as before)
    // for the HTTP-tunneled client->server link.  All future output will be to
    // this socket.
    fOutputSocketNum = setupStreamSocket(envir(), 0, False /* =>blocking */);
    if (fOutputSocketNum < 0) break;

    // Connect to the remote endpoint:
    MAKE_SOCKADDR_IN(remoteName, fServerAddress, htons(fTunnelOverHTTPPortNum));
    if (connect(fOutputSocketNum,
		(struct sockaddr*)&remoteName, sizeof remoteName) != 0) {
      envir().setResultErrMsg("connect() failed: ");
      break;
    }

    // Then, send a HTTP "POST", to set up the client->server link:
    authenticatorStr = createAuthenticatorString(authenticator, "POST", urlSuffix);
    char const* const postCmdFmt =
      "POST %s HTTP/1.0\r\n"
      "%s"
      "%s"
      "x-sessioncookie: %s\r\n"
      "Content-Type: application/x-rtsp-tunnelled\r\n"
      "Pragma: no-cache\r\n"
      "Cache-Control: no-cache\r\n"
      "Content-Length: 32767\r\n"
      "Expires: Sun, 9 Jan 1972 00:00:00 GMT\r\n"
      "\r\n";
    cmdSize = strlen(postCmdFmt)
      + strlen(urlSuffix)
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrSize
      + strlen(sessionCookie);
    delete[] cmd; cmd = new char[cmdSize];
    sprintf(cmd, postCmdFmt,
	    urlSuffix,
	    authenticatorStr,
	    fUserAgentHeaderStr,
	    sessionCookie);
    delete[] authenticatorStr;
    if (!sendRequest(cmd, "HTTP POST", False/*don't base64-encode*/)) break;

    // Note that there's no response to the "POST".

    delete[] cmd;
    return True;
  } while (0);

  // An error occurred:
  delete[] cmd;
  return False;
}

void RTSPClient::incomingRequestHandler(void* instance, int /*mask*/) {
  RTSPClient* session = (RTSPClient*)instance;
  session->incomingRequestHandler1();
}

void RTSPClient::incomingRequestHandler1() {
  unsigned bytesRead;
  char* readBuf = fResponseBuffer;
  bytesRead = getResponse1(readBuf, fResponseBufferSize);
  if (bytesRead == 0) {
    envir().setResultMsg("Failed to read response: Connection was closed by the remote host.");
    envir().taskScheduler().turnOffBackgroundReadHandling(fInputSocketNum); // because the connection died
    return;
  }
  // Parse the request string into command name and 'CSeq',
  // then handle the command:
  char cmdName[RTSP_PARAM_STRING_MAX];
  char urlPreSuffix[RTSP_PARAM_STRING_MAX];
  char urlSuffix[RTSP_PARAM_STRING_MAX];
  char cseq[RTSP_PARAM_STRING_MAX];
  if (!parseRTSPRequestString((char*)readBuf, bytesRead,
			      cmdName, sizeof cmdName,
			      urlPreSuffix, sizeof urlPreSuffix,
			      urlSuffix, sizeof urlSuffix,
			      cseq, sizeof cseq)) {
    return;
  } else {
    if (fVerbosityLevel >= 1) {
      envir() << "Received request: " << readBuf << "\n";
    }
    handleCmd_notSupported(cseq);
  }
}

void RTSPClient::handleCmd_notSupported(char const* cseq) {
  char tmpBuf[512];
  snprintf((char*)tmpBuf, sizeof tmpBuf,
	   "RTSP/1.0 405 Method Not Allowed\r\nCSeq: %s\r\n\r\n", cseq);
  send(fOutputSocketNum, tmpBuf, strlen(tmpBuf), 0);
}
