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
// RTP source for a common kind of payload format: Those which pack multiple,
// complete codec frames (as many as possible) into each RTP packet.
// C++ header

#ifndef _MULTI_FRAMED_RTP_SOURCE_HH
#define _MULTI_FRAMED_RTP_SOURCE_HH

#ifndef _RTP_SOURCE_HH
#include "RTPSource.hh"
#endif

class BufferedPacket; // forward
class BufferedPacketFactory; // forward

class MultiFramedRTPSource: public RTPSource {
protected:
  MultiFramedRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
		       unsigned char rtpPayloadFormat,
		       unsigned rtpTimestampFrequency,
		       BufferedPacketFactory* packetFactory = NULL);
      // virtual base class
  virtual ~MultiFramedRTPSource();

  virtual Boolean processSpecialHeader(BufferedPacket* packet,
				       unsigned& resultSpecialHeaderSize);
      // Subclasses redefine this to handle any special, payload format
      // specific header that follows the RTP header.

  virtual Boolean packetIsUsableInJitterCalculation(unsigned char* packet,
						    unsigned packetSize);
      // The default implementation returns True, but this can be redefined

protected:
  Boolean fCurrentPacketBeginsFrame;
  Boolean fCurrentPacketCompletesFrame;

protected:
  // redefined virtual functions:
  virtual void doStopGettingFrames();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void setPacketReorderingThresholdTime(unsigned uSeconds);

private:
  void reset();
  void doGetNextFrame1();

  static void networkReadHandler(MultiFramedRTPSource* source, int /*mask*/);
  friend void networkReadHandler(MultiFramedRTPSource*, int);

  Boolean fAreDoingNetworkReads;
  Boolean fNeedDelivery;
  Boolean fPacketLossInFragmentedFrame;
  unsigned char* fSavedTo;
  unsigned fSavedMaxSize;

  // A buffer to (optionally) hold incoming pkts that have been reorderered
  class ReorderingPacketBuffer* fReorderingBuffer;
};


// A 'packet data' class that's used to implement the above.
// Note that this can be subclassed - if desired - to redefine
// "nextEnclosedFrameSize()".

class BufferedPacket {
public:
  BufferedPacket();
  virtual ~BufferedPacket();

  Boolean hasUsableData() const { return fTail > fHead; }
  unsigned useCount() const { return fUseCount; }

  Boolean fillInData(RTPInterface& rtpInterface);
  void assignMiscParams(unsigned short rtpSeqNo, unsigned rtpTimestamp,
			struct timeval presentationTime,
			Boolean hasBeenSyncedUsingRTCP,
			Boolean rtpMarkerBit, struct timeval timeReceived);
  void skip(unsigned numBytes); // used to skip over an initial header
  void removePadding(unsigned numBytes); // used to remove trailing bytes
  void appendData(unsigned char* newData, unsigned numBytes);
  void use(unsigned char* to, unsigned toSize,
	   unsigned& bytesUsed, unsigned& bytesTruncated,
	   unsigned short& rtpSeqNo, unsigned& rtpTimestamp,
	   struct timeval& presentationTime,
	   Boolean& hasBeenSyncedUsingRTCP, Boolean& rtpMarkerBit);

  BufferedPacket*& nextPacket() { return fNextPacket; }

  unsigned short rtpSeqNo() const { return fRTPSeqNo; }
  struct timeval const& timeReceived() const { return fTimeReceived; }

  unsigned char* data() const { return &fBuf[fHead]; }
  unsigned dataSize() const { return fTail-fHead; }
  Boolean rtpMarkerBit() const { return fRTPMarkerBit; }
  Boolean& isFirstPacket() { return fIsFirstPacket; }

protected:
  virtual void reset();
  virtual unsigned nextEnclosedFrameSize(unsigned char*& framePtr,
					 unsigned dataSize);
      // The above function has been deprecated.  Instead, new subclasses should use:
  virtual void getNextEnclosedFrameParameters(unsigned char*& framePtr,
					      unsigned dataSize,
					      unsigned& frameSize,
					      unsigned& frameDurationInMicroseconds);

  unsigned fPacketSize;
  unsigned char* fBuf;
  unsigned fHead;
  unsigned fTail;

private:
  BufferedPacket* fNextPacket; // used to link together packets

  unsigned fUseCount;
  unsigned short fRTPSeqNo;
  unsigned fRTPTimestamp;
  struct timeval fPresentationTime; // corresponding to "fRTPTimestamp"
  Boolean fHasBeenSyncedUsingRTCP;
  Boolean fRTPMarkerBit;
  Boolean fIsFirstPacket;
  struct timeval fTimeReceived;
};

// A 'factory' class for creating "BufferedPacket" objects.
// If you want to subclass "BufferedPacket", then you'll also
// want to subclass this, to redefine createNewPacket()

class BufferedPacketFactory {
public:
  BufferedPacketFactory();
  virtual ~BufferedPacketFactory();

  virtual BufferedPacket* createNewPacket(MultiFramedRTPSource* ourSource);
};

#endif
