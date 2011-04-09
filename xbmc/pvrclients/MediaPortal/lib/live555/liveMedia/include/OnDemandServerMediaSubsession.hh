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
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand.
// C++ header

#ifndef _ON_DEMAND_SERVER_MEDIA_SUBSESSION_HH
#define _ON_DEMAND_SERVER_MEDIA_SUBSESSION_HH

#ifndef _SERVER_MEDIA_SESSION_HH
#include "ServerMediaSession.hh"
#endif
#ifndef _RTP_SINK_HH
#include "RTPSink.hh"
#endif

class OnDemandServerMediaSubsession: public ServerMediaSubsession {
protected: // we're a virtual base class
  OnDemandServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource,
				portNumBits initialPortNum = 6970);
  virtual ~OnDemandServerMediaSubsession();

protected: // redefined virtual functions
  virtual char const* sdpLines();
  virtual void getStreamParameters(unsigned clientSessionId,
				   netAddressBits clientAddress,
                                   Port const& clientRTPPort,
                                   Port const& clientRTCPPort,
				   int tcpSocketNum,
                                   unsigned char rtpChannelId,
                                   unsigned char rtcpChannelId,
                                   netAddressBits& destinationAddress,
				   u_int8_t& destinationTTL,
                                   Boolean& isMulticast,
                                   Port& serverRTPPort,
                                   Port& serverRTCPPort,
                                   void*& streamToken);
  virtual void startStream(unsigned clientSessionId, void* streamToken,
			   TaskFunc* rtcpRRHandler,
			   void* rtcpRRHandlerClientData,
			   unsigned short& rtpSeqNum,
                           unsigned& rtpTimestamp,
			   ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
                           void* serverRequestAlternativeByteHandlerClientData);
  virtual void pauseStream(unsigned clientSessionId, void* streamToken);
  virtual void seekStream(unsigned clientSessionId, void* streamToken, double seekNPT);
  virtual void setStreamScale(unsigned clientSessionId, void* streamToken, float scale);
  virtual void deleteStream(unsigned clientSessionId, void*& streamToken);

protected: // new virtual functions, possibly redefined by subclasses
  virtual char const* getAuxSDPLine(RTPSink* rtpSink,
				    FramedSource* inputSource);
  virtual void seekStreamSource(FramedSource* inputSource, double seekNPT);
  virtual void setStreamSourceScale(FramedSource* inputSource, float scale);
  virtual void closeStreamSource(FramedSource *inputSource);

protected: // new virtual functions, defined by all subclasses
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate) = 0;
      // "estBitrate" is the stream's estimated bitrate, in kbps
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
				    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource) = 0;

private:
  void setSDPLinesFromRTPSink(RTPSink* rtpSink, FramedSource* inputSource,
			      unsigned estBitrate);
      // used to implement "sdpLines()"

protected:
  char* fSDPLines;

private:
  Boolean fReuseFirstSource;
  portNumBits fInitialPortNum;
  HashTable* fDestinationsHashTable; // indexed by client session id
  void* fLastStreamToken;
  char fCNAME[100]; // for RTCP
  friend class StreamState;
};

#endif
