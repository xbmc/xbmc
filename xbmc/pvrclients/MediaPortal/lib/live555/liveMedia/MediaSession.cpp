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
// A data structure that represents a session that consists of
// potentially multiple (audio and/or video) sub-sessions
// Implementation

#include "liveMedia.hh"
#include "Locale.hh"
#ifdef SUPPORT_REAL_RTSP
#include "../RealRTSP/include/RealRTSP.hh"
#endif
#include "GroupsockHelper.hh"
#include <ctype.h>

////////// MediaSession //////////

MediaSession* MediaSession::createNew(UsageEnvironment& env,
                                      char const* sdpDescription) {
  MediaSession* newSession = new MediaSession(env);
  if (newSession != NULL) {
    if (!newSession->initializeWithSDP(sdpDescription)) {
      delete newSession;
      return NULL;
    }
  }

  return newSession;
}

Boolean MediaSession::lookupByName(UsageEnvironment& env,
                                   char const* instanceName,
                                   MediaSession*& resultSession) {
  resultSession = NULL; // unless we succeed

  Medium* medium;
  if (!Medium::lookupByName(env, instanceName, medium)) return False;

  if (!medium->isMediaSession()) {
    env.setResultMsg(instanceName, " is not a 'MediaSession' object");
    return False;
  }

  resultSession = (MediaSession*)medium;
  return True;
}

MediaSession::MediaSession(UsageEnvironment& env)
  : Medium(env),
    fSubsessionsHead(NULL), fSubsessionsTail(NULL),
    fConnectionEndpointName(NULL), fMaxPlayStartTime(0.0f), fMaxPlayEndTime(0.0f),
    fScale(1.0f), fMediaSessionType(NULL), fSessionName(NULL), fSessionDescription(NULL),
    fControlPath(NULL) {
#ifdef SUPPORT_REAL_RTSP
  RealInitSDPAttributes(this);
#endif
  fSourceFilterAddr.s_addr = 0;

  // Get our host name, and use this for the RTCP CNAME:
  const unsigned maxCNAMElen = 100;
  char CNAME[maxCNAMElen+1];
#ifndef CRIS
  gethostname((char*)CNAME, maxCNAMElen);
#else
  // "gethostname()" isn't defined for this platform
  sprintf(CNAME, "unknown host %d", (unsigned)(our_random()*0x7FFFFFFF));
#endif
  CNAME[maxCNAMElen] = '\0'; // just in case
  fCNAME = strDup(CNAME);
}

MediaSession::~MediaSession() {
  delete fSubsessionsHead;
  delete[] fCNAME;
  delete[] fConnectionEndpointName;
  delete[] fMediaSessionType;
  delete[] fSessionName;
  delete[] fSessionDescription;
  delete[] fControlPath;
#ifdef SUPPORT_REAL_RTSP
  RealReclaimSDPAttributes(this);
#endif
}

Boolean MediaSession::isMediaSession() const {
  return True;
}

Boolean MediaSession::initializeWithSDP(char const* sdpDescription) {
  if (sdpDescription == NULL) return False;

  // Begin by processing all SDP lines until we see the first "m="
  char const* sdpLine = sdpDescription;
  char const* nextSDPLine;
  while (1) {
    if (!parseSDPLine(sdpLine, nextSDPLine)) return False;
    //##### We should really check for:
    // - "a=control:" attributes (to set the URL for aggregate control)
    // - the correct SDP version (v=0)
    if (sdpLine[0] == 'm') break;
    sdpLine = nextSDPLine;
    if (sdpLine == NULL) break; // there are no m= lines at all

    // Check for various special SDP lines that we understand:
    if (parseSDPLine_s(sdpLine)) continue;
    if (parseSDPLine_i(sdpLine)) continue;
    if (parseSDPLine_c(sdpLine)) continue;
    if (parseSDPAttribute_control(sdpLine)) continue;
    if (parseSDPAttribute_range(sdpLine)) continue;
    if (parseSDPAttribute_type(sdpLine)) continue;
    if (parseSDPAttribute_source_filter(sdpLine)) continue;
#ifdef SUPPORT_REAL_RTSP
    if (RealParseSDPAttributes(this, sdpLine)) continue;
#endif
  }

  while (sdpLine != NULL) {
    // We have a "m=" line, representing a new sub-session:
    MediaSubsession* subsession = new MediaSubsession(*this);
    if (subsession == NULL) {
      envir().setResultMsg("Unable to create new MediaSubsession");
      return False;
    }

    // Parse the line as "m=<medium_name> <client_portNum> RTP/AVP <fmt>"
    // or "m=<medium_name> <client_portNum>/<num_ports> RTP/AVP <fmt>"
    // (Should we be checking for >1 payload format number here?)#####
    char* mediumName = strDupSize(sdpLine); // ensures we have enough space
    char const* protocolName = NULL;
    unsigned payloadFormat;
    if ((sscanf(sdpLine, "m=%s %hu RTP/AVP %u", mediumName, &subsession->fClientPortNum, &payloadFormat) == 3 ||
     sscanf(sdpLine, "m=%s %hu/%*u RTP/AVP %u", mediumName, &subsession->fClientPortNum, &payloadFormat) == 3) && payloadFormat <= 127)
    {
      protocolName = "RTP";
    } else if ((sscanf(sdpLine, "m=%s %hu UDP %u",
               mediumName, &subsession->fClientPortNum, &payloadFormat) == 3 ||
        sscanf(sdpLine, "m=%s %hu udp %u",
               mediumName, &subsession->fClientPortNum, &payloadFormat) == 3 ||
        sscanf(sdpLine, "m=%s %hu RAW/RAW/UDP %u",
               mediumName, &subsession->fClientPortNum, &payloadFormat) == 3)
           && payloadFormat <= 127) {
      // This is a RAW UDP source
      protocolName = "UDP";
    } else {
      // This "m=" line is bad; output an error message saying so:
      char* sdpLineStr;
      if (nextSDPLine == NULL) {
        sdpLineStr = (char*)sdpLine;
      } else {
        sdpLineStr = strDup(sdpLine);
        sdpLineStr[nextSDPLine-sdpLine] = '\0';
      }
      envir() << "Bad SDP \"m=\" line: " <<  sdpLineStr << "\n";
      if (sdpLineStr != (char*)sdpLine) delete[] sdpLineStr;

      delete[] mediumName;
      delete subsession;

      // Skip the following SDP lines, up until the next "m=":
      while (1) {
        sdpLine = nextSDPLine;
        if (sdpLine == NULL) break; // we've reached the end
        if (!parseSDPLine(sdpLine, nextSDPLine)) return False;

        if (sdpLine[0] == 'm') break; // we've reached the next subsession
      }
      continue;
    }

    // Insert this subsession at the end of the list:
    if (fSubsessionsTail == NULL) {
      fSubsessionsHead = fSubsessionsTail = subsession;
    } else {
      fSubsessionsTail->setNext(subsession);
      fSubsessionsTail = subsession;
    }

    subsession->serverPortNum = subsession->fClientPortNum; // by default

    char const* mStart = sdpLine;
    subsession->fSavedSDPLines = strDup(mStart);

    subsession->fMediumName = strDup(mediumName);
    delete[] mediumName;
    subsession->fProtocolName = strDup(protocolName);
    subsession->fRTPPayloadFormat = payloadFormat;

    // Process the following SDP lines, up until the next "m=":
    while (1) {
      sdpLine = nextSDPLine;
      if (sdpLine == NULL) break; // we've reached the end
      if (!parseSDPLine(sdpLine, nextSDPLine)) return False;

      if (sdpLine[0] == 'm') break; // we've reached the next subsession

      // Check for various special SDP lines that we understand:
      if (subsession->parseSDPLine_c(sdpLine)) continue;
      if (subsession->parseSDPLine_b(sdpLine)) continue;
      if (subsession->parseSDPAttribute_rtpmap(sdpLine)) continue;
      if (subsession->parseSDPAttribute_control(sdpLine)) continue;
      if (subsession->parseSDPAttribute_range(sdpLine)) continue;
      if (subsession->parseSDPAttribute_fmtp(sdpLine)) continue;
      if (subsession->parseSDPAttribute_source_filter(sdpLine)) continue;
      if (subsession->parseSDPAttribute_x_dimensions(sdpLine)) continue;
      if (subsession->parseSDPAttribute_framerate(sdpLine)) continue;
#ifdef SUPPORT_REAL_RTSP
      if (RealParseSDPAttributes(subsession, sdpLine)) continue;
#endif

      // (Later, check for malformed lines, and other valid SDP lines#####)
    }
    if (sdpLine != NULL) subsession->fSavedSDPLines[sdpLine-mStart] = '\0';

    // If we don't yet know the codec name, try looking it up from the
    // list of static payload types:
    if (subsession->fCodecName == NULL) {
      subsession->fCodecName = lookupPayloadFormat(subsession->fRTPPayloadFormat,
                  subsession->fRTPTimestampFrequency,
                  subsession->fNumChannels);
      if (subsession->fCodecName == NULL) {
        char typeStr[20];
        sprintf(typeStr, "%d", subsession->fRTPPayloadFormat);
        envir().setResultMsg("Unknown codec name for RTP payload type ", typeStr);
        return False;
      }
    }

    // If we don't yet know this subsession's RTP timestamp frequency
    // (because it uses a dynamic payload type and the corresponding
    // SDP "rtpmap" attribute erroneously didn't specify it),
    // then guess it now:
    if (subsession->fRTPTimestampFrequency == 0) {
      subsession->fRTPTimestampFrequency = guessRTPTimestampFrequency(subsession->fMediumName, subsession->fCodecName);
    }
  }

  return True;
}

Boolean MediaSession::parseSDPLine(char const* inputLine, char const*& nextLine)
{
  // Begin by finding the start of the next line (if any):
  nextLine = NULL;
  for (char const* ptr = inputLine; *ptr != '\0'; ++ptr) {
    if (*ptr == '\r' || *ptr == '\n') {
      // We found the end of the line
      ++ptr;
      while (*ptr == '\r' || *ptr == '\n') ++ptr;
      nextLine = ptr;
      if (nextLine[0] == '\0') nextLine = NULL; // special case for end
      break;
    }
  }

  // Then, check that this line is a SDP line of the form <char>=<etc>
  // (However, we also accept blank lines in the input.)
  if (inputLine[0] == '\r' || inputLine[0] == '\n') return True;
  if (strlen(inputLine) < 2 || inputLine[1] != '='
      || inputLine[0] < 'a' || inputLine[0] > 'z') {
    envir().setResultMsg("Invalid SDP line: ", inputLine);
    return False;
  }

  return True;
}

static char* parseCLine(char const* sdpLine) {
  char* resultStr = NULL;
  char* buffer = strDupSize(sdpLine); // ensures we have enough space
  if (sscanf(sdpLine, "c=IN IP4 %[^/\r\n]", buffer) == 1) {
    // Later, handle the optional /<ttl> and /<numAddresses> #####
    resultStr = strDup(buffer);
  }
  delete[] buffer;

  return resultStr;
}

Boolean MediaSession::parseSDPLine_s(char const* sdpLine) {
  // Check for "s=<session name>" line
  char* buffer = strDupSize(sdpLine);
  Boolean parseSuccess = False;

  if (sscanf(sdpLine, "s=%[^\r\n]", buffer) == 1) {
    delete[] fSessionName; fSessionName = strDup(buffer);
    parseSuccess = True;
  }
  delete[] buffer;

  return parseSuccess;
}

Boolean MediaSession::parseSDPLine_i(char const* sdpLine) {
  // Check for "i=<session description>" line
  char* buffer = strDupSize(sdpLine);
  Boolean parseSuccess = False;

  if (sscanf(sdpLine, "i=%[^\r\n]", buffer) == 1) {
    delete[] fSessionDescription; fSessionDescription = strDup(buffer);
    parseSuccess = True;
  }
  delete[] buffer;

  return parseSuccess;
}

Boolean MediaSession::parseSDPLine_c(char const* sdpLine) {
  // Check for "c=IN IP4 <connection-endpoint>"
  // or "c=IN IP4 <connection-endpoint>/<ttl+numAddresses>"
  // (Later, do something with <ttl+numAddresses> also #####)
  char* connectionEndpointName = parseCLine(sdpLine);
  if (connectionEndpointName != NULL) {
    delete[] fConnectionEndpointName;
    fConnectionEndpointName = connectionEndpointName;
    return True;
  }

  return False;
}

Boolean MediaSession::parseSDPAttribute_type(char const* sdpLine) {
  // Check for a "a=type:broadcast|meeting|moderated|test|H.332|recvonly" line:
  Boolean parseSuccess = False;

  char* buffer = strDupSize(sdpLine);
  if (sscanf(sdpLine, "a=type: %[^ ]", buffer) == 1) {
    delete[] fMediaSessionType;
    fMediaSessionType = strDup(buffer);
    parseSuccess = True;
  }
  delete[] buffer;

  return parseSuccess;
}

static Boolean parseRangeAttribute(char const* sdpLine, double& startTime, double& endTime) {
  return sscanf(sdpLine, "a=range: npt = %lg - %lg", &startTime, &endTime) == 2;
}

Boolean MediaSession::parseSDPAttribute_control(char const* sdpLine) {
  // Check for a "a=control:<control-path>" line:
  Boolean parseSuccess = False;

  char* controlPath = strDupSize(sdpLine); // ensures we have enough space
  if (sscanf(sdpLine, "a=control: %s", controlPath) == 1) {
    parseSuccess = True;
    delete[] fControlPath; fControlPath = strDup(controlPath);
  }
  delete[] controlPath;

  return parseSuccess;
}

Boolean MediaSession::parseSDPAttribute_range(char const* sdpLine) {
  // Check for a "a=range:npt=<startTime>-<endTime>" line:
  // (Later handle other kinds of "a=range" attributes also???#####)
  Boolean parseSuccess = False;

  double playStartTime;
  double playEndTime;
  if (parseRangeAttribute(sdpLine, playStartTime, playEndTime)) {
    parseSuccess = True;
    if (playStartTime > fMaxPlayStartTime) {
      fMaxPlayStartTime = playStartTime;
    }
    if (playEndTime > fMaxPlayEndTime) {
      fMaxPlayEndTime = playEndTime;
    }
  }

  return parseSuccess;
}

static Boolean parseSourceFilterAttribute(char const* sdpLine,
                                          struct in_addr& sourceAddr) {
  // Check for a "a=source-filter:incl IN IP4 <something> <source>" line.
  // Note: At present, we don't check that <something> really matches
  // one of our multicast addresses.  We also don't support more than
  // one <source> #####
  Boolean result = False; // until we succeed
  char* sourceName = strDupSize(sdpLine); // ensures we have enough space
  do {
    if (sscanf(sdpLine, "a=source-filter: incl IN IP4 %*s %s", sourceName) != 1) break;

    // Now, convert this name to an address, if we can:
    NetAddressList addresses(sourceName);
    if (addresses.numAddresses() == 0) break;

    netAddressBits sourceAddrBits
      = *(netAddressBits*)(addresses.firstAddress()->data());
    if (sourceAddrBits == 0) break;

    sourceAddr.s_addr = sourceAddrBits;
    result = True;
  } while (0);

  delete[] sourceName;
  return result;
}

Boolean MediaSession
::parseSDPAttribute_source_filter(char const* sdpLine) {
  return parseSourceFilterAttribute(sdpLine, fSourceFilterAddr);
}

char* MediaSession::lookupPayloadFormat(unsigned char rtpPayloadType,
                                        unsigned& freq, unsigned& nCh) {
  // Look up the codec name and timestamp frequency for known (static)
  // RTP payload formats.
  char const* temp = NULL;
  switch (rtpPayloadType) {
  case 0: {temp = "PCMU"; freq = 8000; nCh = 1; break;}
  case 2: {temp = "G726-32"; freq = 8000; nCh = 1; break;}
  case 3: {temp = "GSM"; freq = 8000; nCh = 1; break;}
  case 4: {temp = "G723"; freq = 8000; nCh = 1; break;}
  case 5: {temp = "DVI4"; freq = 8000; nCh = 1; break;}
  case 6: {temp = "DVI4"; freq = 16000; nCh = 1; break;}
  case 7: {temp = "LPC"; freq = 8000; nCh = 1; break;}
  case 8: {temp = "PCMA"; freq = 8000; nCh = 1; break;}
  case 9: {temp = "G722"; freq = 8000; nCh = 1; break;}
  case 10: {temp = "L16"; freq = 44100; nCh = 2; break;}
  case 11: {temp = "L16"; freq = 44100; nCh = 1; break;}
  case 12: {temp = "QCELP"; freq = 8000; nCh = 1; break;}
  case 14: {temp = "MPA"; freq = 90000; nCh = 1; break;}
    // 'number of channels' is actually encoded in the media stream
  case 15: {temp = "G728"; freq = 8000; nCh = 1; break;}
  case 16: {temp = "DVI4"; freq = 11025; nCh = 1; break;}
  case 17: {temp = "DVI4"; freq = 22050; nCh = 1; break;}
  case 18: {temp = "G729"; freq = 8000; nCh = 1; break;}
  case 25: {temp = "CELB"; freq = 90000; nCh = 1; break;}
  case 26: {temp = "JPEG"; freq = 90000; nCh = 1; break;}
  case 28: {temp = "NV"; freq = 90000; nCh = 1; break;}
  case 31: {temp = "H261"; freq = 90000; nCh = 1; break;}
  case 32: {temp = "MPV"; freq = 90000; nCh = 1; break;}
  case 33: {temp = "MP2T"; freq = 90000; nCh = 1; break;}
  case 34: {temp = "H263"; freq = 90000; nCh = 1; break;}
  };

  return strDup(temp);
}

unsigned MediaSession::guessRTPTimestampFrequency(char const* mediumName,
                                                  char const* codecName) {
  // By default, we assume that audio sessions use a frequency of 8000,
  // video sessions use a frequency of 90000,
  // and text sessions use a frequency of 1000.
  // Begin by checking for known exceptions to this rule
  // (where the frequency is known unambiguously (e.g., not like "DVI4"))
  if (strcmp(codecName, "L16") == 0) return 44100;
  if (strcmp(codecName, "MPA") == 0
      || strcmp(codecName, "MPA-ROBUST") == 0
      || strcmp(codecName, "X-MP3-DRAFT-00")) return 90000;

  // Now, guess default values:
  if (strcmp(mediumName, "video") == 0) return 90000;
  else if (strcmp(mediumName, "text") == 0) return 1000;
  return 8000; // for "audio", and any other medium
}

Boolean MediaSession
::initiateByMediaType(char const* mimeType,
                      MediaSubsession*& resultSubsession,
                      int useSpecialRTPoffset) {
  // Look through this session's subsessions for media that match "mimeType"
  resultSubsession = NULL;
  MediaSubsessionIterator iter(*this);
  MediaSubsession* subsession;
  while ((subsession = iter.next()) != NULL) {
    Boolean wasAlreadyInitiated = subsession->readSource() != NULL;
    if (!wasAlreadyInitiated) {
      // Try to create a source for this subsession:
      if (!subsession->initiate(useSpecialRTPoffset)) return False;
    }

    // Make sure the source's MIME type is one that we handle:
    if (strcmp(subsession->readSource()->MIMEtype(), mimeType) != 0) {
      if (!wasAlreadyInitiated) subsession->deInitiate();
      continue;
    }

    resultSubsession = subsession;
    break; // use this
  }

  if (resultSubsession == NULL) {
    envir().setResultMsg("Session has no usable media subsession");
    return False;
  }

  return True;
}


////////// MediaSubsessionIterator //////////

MediaSubsessionIterator::MediaSubsessionIterator(MediaSession& session)
  : fOurSession(session) {
  reset();
}

MediaSubsessionIterator::~MediaSubsessionIterator() {
}

MediaSubsession* MediaSubsessionIterator::next() {
  MediaSubsession* result = fNextPtr;

  if (fNextPtr != NULL) fNextPtr = fNextPtr->fNext;

  return result;
}

void MediaSubsessionIterator::reset() {
  fNextPtr = fOurSession.fSubsessionsHead;
}

////////// MediaSubsession //////////

MediaSubsession::MediaSubsession(MediaSession& parent)
  : sessionId(NULL), serverPortNum(0), sink(NULL), miscPtr(NULL),
    fParent(parent), fNext(NULL),
    fConnectionEndpointName(NULL),
    fClientPortNum(0), fRTPPayloadFormat(0xFF),
    fSavedSDPLines(NULL), fMediumName(NULL), fCodecName(NULL), fProtocolName(NULL),
    fRTPTimestampFrequency(0), fControlPath(NULL),
    fSourceFilterAddr(parent.sourceFilterAddr()), fBandwidth(0),
    fAuxiliarydatasizelength(0), fConstantduration(0), fConstantsize(0),
    fCRC(0), fCtsdeltalength(0), fDe_interleavebuffersize(0), fDtsdeltalength(0),
    fIndexdeltalength(0), fIndexlength(0), fInterleaving(0), fMaxdisplacement(0),
    fObjecttype(0), fOctetalign(0), fProfile_level_id(0), fRobustsorting(0),
    fSizelength(0), fStreamstateindication(0), fStreamtype(0),
    fCpresent(False), fRandomaccessindication(False),
    fConfig(NULL), fMode(NULL), fSpropParameterSets(NULL),
    fPlayStartTime(0.0), fPlayEndTime(0.0),
    fVideoWidth(0), fVideoHeight(0), fVideoFPS(0), fNumChannels(1), fScale(1.0f), fNPT_PTS_Offset(0.0f),
    fRTPSocket(NULL), fRTCPSocket(NULL),
    fRTPSource(NULL), fRTCPInstance(NULL), fReadSource(NULL) {
  rtpInfo.seqNum = 0; rtpInfo.timestamp = 0; rtpInfo.infoIsNew = False;
#ifdef SUPPORT_REAL_RTSP
  RealInitSDPAttributes(this);
#endif
}

MediaSubsession::~MediaSubsession() {
  deInitiate();

  delete[] fConnectionEndpointName; delete[] fSavedSDPLines;
  delete[] fMediumName; delete[] fCodecName; delete[] fProtocolName;
  delete[] fControlPath; delete[] fConfig; delete[] fMode; delete[] fSpropParameterSets;

  delete fNext;
#ifdef SUPPORT_REAL_RTSP
  RealReclaimSDPAttributes(this);
#endif
}

double MediaSubsession::playStartTime() const {
  if (fPlayStartTime > 0) return fPlayStartTime;

  return fParent.playStartTime();
}

double MediaSubsession::playEndTime() const {
  if (fPlayEndTime > 0) return fPlayEndTime;

  return fParent.playEndTime();
}

Boolean MediaSubsession::initiate(int useSpecialRTPoffset) {
  if (fReadSource != NULL) return True; // has already been initiated

  do {
    if (fCodecName == NULL) {
      env().setResultMsg("Codec is unspecified");
      break;
    }

    // Create RTP and RTCP 'Groupsocks' on which to receive incoming data.
    // (Groupsocks will work even for unicast addresses)
    struct in_addr tempAddr;
    tempAddr.s_addr = connectionEndpointAddress();
        // This could get changed later, as a result of a RTSP "SETUP"

    if (fClientPortNum != 0) {
      // The sockets' port numbers were specified for us.  Use these:
      fClientPortNum = fClientPortNum&~1; // even
      if (isSSM()) {
        fRTPSocket = new Groupsock(env(), tempAddr, fSourceFilterAddr, fClientPortNum);
      } else {
        fRTPSocket = new Groupsock(env(), tempAddr, fClientPortNum, 255);
      }
      if (fRTPSocket == NULL) {
        env().setResultMsg("Failed to create RTP socket");
        break;
      }
      
      // Set our RTCP port to be the RTP port +1
      portNumBits const rtcpPortNum = fClientPortNum|1;
      if (isSSM()) {
        fRTCPSocket = new Groupsock(env(), tempAddr, fSourceFilterAddr, rtcpPortNum);
      } else {
        fRTCPSocket = new Groupsock(env(), tempAddr, rtcpPortNum, 255);
      }
      if (fRTCPSocket == NULL) {
        char tmpBuf[100];
        sprintf(tmpBuf, "Failed to create RTCP socket (port %d)", rtcpPortNum);
        env().setResultMsg(tmpBuf);
        break;
      }
    } else {
      // Port numbers were not specified in advance, so we use ephemeral port numbers.
      // Create sockets until we get a port-number pair (even: RTP; even+1: RTCP).
      // We need to make sure that we don't keep trying to use the same bad port numbers over and over again.
      // so we store bad sockets in a table, and delete them all when we're done.
      HashTable* socketHashTable = HashTable::create(ONE_WORD_HASH_KEYS);
      if (socketHashTable == NULL) break;
      Boolean success = False;
      NoReuse dummy; // ensures that our new ephemeral port number won't be one that's already in use

      while (1) {
        // Create a new socket:
        if (isSSM()) {
          fRTPSocket = new Groupsock(env(), tempAddr, fSourceFilterAddr, 0);
        } else {
          fRTPSocket = new Groupsock(env(), tempAddr, 0, 255);
        }
        if (fRTPSocket == NULL) {
          env().setResultMsg("MediaSession::initiate(): unable to create RTP and RTCP sockets");
          break;
        }

        // Get the client port number, and check whether it's even (for RTP):
        Port clientPort(0);
        if (!getSourcePort(env(), fRTPSocket->socketNum(), clientPort)) {
          break;
        }
        fClientPortNum = ntohs(clientPort.num()); 
        if ((fClientPortNum&1) != 0) { // it's odd
          // Record this socket in our table, and keep trying:
          unsigned key = (unsigned)fClientPortNum;
          Groupsock* existing = (Groupsock*)socketHashTable->Add((char const*)key, fRTPSocket);
          delete existing; // in case it wasn't NULL
          continue;
        }

        // Make sure we can use the next (i.e., odd) port number, for RTCP:
        portNumBits rtcpPortNum = fClientPortNum|1;
        if (isSSM()) {
          fRTCPSocket = new Groupsock(env(), tempAddr, fSourceFilterAddr, rtcpPortNum);
        } else {
          fRTCPSocket = new Groupsock(env(), tempAddr, rtcpPortNum, 255);
        }
        if (fRTCPSocket != NULL) {
          // Success! Use these two sockets.
          success = True;
          break;
        } else {
          // We couldn't create the RTCP socket (perhaps that port number's already in use elsewhere?).
          // Record the first socket in our table, and keep trying:
          unsigned key = (unsigned)fClientPortNum;
          Groupsock* existing = (Groupsock*)socketHashTable->Add((char const*)key, fRTPSocket);
          delete existing; // in case it wasn't NULL
          continue;
        }
      }

      // Clean up the socket hash table (and contents):
      Groupsock* oldGS;
      while ((oldGS = (Groupsock*)socketHashTable->RemoveNext()) != NULL) {
        delete oldGS;
      }
      delete socketHashTable;

      if (!success) break; // a fatal error occurred trying to create the RTP and RTCP sockets; we can't continue
    }

    // Try to use a big receive buffer for RTP - at least 0.1 second of
    // specified bandwidth and at least 50 KB
    unsigned rtpBufSize = fBandwidth * 25 / 2; // 1 kbps * 0.1 s = 12.5 bytes
    if (rtpBufSize < 50 * 1024)
      rtpBufSize = 50 * 1024;
    increaseReceiveBufferTo(env(), fRTPSocket->socketNum(), rtpBufSize);

    // ASSERT: fRTPSocket != NULL && fRTCPSocket != NULL
    if (isSSM()) {
      // Special case for RTCP SSM: Send RTCP packets back to the source via unicast:
      fRTCPSocket->changeDestinationParameters(fSourceFilterAddr,0,~0);
    }

    // Check "fProtocolName"
    if (strcmp(fProtocolName, "UDP") == 0) {
      // A UDP-packetized stream (*not* a RTP stream)
      fReadSource = BasicUDPSource::createNew(env(), fRTPSocket);
      fRTPSource = NULL; // Note!

      if (strcmp(fCodecName, "MP2T") == 0) { // MPEG-2 Transport Stream
        fReadSource = MPEG2TransportStreamFramer::createNew(env(), fReadSource);
        // this sets "durationInMicroseconds" correctly, based on the PCR values
      }
    } else {
      // Check "fCodecName" against the set of codecs that we support,
      // and create our RTP source accordingly
      // (Later make this code more efficient, as this set grows #####)
      // (Also, add more fmts that can be implemented by SimpleRTPSource#####)
      Boolean createSimpleRTPSource = False; // by default; can be changed below
      Boolean doNormalMBitRule = False; // default behavior if "createSimpleRTPSource" is True
      if (strcmp(fCodecName, "QCELP") == 0) { // QCELP audio
        fReadSource = QCELPAudioRTPSource::createNew(env(), fRTPSocket, fRTPSource,
                                         fRTPPayloadFormat,
                                         fRTPTimestampFrequency);
        // Note that fReadSource will differ from fRTPSource in this case
      } else if (strcmp(fCodecName, "AMR") == 0) { // AMR audio (narrowband)
        fReadSource = AMRAudioRTPSource::createNew(env(), fRTPSocket, fRTPSource,
                                         fRTPPayloadFormat, 0 /*isWideband*/,
                                         fNumChannels, fOctetalign, fInterleaving,
                                         fRobustsorting, fCRC);
        // Note that fReadSource will differ from fRTPSource in this case
      } else if (strcmp(fCodecName, "AMR-WB") == 0) { // AMR audio (wideband)
        fReadSource = AMRAudioRTPSource::createNew(env(), fRTPSocket, fRTPSource,
                                         fRTPPayloadFormat, 1 /*isWideband*/,
                                          fNumChannels, fOctetalign, fInterleaving,
                                         fRobustsorting, fCRC);
        // Note that fReadSource will differ from fRTPSource in this case
      } else if (strcmp(fCodecName, "MPA") == 0) { // MPEG-1 or 2 audio
        fReadSource = fRTPSource = MPEG1or2AudioRTPSource::createNew(env(), fRTPSocket,
                                         fRTPPayloadFormat,
                                         fRTPTimestampFrequency);
      } else if (strcmp(fCodecName, "MPA-ROBUST") == 0) { // robust MP3 audio
        fRTPSource = MP3ADURTPSource::createNew(env(), fRTPSocket, fRTPPayloadFormat,
                                         fRTPTimestampFrequency);
        if (fRTPSource == NULL) break;

        // Add a filter that deinterleaves the ADUs after depacketizing them:
        MP3ADUdeinterleaver* deinterleaver = MP3ADUdeinterleaver::createNew(env(), fRTPSource);
        if (deinterleaver == NULL) break;

        // Add another filter that converts these ADUs to MP3 frames:
        fReadSource = MP3FromADUSource::createNew(env(), deinterleaver);
      } else if (strcmp(fCodecName, "X-MP3-DRAFT-00") == 0) {
        // a non-standard variant of "MPA-ROBUST" used by RealNetworks
        // (one 'ADU'ized MP3 frame per packet; no headers)
        fRTPSource = SimpleRTPSource::createNew(env(), fRTPSocket, fRTPPayloadFormat,
                                         fRTPTimestampFrequency,
                                         "audio/MPA-ROBUST" /*hack*/);
        if (fRTPSource == NULL) break;

        // Add a filter that converts these ADUs to MP3 frames:
        fReadSource = MP3FromADUSource::createNew(env(), fRTPSource, False /*no ADU header*/);
      } else if (strcmp(fCodecName, "MP4A-LATM") == 0) { // MPEG-4 LATM audio
        fReadSource = fRTPSource = MPEG4LATMAudioRTPSource::createNew(env(), fRTPSocket,
                                         fRTPPayloadFormat,
                                         fRTPTimestampFrequency);
      } else if (strcmp(fCodecName, "AC3") == 0) { // AC3 audio
        fReadSource = fRTPSource = AC3AudioRTPSource::createNew(env(), fRTPSocket,
                                         fRTPPayloadFormat,
                                         fRTPTimestampFrequency);
      } else if (strcmp(fCodecName, "MP4V-ES") == 0) { // MPEG-4 Elem Str vid
        fReadSource = fRTPSource
          = MPEG4ESVideoRTPSource::createNew(env(), fRTPSocket,
                                         fRTPPayloadFormat,
                                         fRTPTimestampFrequency);
      } else if (strcmp(fCodecName, "MPEG4-GENERIC") == 0) {
        fReadSource = fRTPSource = MPEG4GenericRTPSource::createNew(env(), fRTPSocket,
                                         fRTPPayloadFormat,
                                         fRTPTimestampFrequency,
                                         fMediumName, fMode,
                                         fSizelength, fIndexlength,
                                         fIndexdeltalength);
      } else if (strcmp(fCodecName, "MPV") == 0) { // MPEG-1 or 2 video
        fReadSource = fRTPSource = MPEG1or2VideoRTPSource::createNew(env(), fRTPSocket,
                                         fRTPPayloadFormat,
                                         fRTPTimestampFrequency);
      } else if (strcmp(fCodecName, "MP2T") == 0) { // MPEG-2 Transport Stream
        fRTPSource = SimpleRTPSource::createNew(env(), fRTPSocket, fRTPPayloadFormat,
                                        fRTPTimestampFrequency, "video/MP2T",
                                        0, False);
        fReadSource = MPEG2TransportStreamFramer::createNew(env(), fRTPSource);
        // this sets "durationInMicroseconds" correctly, based on the PCR values
      } else if (strcmp(fCodecName, "H261") == 0) { // H.261
        fReadSource = fRTPSource = H261VideoRTPSource::createNew(env(), fRTPSocket,
                                         fRTPPayloadFormat,
                                         fRTPTimestampFrequency);
      } else if (strcmp(fCodecName, "H263-1998") == 0 || strcmp(fCodecName, "H263-2000") == 0) { // H.263+
        fReadSource = fRTPSource = H263plusVideoRTPSource::createNew(env(), fRTPSocket,
                                         fRTPPayloadFormat,
                                         fRTPTimestampFrequency);
      } else if (strcmp(fCodecName, "H264") == 0) {
        fReadSource = fRTPSource = H264VideoRTPSource::createNew(env(), fRTPSocket,
                                         fRTPPayloadFormat,
                                         fRTPTimestampFrequency);
      } else if (strcmp(fCodecName, "DV") == 0) {
        fReadSource = fRTPSource = DVVideoRTPSource::createNew(env(), fRTPSocket,
                                         fRTPPayloadFormat,
                                         fRTPTimestampFrequency);
      } else if (strcmp(fCodecName, "JPEG") == 0) { // motion JPEG
        fReadSource = fRTPSource = JPEGVideoRTPSource::createNew(env(), fRTPSocket,
                                         fRTPPayloadFormat,
                                         fRTPTimestampFrequency,
                                         videoWidth(),
                                         videoHeight());
      } else if (strcmp(fCodecName, "X-QT") == 0 || strcmp(fCodecName, "X-QUICKTIME") == 0) {
        // Generic QuickTime streams, as defined in
        // <http://developer.apple.com/quicktime/icefloe/dispatch026.html>
        char* mimeType = new char[strlen(mediumName()) + strlen(codecName()) + 2] ;
        sprintf(mimeType, "%s/%s", mediumName(), codecName());
        fReadSource = fRTPSource = QuickTimeGenericRTPSource::createNew(env(), fRTPSocket,
                                                                        fRTPPayloadFormat,
                                                                        fRTPTimestampFrequency,
                                                                        mimeType);
        delete[] mimeType;
#ifdef SUPPORT_REAL_RTSP
      } else if (strcmp(fCodecName, "X-PN-REALAUDIO") == 0 ||
                 strcmp(fCodecName, "X-PN-MULTIRATE-REALAUDIO-LIVE") == 0 ||
                 strcmp(fCodecName, "X-PN-REALVIDEO") == 0 ||
                 strcmp(fCodecName, "X-PN-MULTIRATE-REALVIDEO-LIVE") == 0) {
        // A RealNetworks 'RDT' stream (*not* a RTP stream)
        fReadSource = RealRDTSource::createNew(env());
        fRTPSource = NULL; // Note!
        parentSession().isRealNetworksRDT = True;
#endif
      } else if (  strcmp(fCodecName, "PCMU") == 0 // PCM u-law audio
                   || strcmp(fCodecName, "GSM") == 0 // GSM audio
                   || strcmp(fCodecName, "PCMA") == 0 // PCM a-law audio
                   || strcmp(fCodecName, "L16") == 0 // 16-bit linear audio
                   || strcmp(fCodecName, "MP1S") == 0 // MPEG-1 System Stream
                   || strcmp(fCodecName, "MP2P") == 0 // MPEG-2 Program Stream
                   || strcmp(fCodecName, "L8") == 0 // 8-bit linear audio
                   || strcmp(fCodecName, "G726-16") == 0 // G.726, 16 kbps
                   || strcmp(fCodecName, "G726-24") == 0 // G.726, 24 kbps
                   || strcmp(fCodecName, "G726-32") == 0 // G.726, 32 kbps
                   || strcmp(fCodecName, "G726-40") == 0 // G.726, 40 kbps
                   || strcmp(fCodecName, "SPEEX") == 0 // SPEEX audio
                   || strcmp(fCodecName, "T140") == 0 // T.140 text (RFC 4103)
                   ) {
        createSimpleRTPSource = True;
        useSpecialRTPoffset = 0;
      } else if (useSpecialRTPoffset >= 0) {
        // We don't know this RTP payload format, but try to receive
        // it using a 'SimpleRTPSource' with the specified header offset:
        createSimpleRTPSource = True;
      } else {
        env().setResultMsg("RTP payload format unknown or not supported");
        break;
      }

      if (createSimpleRTPSource) {
        char* mimeType = new char[strlen(mediumName()) + strlen(codecName()) + 2] ;
        sprintf(mimeType, "%s/%s", mediumName(), codecName());
        fReadSource = fRTPSource = SimpleRTPSource::createNew(env(), fRTPSocket, fRTPPayloadFormat,
                                         fRTPTimestampFrequency, mimeType,
                                         (unsigned)useSpecialRTPoffset,
                                         doNormalMBitRule);
        delete[] mimeType;
      }
    }

    if (fReadSource == NULL) {
      env().setResultMsg("Failed to create read source");
      break;
    }

    // Finally, create our RTCP instance. (It starts running automatically)
    if (fRTPSource != NULL) {
      // If bandwidth is specified, use it and add 5% for RTCP overhead.
      // Otherwise make a guess at 500 kbps.
      unsigned totSessionBandwidth = fBandwidth ? fBandwidth + fBandwidth / 20 : 500;
      fRTCPInstance = RTCPInstance::createNew(env(), fRTCPSocket,
                                              totSessionBandwidth,
                                              (unsigned char const*)
                                              fParent.CNAME(),
                                              NULL /* we're a client */,
                                              fRTPSource);
      if (fRTCPInstance == NULL) {
        env().setResultMsg("Failed to create RTCP instance");
        break;
      }
    }

    return True;
  } while (0);

  delete fRTPSocket; fRTPSocket = NULL;
  delete fRTCPSocket; fRTCPSocket = NULL;
  Medium::close(fRTCPInstance); fRTCPInstance = NULL;
  Medium::close(fReadSource); fReadSource = fRTPSource = NULL;
  fClientPortNum = 0;
  return False;
}

void MediaSubsession::deInitiate() {
  Medium::close(fRTCPInstance);
  fRTCPInstance = NULL;

  Medium::close(fReadSource); // this is assumed to also close fRTPSource
  fReadSource = NULL; fRTPSource = NULL;

  delete fRTCPSocket; delete fRTPSocket;
  fRTCPSocket = fRTPSocket = NULL;
}

Boolean MediaSubsession::setClientPortNum(unsigned short portNum) {
  if (fReadSource != NULL) {
    env().setResultMsg("A read source has already been created");
    return False;
  }

  fClientPortNum = portNum;
  return True;
}

netAddressBits MediaSubsession::connectionEndpointAddress() const {
  do {
    // Get the endpoint name from with us, or our parent session:
    char const* endpointString = connectionEndpointName();
    if (endpointString == NULL) {
      endpointString = parentSession().connectionEndpointName();
    }
    if (endpointString == NULL) break;

    // Now, convert this name to an address, if we can:
    NetAddressList addresses(endpointString);
    if (addresses.numAddresses() == 0) break;

    return *(netAddressBits*)(addresses.firstAddress()->data());
  } while (0);

  // No address known:
  return 0;
}

void MediaSubsession::setDestinations(netAddressBits defaultDestAddress) {
  // Get the destination address from the connection endpoint name
  // (This will be 0 if it's not known, in which case we use the default)
  netAddressBits destAddress = connectionEndpointAddress();
  if (destAddress == 0) destAddress = defaultDestAddress;
  struct in_addr destAddr; destAddr.s_addr = destAddress;

  // The destination TTL remains unchanged:
  int destTTL = ~0; // means: don't change

  if (fRTPSocket != NULL) {
    Port destPort(serverPortNum);
    fRTPSocket->changeDestinationParameters(destAddr, destPort, destTTL);
  }
  if (fRTCPSocket != NULL && !isSSM()) {
    // Note: For SSM sessions, the dest address for RTCP was already set.
    Port destPort(serverPortNum+1);
    fRTCPSocket->
      changeDestinationParameters(destAddr, destPort, destTTL);
  }
}

double MediaSubsession::getNormalPlayTime(struct timeval const& presentationTime) {
  // First, check whether our "RTPSource" object has already been synchronized using RTCP.
  // If it hasn't, then - as a special case - we need to use the RTP timestamp to compute the NPT.
  if (rtpSource() == NULL || rtpSource()->timestampFrequency() == 0) return 0.0; // no RTP source, or bad freq!

  if (!rtpSource()->hasBeenSynchronizedUsingRTCP()) {
    if (!rtpInfo.infoIsNew) return 0.0; // the "rtpInfo" structure has not been filled in
    u_int32_t timestampOffset = rtpSource()->curPacketRTPTimestamp() - rtpInfo.timestamp;
    double nptOffset = (timestampOffset/(double)(rtpSource()->timestampFrequency()))*scale();
    double npt = playStartTime() + nptOffset;

    return npt;
  } else {
    // Common case: We have been synchronized using RTCP.  This means that the "presentationTime" parameter
    // will be accurate, and so we should use this to compute the NPT.
    double ptsDouble = (double)(presentationTime.tv_sec + presentationTime.tv_usec/1000000.0);

    if (rtpInfo.infoIsNew) {
      // This is the first time we've been called with a synchronized presentation time since the "rtpInfo"
      // structure was last filled in.  Use this "presentationTime" to compute "fNPT_PTS_Offset":
      u_int32_t timestampOffset = rtpSource()->curPacketRTPTimestamp() - rtpInfo.timestamp;
      double nptOffset = (timestampOffset/(double)(rtpSource()->timestampFrequency()))*scale();
      double npt = playStartTime() + nptOffset;
      fNPT_PTS_Offset = npt - ptsDouble*scale();
      rtpInfo.infoIsNew = False; // for next time

      return npt;
    } else {
      // Use the precomputed "fNPT_PTS_Offset" to compute the NPT from the PTS:
      if (fNPT_PTS_Offset == 0.0) return 0.0; // error: The "rtpInfo" structure was apparently never filled in
      return (double)(ptsDouble*scale() + fNPT_PTS_Offset);
    }
  }
}

Boolean MediaSubsession::parseSDPLine_c(char const* sdpLine) {
  // Check for "c=IN IP4 <connection-endpoint>"
  // or "c=IN IP4 <connection-endpoint>/<ttl+numAddresses>"
  // (Later, do something with <ttl+numAddresses> also #####)
  char* connectionEndpointName = parseCLine(sdpLine);
  if (connectionEndpointName != NULL) {
    delete[] fConnectionEndpointName;
    fConnectionEndpointName = connectionEndpointName;
    return True;
  }

  return False;
}

Boolean MediaSubsession::parseSDPLine_b(char const* sdpLine) {
  // Check for "b=<bwtype>:<bandwidth>" line
  // RTP applications are expected to use bwtype="AS"
  return sscanf(sdpLine, "b=AS:%u", &fBandwidth) == 1;
}

Boolean MediaSubsession::parseSDPAttribute_rtpmap(char const* sdpLine) {
  // Check for a "a=rtpmap:<fmt> <codec>/<freq>" line:
  // (Also check without the "/<freq>"; RealNetworks omits this)
  // Also check for a trailing "/<numChannels>".
  Boolean parseSuccess = False;

  unsigned rtpmapPayloadFormat;
  char* codecName = strDupSize(sdpLine); // ensures we have enough space
  unsigned rtpTimestampFrequency = 0;
  unsigned numChannels = 1;
  if (sscanf(sdpLine, "a=rtpmap: %u %[^/]/%u/%u",
             &rtpmapPayloadFormat, codecName, &rtpTimestampFrequency,
             &numChannels) == 4
      || sscanf(sdpLine, "a=rtpmap: %u %[^/]/%u",
             &rtpmapPayloadFormat, codecName, &rtpTimestampFrequency) == 3
      || sscanf(sdpLine, "a=rtpmap: %u %s",
                &rtpmapPayloadFormat, codecName) == 2) {
    parseSuccess = True;
    if (rtpmapPayloadFormat == fRTPPayloadFormat) {
      // This "rtpmap" matches our payload format, so set our
      // codec name and timestamp frequency:
      // (First, make sure the codec name is upper case)
      {
        Locale l("POSIX");
        for (char* p = codecName; *p != '\0'; ++p) *p = toupper(*p);
      }
      delete[] fCodecName; fCodecName = strDup(codecName);
      fRTPTimestampFrequency = rtpTimestampFrequency;
      fNumChannels = numChannels;
    }
  }
  delete[] codecName;

  return parseSuccess;
}

Boolean MediaSubsession::parseSDPAttribute_control(char const* sdpLine) {
  // Check for a "a=control:<control-path>" line:
  Boolean parseSuccess = False;

  char* controlPath = strDupSize(sdpLine); // ensures we have enough space
  if (sscanf(sdpLine, "a=control: %s", controlPath) == 1) {
    parseSuccess = True;
    delete[] fControlPath; fControlPath = strDup(controlPath);
  }
  delete[] controlPath;

  return parseSuccess;
}

Boolean MediaSubsession::parseSDPAttribute_range(char const* sdpLine) {
  // Check for a "a=range:npt=<startTime>-<endTime>" line:
  // (Later handle other kinds of "a=range" attributes also???#####)
  Boolean parseSuccess = False;

  double playStartTime;
  double playEndTime;
  if (parseRangeAttribute(sdpLine, playStartTime, playEndTime)) {
    parseSuccess = True;
    if (playStartTime > fPlayStartTime) {
      fPlayStartTime = playStartTime;
      if (playStartTime > fParent.playStartTime()) {
        fParent.playStartTime() = playStartTime;
      }
    }
    if (playEndTime > fPlayEndTime) {
      fPlayEndTime = playEndTime;
      if (playEndTime > fParent.playEndTime()) {
        fParent.playEndTime() = playEndTime;
      }
    }
  }

  return parseSuccess;
}

Boolean MediaSubsession::parseSDPAttribute_fmtp(char const* sdpLine) {
  // Check for a "a=fmtp:" line:
  // TEMP: We check only for a handful of expected parameter names #####
  // Later: (i) check that payload format number matches; #####
  //        (ii) look for other parameters also (generalize?) #####
  do {
    if (strncmp(sdpLine, "a=fmtp:", 7) != 0) break; sdpLine += 7;
    while (isdigit(*sdpLine)) ++sdpLine;

    // The remaining "sdpLine" should be a sequence of
    //     <name>=<value>;
    // parameter assignments.  Look at each of these.
    // First, convert the line to lower-case, to ease comparison:
    char* const lineCopy = strDup(sdpLine); char* line = lineCopy;
    {
      Locale l("POSIX");
      for (char* c = line; *c != '\0'; ++c) *c = tolower(*c);
    }
    while (*line != '\0' && *line != '\r' && *line != '\n') {
      unsigned u;
      char* valueStr = strDupSize(line);
      if (sscanf(line, " auxiliarydatasizelength = %u", &u) == 1) {
        fAuxiliarydatasizelength = u;
      } else if (sscanf(line, " constantduration = %u", &u) == 1) {
        fConstantduration = u;
      } else if (sscanf(line, " constantsize; = %u", &u) == 1) {
        fConstantsize = u;
      } else if (sscanf(line, " crc = %u", &u) == 1) {
        fCRC = u;
      } else if (sscanf(line, " ctsdeltalength = %u", &u) == 1) {
        fCtsdeltalength = u;
      } else if (sscanf(line, " de-interleavebuffersize = %u", &u) == 1) {
        fDe_interleavebuffersize = u;
      } else if (sscanf(line, " dtsdeltalength = %u", &u) == 1) {
        fDtsdeltalength = u;
      } else if (sscanf(line, " indexdeltalength = %u", &u) == 1) {
        fIndexdeltalength = u;
      } else if (sscanf(line, " indexlength = %u", &u) == 1) {
        fIndexlength = u;
      } else if (sscanf(line, " interleaving = %u", &u) == 1) {
        fInterleaving = u;
      } else if (sscanf(line, " maxdisplacement = %u", &u) == 1) {
        fMaxdisplacement = u;
      } else if (sscanf(line, " objecttype = %u", &u) == 1) {
        fObjecttype = u;
      } else if (sscanf(line, " octet-align = %u", &u) == 1) {
        fOctetalign = u;
      } else if (sscanf(line, " profile-level-id = %x", &u) == 1) {
        // Note that the "profile-level-id" parameter is assumed to be hexadecimal
        fProfile_level_id = u;
      } else if (sscanf(line, " robust-sorting = %u", &u) == 1) {
        fRobustsorting = u;
      } else if (sscanf(line, " sizelength = %u", &u) == 1) {
        fSizelength = u;
      } else if (sscanf(line, " streamstateindication = %u", &u) == 1) {
        fStreamstateindication = u;
      } else if (sscanf(line, " streamtype = %u", &u) == 1) {
        fStreamtype = u;
      } else if (sscanf(line, " cpresent = %u", &u) == 1) {
        fCpresent = u != 0;
      } else if (sscanf(line, " randomaccessindication = %u", &u) == 1) {
        fRandomaccessindication = u != 0;
      } else if (sscanf(line, " config = %[^; \t\r\n]", valueStr) == 1) {
        delete[] fConfig; fConfig = strDup(valueStr);
      } else if (sscanf(line, " mode = %[^; \t\r\n]", valueStr) == 1) {
        delete[] fMode; fMode = strDup(valueStr);
      } else if (sscanf(sdpLine, " sprop-parameter-sets = %[^; \t\r\n]", valueStr) == 1) {
        // Note: We used "sdpLine" here, because the value is case-sensitive.
        delete[] fSpropParameterSets; fSpropParameterSets = strDup(valueStr);
      } else {
        // Some of the above parameters are Boolean.  Check whether the parameter
        // names appear alone, without a "= 1" at the end:
        if (sscanf(line, " %[^; \t\r\n]", valueStr) == 1) {
          if (strcmp(valueStr, "octet-align") == 0) {
            fOctetalign = 1;
          } else if (strcmp(valueStr, "cpresent") == 0) {
            fCpresent = True;
          } else if (strcmp(valueStr, "crc") == 0) {
            fCRC = 1;
          } else if (strcmp(valueStr, "robust-sorting") == 0) {
            fRobustsorting = 1;
          } else if (strcmp(valueStr, "randomaccessindication") == 0) {
            fRandomaccessindication = True;
          }
        }
      }
      delete[] valueStr;

      // Move to the next parameter assignment string:
      while (*line != '\0' && *line != '\r' && *line != '\n'
             && *line != ';') ++line;
      while (*line == ';') ++line;

      // Do the same with sdpLine; needed for finding case sensitive values:
      while (*sdpLine != '\0' && *sdpLine != '\r' && *sdpLine != '\n'
             && *sdpLine != ';') ++sdpLine;
      while (*sdpLine == ';') ++sdpLine;
    }
    delete[] lineCopy;
    return True;
  } while (0);

  return False;
}

Boolean MediaSubsession
::parseSDPAttribute_source_filter(char const* sdpLine) {
  return parseSourceFilterAttribute(sdpLine, fSourceFilterAddr);
}

Boolean MediaSubsession::parseSDPAttribute_x_dimensions(char const* sdpLine) {
  // Check for a "a=x-dimensions:<width>,<height>" line:
  Boolean parseSuccess = False;

  int width, height;
  if (sscanf(sdpLine, "a=x-dimensions:%d,%d", &width, &height) == 2) {
    parseSuccess = True;
    fVideoWidth = (unsigned short)width;
    fVideoHeight = (unsigned short)height;
  }

  return parseSuccess;
}

Boolean MediaSubsession::parseSDPAttribute_framerate(char const* sdpLine) {
  // Check for a "a=framerate: <fps>" or "a=x-framerate: <fps>" line:
  Boolean parseSuccess = False;

  float frate;
  int rate;
  if (sscanf(sdpLine, "a=framerate: %f", &frate) == 1 || sscanf(sdpLine, "a=framerate:%f", &frate) == 1) {
    parseSuccess = True;
    fVideoFPS = (unsigned)frate;
  } else if (sscanf(sdpLine, "a=x-framerate: %d", &rate) == 1) {
    parseSuccess = True;
    fVideoFPS = (unsigned)rate;
  }

  return parseSuccess;
}
