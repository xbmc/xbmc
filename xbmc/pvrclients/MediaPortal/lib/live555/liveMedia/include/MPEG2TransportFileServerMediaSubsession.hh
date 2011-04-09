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
// on demand, from a MPEG-2 Transport Stream file.
// C++ header

#ifndef _MPEG2_TRANSPORT_FILE_SERVER_MEDIA_SUBSESSION_HH
#define _MPEG2_TRANSPORT_FILE_SERVER_MEDIA_SUBSESSION_HH

#ifndef _FILE_SERVER_MEDIA_SUBSESSION_HH
#include "FileServerMediaSubsession.hh"
#endif
#ifndef _MPEG2_TRANSPORT_STREAM_INDEX_FILE_HH
#include "MPEG2TransportStreamIndexFile.hh"
#endif

class ClientTrickPlayState; // forward

class MPEG2TransportFileServerMediaSubsession: public FileServerMediaSubsession{
public:
  static MPEG2TransportFileServerMediaSubsession*
  createNew(UsageEnvironment& env,
	    char const* dataFileName, char const* indexFileName,
	    Boolean reuseFirstSource);

protected:
  MPEG2TransportFileServerMediaSubsession(UsageEnvironment& env,
					  char const* fileName,
					  MPEG2TransportStreamIndexFile* indexFile,
					  Boolean reuseFirstSource);
      // called only by createNew();
  virtual ~MPEG2TransportFileServerMediaSubsession();

private: // redefined virtual functions
  // Note that because - to implement 'trick play' operations - we're operating on
  // more than just the input source, we reimplement some functions that are
  // already implemented in "OnDemandServerMediaSubsession", rather than
  // reimplementing "seekStreamSource()" and "setStreamSourceScale()":
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

  // The virtual functions thare are usually implemented by "ServerMediaSubsession"s:
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);

  virtual void testScaleFactor(float& scale);
  virtual float duration() const;

private:
  ClientTrickPlayState* lookupClient(unsigned clientSessionId);

private:
  MPEG2TransportStreamIndexFile* fIndexFile;
  float fDuration;
  HashTable* fClientSessionHashTable; // indexed by client session id
};

#endif
