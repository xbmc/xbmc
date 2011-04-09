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
// A filter that produces a sequence of I-frame indices from a MPEG-2 Transport Stream
// C++ header

#ifndef _MPEG2_IFRAME_INDEX_FROM_TRANSPORT_STREAM_HH
#define _MPEG2_IFRAME_INDEX_FROM_TRANSPORT_STREAM_HH

#ifndef _FRAMED_FILTER_HH
#include "FramedFilter.hh"
#endif

#ifndef TRANSPORT_PACKET_SIZE
#define TRANSPORT_PACKET_SIZE 188
#endif

#ifndef MAX_PES_PACKET_SIZE
#define MAX_PES_PACKET_SIZE 65536
#endif

class IndexRecord; // forward

class MPEG2IFrameIndexFromTransportStream: public FramedFilter {
public:
  static MPEG2IFrameIndexFromTransportStream*
  createNew(UsageEnvironment& env, FramedSource* inputSource);

protected:
  MPEG2IFrameIndexFromTransportStream(UsageEnvironment& env,
				      FramedSource* inputSource);
      // called only by createNew()
  virtual ~MPEG2IFrameIndexFromTransportStream();

private:
  // Redefined virtual functions:
  virtual void doGetNextFrame();

private:
  static void afterGettingFrame(void* clientData, unsigned frameSize,
				unsigned numTruncatedBytes,
				struct timeval presentationTime,
				unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned frameSize,
			  unsigned numTruncatedBytes,
			  struct timeval presentationTime,
			  unsigned durationInMicroseconds);

  static void handleInputClosure(void* clientData);
  void handleInputClosure1();

  void analyzePAT(unsigned char* pkt, unsigned size);
  void analyzePMT(unsigned char* pkt, unsigned size);

  Boolean deliverIndexRecord();
  Boolean parseFrame();
  Boolean parseToNextCode(unsigned char& nextCode);
  void compactParseBuffer();
  void addToTail(IndexRecord* newIndexRecord);

private:
  unsigned long fInputTransportPacketCounter;
  unsigned fClosureNumber;
  u_int8_t fLastContinuityCounter;
  float fFirstPCR, fLastPCR;
  Boolean fHaveSeenFirstPCR;
  u_int16_t fPMT_PID, fVideo_PID;
      // Note: We assume: 1 program per Transport Stream; 1 video stream per program
  unsigned char fInputBuffer[TRANSPORT_PACKET_SIZE];
  unsigned char* fParseBuffer;
  unsigned fParseBufferSize;
  unsigned fParseBufferFrameStart;
  unsigned fParseBufferParseEnd;
  unsigned fParseBufferDataEnd;
  IndexRecord* fHeadIndexRecord;
  IndexRecord* fTailIndexRecord;
};

#endif
