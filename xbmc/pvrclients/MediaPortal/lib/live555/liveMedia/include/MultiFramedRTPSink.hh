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
// RTP sink for a common kind of payload format: Those which pack multiple,
// complete codec frames (as many as possible) into each RTP packet.
// C++ header

#ifndef _MULTI_FRAMED_RTP_SINK_HH
#define _MULTI_FRAMED_RTP_SINK_HH

#ifndef _RTP_SINK_HH
#include "RTPSink.hh"
#endif

class MultiFramedRTPSink: public RTPSink {
public:
  void setPacketSizes(unsigned preferredPacketSize, unsigned maxPacketSize);

protected:
  MultiFramedRTPSink(UsageEnvironment& env,
		     Groupsock* rtpgs, unsigned char rtpPayloadType,
		     unsigned rtpTimestampFrequency,
		     char const* rtpPayloadFormatName,
		     unsigned numChannels = 1);
	// we're a virtual base class

  virtual ~MultiFramedRTPSink();

  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
				      unsigned char* frameStart,
				      unsigned numBytesInFrame,
				      struct timeval frameTimestamp,
				      unsigned numRemainingBytes);
      // perform any processing specific to the particular payload format
  virtual Boolean allowFragmentationAfterStart() const;
      // whether a frame can be fragmented if other frame(s) appear earlier
      // in the packet (by default: False)
  virtual Boolean allowOtherFramesAfterLastFragment() const;
      // whether other frames can be packed into a packet following the
      // final fragment of a previous, fragmented frame (by default: False)
  virtual Boolean frameCanAppearAfterPacketStart(unsigned char const* frameStart,
					 unsigned numBytesInFrame) const;
      // whether this frame can appear in position >1 in a pkt (default: True)
  virtual unsigned specialHeaderSize() const;
      // returns the size of any special header used (following the RTP header) (default: 0)
  virtual unsigned frameSpecificHeaderSize() const;
      // returns the size of any frame-specific header used (before each frame
      // within the packet) (default: 0)
  virtual unsigned computeOverflowForNewFrame(unsigned newFrameSize) const;
      // returns the number of overflow bytes that would be produced by adding a new
      // frame of size "newFrameSize" to the current RTP packet.
      // (By default, this just calls "numOverflowBytes()", but subclasses can redefine
      // this to (e.g.) impose a granularity upon RTP payload fragments.)

  // Functions that might be called by doSpecialFrameHandling(), or other subclass virtual functions:
  Boolean isFirstPacket() const { return fIsFirstPacket; }
  Boolean isFirstFrameInPacket() const { return fNumFramesUsedSoFar == 0; }
  Boolean curFragmentationOffset() const { return fCurFragmentationOffset; }
  void setMarkerBit();
  void setTimestamp(struct timeval timestamp);
  void setSpecialHeaderWord(unsigned word, /* 32 bits, in host order */
			    unsigned wordPosition = 0);
  void setSpecialHeaderBytes(unsigned char const* bytes, unsigned numBytes,
			     unsigned bytePosition = 0);
  void setFrameSpecificHeaderWord(unsigned word, /* 32 bits, in host order */
				  unsigned wordPosition = 0);
  void setFrameSpecificHeaderBytes(unsigned char const* bytes, unsigned numBytes,
				   unsigned bytePosition = 0);
  void setFramePadding(unsigned numPaddingBytes);
  unsigned numFramesUsedSoFar() const { return fNumFramesUsedSoFar; }
  unsigned ourMaxPacketSize() const { return fOurMaxPacketSize; }

public: // redefined virtual functions:
  virtual void stopPlaying();

protected: // redefined virtual functions:
  virtual Boolean continuePlaying();

private:
  void buildAndSendPacket(Boolean isFirstPacket);
  void packFrame();
  void sendPacketIfNecessary();
  static void sendNext(void* firstArg);
  friend void sendNext(void*);

  static void afterGettingFrame(void* clientData,
				unsigned numBytesRead, unsigned numTruncatedBytes,
				struct timeval presentationTime,
				unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned numBytesRead, unsigned numTruncatedBytes,
			  struct timeval presentationTime,
			  unsigned durationInMicroseconds);
  Boolean isTooBigForAPacket(unsigned numBytes) const;

  static void ourHandleClosure(void* clientData);

private:
  OutPacketBuffer* fOutBuf;

  Boolean fNoFramesLeft;
  unsigned fNumFramesUsedSoFar;
  unsigned fCurFragmentationOffset;
  Boolean fPreviousFrameEndedFragmentation;

  Boolean fIsFirstPacket;
  struct timeval fNextSendTime;
  unsigned fTimestampPosition;
  unsigned fSpecialHeaderPosition;
  unsigned fSpecialHeaderSize; // size in bytes of any special header used
  unsigned fCurFrameSpecificHeaderPosition;
  unsigned fCurFrameSpecificHeaderSize; // size in bytes of cur frame-specific header
  unsigned fTotalFrameSpecificHeaderSizes; // size of all frame-specific hdrs in pkt
  unsigned fOurMaxPacketSize;
};

#endif
