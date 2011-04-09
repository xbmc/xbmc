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
// An object that redirects one or more RTP/RTCP streams - forming a single
// multimedia session - into a 'Darwin Streaming Server' (for subsequent
// reflection to potentially arbitrarily many remote RTSP clients).
// Implementation

#include "DarwinInjector.hh"
#include <GroupsockHelper.hh>

////////// SubstreamDescriptor definition //////////

class SubstreamDescriptor {
public:
  SubstreamDescriptor(RTPSink* rtpSink, RTCPInstance* rtcpInstance, unsigned trackId);
  ~SubstreamDescriptor();

  SubstreamDescriptor*& next() { return fNext; }
  RTPSink* rtpSink() const { return fRTPSink; }
  RTCPInstance* rtcpInstance() const { return fRTCPInstance; }
  char const* sdpLines() const { return fSDPLines; }

private:
  SubstreamDescriptor* fNext;
  RTPSink* fRTPSink;
  RTCPInstance* fRTCPInstance;
  char* fSDPLines;
};


////////// DarwinInjector implementation //////////

DarwinInjector* DarwinInjector::createNew(UsageEnvironment& env,
					  char const* applicationName,
					  int verbosityLevel) {
  return new DarwinInjector(env, applicationName, verbosityLevel);
}

Boolean DarwinInjector::lookupByName(UsageEnvironment& env, char const* name,
				     DarwinInjector*& result) {
  result = NULL; // unless we succeed

  Medium* medium;
  if (!Medium::lookupByName(env, name, medium)) return False;

  if (!medium->isDarwinInjector()) {
    env.setResultMsg(name, " is not a 'Darwin injector'");
    return False;
  }

  result = (DarwinInjector*)medium;
  return True;
}

DarwinInjector::DarwinInjector(UsageEnvironment& env,
			       char const* applicationName, int verbosityLevel)
  : Medium(env),
    fApplicationName(strDup(applicationName)), fVerbosityLevel(verbosityLevel),
    fRTSPClient(NULL), fSubstreamSDPSizes(0),
    fHeadSubstream(NULL), fTailSubstream(NULL), fSession(NULL), fLastTrackId(0) {
}

DarwinInjector::~DarwinInjector() {
  if (fSession != NULL) { // close down and delete the session
    fRTSPClient->teardownMediaSession(*fSession);
    Medium::close(fSession);
  }

  delete fHeadSubstream;
  delete[] (char*)fApplicationName;
  Medium::close(fRTSPClient);
}

void DarwinInjector::addStream(RTPSink* rtpSink, RTCPInstance* rtcpInstance) {
  if (rtpSink == NULL) return; // "rtpSink" should be non-NULL

  SubstreamDescriptor* newDescriptor = new SubstreamDescriptor(rtpSink, rtcpInstance, ++fLastTrackId);
  if (fHeadSubstream == NULL) {
    fHeadSubstream = fTailSubstream = newDescriptor;
  } else {
    fTailSubstream->next() = newDescriptor;
    fTailSubstream = newDescriptor;
  }

  fSubstreamSDPSizes += strlen(newDescriptor->sdpLines());
}

Boolean DarwinInjector
::setDestination(char const* remoteRTSPServerNameOrAddress,
		 char const* remoteFileName,
		 char const* sessionName,
		 char const* sessionInfo,
		 portNumBits remoteRTSPServerPortNumber,
		 char const* remoteUserName,
		 char const* remotePassword,
		 char const* sessionAuthor,
		 char const* sessionCopyright,
		 int timeout) {
  char* sdp = NULL;
  char* url = NULL;
  Boolean success = False; // until we learn otherwise

  do {
    // Begin by creating our RTSP client object:
    fRTSPClient = RTSPClient::createNew(envir(), fVerbosityLevel, fApplicationName);
    if (fRTSPClient == NULL) break;

    // Get the remote RTSP server's IP address:
    struct in_addr addr;
    {
      NetAddressList addresses(remoteRTSPServerNameOrAddress);
      if (addresses.numAddresses() == 0) break;
      NetAddress const* address = addresses.firstAddress();
      addr.s_addr = *(unsigned*)(address->data());
    }
    char const* remoteRTSPServerAddressStr = our_inet_ntoa(addr);

    // Construct a SDP description for the session that we'll be streaming:
    char const* const sdpFmt =
      "v=0\r\n"
      "o=- %u %u IN IP4 127.0.0.1\r\n"
      "s=%s\r\n"
      "i=%s\r\n"
      "c=IN IP4 %s\r\n"
      "t=0 0\r\n"
      "a=x-qt-text-nam:%s\r\n"
      "a=x-qt-text-inf:%s\r\n"
      "a=x-qt-text-cmt:source application:%s\r\n"
      "a=x-qt-text-aut:%s\r\n"
      "a=x-qt-text-cpy:%s\r\n";
      // plus, %s for each substream SDP
    unsigned sdpLen = strlen(sdpFmt)
      + 20 /* max int len */ + 20 /* max int len */
      + strlen(sessionName)
      + strlen(sessionInfo)
      + strlen(remoteRTSPServerAddressStr)
      + strlen(sessionName)
      + strlen(sessionInfo)
      + strlen(fApplicationName)
      + strlen(sessionAuthor)
      + strlen(sessionCopyright)
      + fSubstreamSDPSizes;
    unsigned const sdpSessionId = our_random();
    unsigned const sdpVersion = sdpSessionId;
    sdp = new char[sdpLen];
    sprintf(sdp, sdpFmt,
	    sdpSessionId, sdpVersion, // o= line
	    sessionName, // s= line
	    sessionInfo, // i= line
	    remoteRTSPServerAddressStr, // c= line
	    sessionName, // a=x-qt-text-nam: line
	    sessionInfo, // a=x-qt-text-inf: line
	    fApplicationName, // a=x-qt-text-cmt: line
	    sessionAuthor, // a=x-qt-text-aut: line
	    sessionCopyright // a=x-qt-text-cpy: line
	    );
    char* p = &sdp[strlen(sdp)];
    SubstreamDescriptor* ss;
    for (ss = fHeadSubstream; ss != NULL; ss = ss->next()) {
      sprintf(p, "%s", ss->sdpLines());
      p += strlen(p);
    }

    // Construct a RTSP URL for the remote stream:
    char const* const urlFmt = "rtsp://%s:%u/%s";
    unsigned urlLen
      = strlen(urlFmt) + strlen(remoteRTSPServerNameOrAddress) + 5 /* max short len */ + strlen(remoteFileName);
    url = new char[urlLen];
    sprintf(url, urlFmt, remoteRTSPServerNameOrAddress, remoteRTSPServerPortNumber, remoteFileName);

    // Do a RTSP "ANNOUNCE" with this SDP description:
    Boolean announceSuccess;
    if (remoteUserName[0] != '\0' || remotePassword[0] != '\0') {
      announceSuccess
	= fRTSPClient->announceWithPassword(url, sdp, remoteUserName, remotePassword, timeout);
    } else {
      announceSuccess = fRTSPClient->announceSDPDescription(url, sdp, NULL, timeout);
    }
    if (!announceSuccess) break;

    // Tell the remote server to start receiving the stream from us.
    // (To do this, we first create a "MediaSession" object from the SDP description.)
    fSession = MediaSession::createNew(envir(), sdp);
    if (fSession == NULL) break;

    ss = fHeadSubstream;
    MediaSubsessionIterator iter(*fSession);
    MediaSubsession* subsession;
    ss = fHeadSubstream;
    unsigned streamChannelId = 0;
    while ((subsession = iter.next()) != NULL) {
      if (!subsession->initiate()) break;

      if (!fRTSPClient->setupMediaSubsession(*subsession,
					     True /*streamOutgoing*/,
					     True /*streamUsingTCP*/)) {
	break;
      }

      // Tell this subsession's RTPSink and RTCPInstance to use
      // the RTSP TCP connection:
      ss->rtpSink()->setStreamSocket(fRTSPClient->socketNum(), streamChannelId++);
      if (ss->rtcpInstance() != NULL) {
	ss->rtcpInstance()->setStreamSocket(fRTSPClient->socketNum(),
					    streamChannelId++);
      }
      ss = ss->next();
    }
    if (subsession != NULL) break; // an error occurred above

    // Tell the RTSP server to start:
    if (!fRTSPClient->playMediaSession(*fSession)) break;

    // Finally, make sure that the output TCP buffer is a reasonable size:
    increaseSendBufferTo(envir(), fRTSPClient->socketNum(), 100*1024);

    success = True;
  } while (0);

  delete[] sdp;
  delete[] url;
  return success;
}

Boolean DarwinInjector::isDarwinInjector() const {
  return True;
}


////////// SubstreamDescriptor implementation //////////

SubstreamDescriptor::SubstreamDescriptor(RTPSink* rtpSink,
					 RTCPInstance* rtcpInstance, unsigned trackId)
  : fNext(NULL), fRTPSink(rtpSink), fRTCPInstance(rtcpInstance) {
  // Create the SDP description for this substream
  char const* mediaType = fRTPSink->sdpMediaType();
  unsigned char rtpPayloadType = fRTPSink->rtpPayloadType();
  char const* rtpPayloadFormatName = fRTPSink->rtpPayloadFormatName();
  unsigned rtpTimestampFrequency = fRTPSink->rtpTimestampFrequency();
  unsigned numChannels = fRTPSink->numChannels();
  char* rtpmapLine;
  if (rtpPayloadType >= 96) {
    char* encodingParamsPart;
    if (numChannels != 1) {
      encodingParamsPart = new char[1 + 20 /* max int len */];
      sprintf(encodingParamsPart, "/%d", numChannels);
    } else {
      encodingParamsPart = strDup("");
    }
    char const* const rtpmapFmt = "a=rtpmap:%d %s/%d%s\r\n";
    unsigned rtpmapFmtSize = strlen(rtpmapFmt)
      + 3 /* max char len */ + strlen(rtpPayloadFormatName)
      + 20 /* max int len */ + strlen(encodingParamsPart);
    rtpmapLine = new char[rtpmapFmtSize];
    sprintf(rtpmapLine, rtpmapFmt,
            rtpPayloadType, rtpPayloadFormatName,
            rtpTimestampFrequency, encodingParamsPart);
    delete[] encodingParamsPart;
  } else {
    // Static payload type => no "a=rtpmap:" line
    rtpmapLine = strDup("");
  }
  unsigned rtpmapLineSize = strlen(rtpmapLine);
  char const* auxSDPLine = fRTPSink->auxSDPLine();
  if (auxSDPLine == NULL) auxSDPLine = "";
  unsigned auxSDPLineSize = strlen(auxSDPLine);

  char const* const sdpFmt =
    "m=%s 0 RTP/AVP %u\r\n"
    "%s" // "a=rtpmap:" line (if present)
    "%s" // auxilliary (e.g., "a=fmtp:") line (if present)
    "a=control:trackID=%u\r\n";
  unsigned sdpFmtSize = strlen(sdpFmt)
    + strlen(mediaType) + 3 /* max char len */
    + rtpmapLineSize
    + auxSDPLineSize
    + 20 /* max int len */;
  char* sdpLines = new char[sdpFmtSize];
  sprintf(sdpLines, sdpFmt,
          mediaType, // m= <media>
          rtpPayloadType, // m= <fmt list>
          rtpmapLine, // a=rtpmap:... (if present)
          auxSDPLine, // optional extra SDP line
          trackId); // a=control:<track-id>
  fSDPLines = strDup(sdpLines);
  delete[] sdpLines;
  delete[] rtpmapLine;
}

SubstreamDescriptor::~SubstreamDescriptor() {
  delete fSDPLines;
  delete fNext;
}
