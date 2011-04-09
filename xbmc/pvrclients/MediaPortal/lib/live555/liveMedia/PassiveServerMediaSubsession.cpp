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
// A 'ServerMediaSubsession' object that represents an existing
// 'RTPSink', rather than one that creates new 'RTPSink's on demand.
// Implementation

#include "PassiveServerMediaSubsession.hh"
#include <GroupsockHelper.hh>

////////// PassiveServerMediaSubsession //////////

PassiveServerMediaSubsession*
PassiveServerMediaSubsession::createNew(RTPSink& rtpSink,
					RTCPInstance* rtcpInstance) {
  return new PassiveServerMediaSubsession(rtpSink, rtcpInstance);
}

PassiveServerMediaSubsession
::PassiveServerMediaSubsession(RTPSink& rtpSink, RTCPInstance* rtcpInstance)
  : ServerMediaSubsession(rtpSink.envir()),
    fSDPLines(NULL), fRTPSink(rtpSink), fRTCPInstance(rtcpInstance) {
}

char const*
PassiveServerMediaSubsession::sdpLines() {
  if (fSDPLines == NULL ) {
    // Construct a set of SDP lines that describe this subsession:
    // Use the components from "rtpSink":
    Groupsock const& gs = fRTPSink.groupsockBeingUsed();
    struct in_addr const& ipAddress = gs.groupAddress();
    unsigned short portNum = ntohs(gs.port().num());
    unsigned char ttl = gs.ttl();
    unsigned char rtpPayloadType = fRTPSink.rtpPayloadType();
    char const* mediaType = fRTPSink.sdpMediaType();
    unsigned estBitrate
      = fRTCPInstance == NULL ? 50 : fRTCPInstance->totSessionBW();
    char* rtpmapLine = fRTPSink.rtpmapLine();
    char const* rangeLine = rangeSDPLine();
    char const* auxSDPLine = fRTPSink.auxSDPLine();
    if (auxSDPLine == NULL) auxSDPLine = "";

    char* const ipAddressStr = strDup(our_inet_ntoa(ipAddress));

    char const* const sdpFmt =
      "m=%s %d RTP/AVP %d\r\n"
      "c=IN IP4 %s/%d\r\n"
      "b=AS:%u\r\n"
      "%s"
      "%s"
      "%s"
      "a=control:%s\r\n";
    unsigned sdpFmtSize = strlen(sdpFmt)
      + strlen(mediaType) + 5 /* max short len */ + 3 /* max char len */
      + strlen(ipAddressStr) + 3 /* max char len */
      + 20 /* max int len */
      + strlen(rtpmapLine)
      + strlen(rangeLine)
      + strlen(auxSDPLine)
      + strlen(trackId());
    char* sdpLines = new char[sdpFmtSize];
    sprintf(sdpLines, sdpFmt,
	    mediaType, // m= <media>
	    portNum, // m= <port>
	    rtpPayloadType, // m= <fmt list>
	    ipAddressStr, // c= <connection address>
	    ttl, // c= TTL
	    estBitrate, // b=AS:<bandwidth>
	    rtpmapLine, // a=rtpmap:... (if present)
	    rangeLine, // a=range:... (if present)
	    auxSDPLine, // optional extra SDP line
	    trackId()); // a=control:<track-id>
    delete[] ipAddressStr; delete[] (char*)rangeLine; delete[] rtpmapLine;

    fSDPLines = strDup(sdpLines);
    delete[] sdpLines;
  }

  return fSDPLines;
}

void PassiveServerMediaSubsession
::getStreamParameters(unsigned /*clientSessionId*/,
		      netAddressBits /*clientAddress*/,
		      Port const& /*clientRTPPort*/,
		      Port const& /*clientRTCPPort*/,
		      int /*tcpSocketNum*/,
		      unsigned char /*rtpChannelId*/,
		      unsigned char /*rtcpChannelId*/,
		      netAddressBits& destinationAddress,
		      u_int8_t& destinationTTL,
		      Boolean& isMulticast,
		      Port& serverRTPPort,
		      Port& serverRTCPPort,
		      void*& streamToken) {
  isMulticast = True;
  Groupsock& gs = fRTPSink.groupsockBeingUsed();
  if (destinationTTL == 255) destinationTTL = gs.ttl();
  if (destinationAddress == 0) { // normal case
    destinationAddress = gs.groupAddress().s_addr;
  } else { // use the client-specified destination address instead:
    struct in_addr destinationAddr; destinationAddr.s_addr = destinationAddress;
    gs.changeDestinationParameters(destinationAddr, 0, destinationTTL);
    if (fRTCPInstance != NULL) {
      Groupsock* rtcpGS = fRTCPInstance->RTCPgs();
      rtcpGS->changeDestinationParameters(destinationAddr, 0, destinationTTL);
    }
  }
  serverRTPPort = gs.port();
  if (fRTCPInstance != NULL) {
    Groupsock* rtcpGS = fRTCPInstance->RTCPgs();
    serverRTCPPort = rtcpGS->port();
  }
  streamToken = NULL; // not used
}

void PassiveServerMediaSubsession::startStream(unsigned /*clientSessionId*/,
					       void* /*streamToken*/,
					       TaskFunc* /*rtcpRRHandler*/,
					       void* /*rtcpRRHandlerClientData*/,
					       unsigned short& rtpSeqNum,
					       unsigned& rtpTimestamp,
					       ServerRequestAlternativeByteHandler* /*serverRequestAlternativeByteHandler*/,
					       void* /*serverRequestAlternativeByteHandlerClientData*/) {
  // Note: We don't set a RTCP RR handler, because (i) we're called potentially
  // many times on the same "RTCPInstance", and (ii) the "RTCPInstance" remains
  // in existence after "deleteStream()" is called.
  rtpSeqNum = fRTPSink.currentSeqNo();
  rtpTimestamp = fRTPSink.presetNextTimestamp();

  // Try to use a big send buffer for RTP -  at least 0.1 second of
  // specified bandwidth and at least 50 KB
  unsigned streamBitrate = fRTCPInstance == NULL ? 50 : fRTCPInstance->totSessionBW(); // in kbps
  unsigned rtpBufSize = streamBitrate * 25 / 2; // 1 kbps * 0.1 s = 12.5 bytes
  if (rtpBufSize < 50 * 1024) rtpBufSize = 50 * 1024;
  increaseSendBufferTo(envir(), fRTPSink.groupsockBeingUsed().socketNum(), rtpBufSize);
}

PassiveServerMediaSubsession::~PassiveServerMediaSubsession() {
  delete[] fSDPLines;
}
