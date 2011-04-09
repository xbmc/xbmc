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
// Media Sinks
// C++ header

#ifndef _MEDIA_SINK_HH
#define _MEDIA_SINK_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

class MediaSink: public Medium {
public:
  static Boolean lookupByName(UsageEnvironment& env, char const* sinkName,
			      MediaSink*& resultSink);

  typedef void (afterPlayingFunc)(void* clientData);
  Boolean startPlaying(MediaSource& source,
		       afterPlayingFunc* afterFunc,
		       void* afterClientData);
  virtual void stopPlaying();

  // Test for specific types of sink:
  virtual Boolean isRTPSink() const;

  FramedSource* source() const {return fSource;}

protected:
  MediaSink(UsageEnvironment& env); // abstract base class
  virtual ~MediaSink();

  virtual Boolean sourceIsCompatibleWithUs(MediaSource& source);
      // called by startPlaying()
  virtual Boolean continuePlaying() = 0;
      // called by startPlaying()

  static void onSourceClosure(void* clientData);
      // should be called (on ourselves) by continuePlaying() when it
      // discovers that the source we're playing from has closed.

  FramedSource* fSource;

private:
  // redefined virtual functions:
  virtual Boolean isSink() const;

private:
  // The following fields are used when we're being played:
  afterPlayingFunc* fAfterFunc;
  void* fAfterClientData;
};

// A data structure that a sink may use for an output packet:
class OutPacketBuffer {
public:
  OutPacketBuffer(unsigned preferredPacketSize, unsigned maxPacketSize);
  ~OutPacketBuffer();

  static unsigned maxSize;

  unsigned char* curPtr() const {return &fBuf[fPacketStart + fCurOffset];}
  unsigned totalBytesAvailable() const {
    return fLimit - (fPacketStart + fCurOffset);
  }
  unsigned totalBufferSize() const { return fLimit; }
  unsigned char* packet() const {return &fBuf[fPacketStart];}
  unsigned curPacketSize() const {return fCurOffset;}

  void increment(unsigned numBytes) {fCurOffset += numBytes;}

  void enqueue(unsigned char const* from, unsigned numBytes);
  void enqueueWord(unsigned word);
  void insert(unsigned char const* from, unsigned numBytes, unsigned toPosition);
  void insertWord(unsigned word, unsigned toPosition);
  void extract(unsigned char* to, unsigned numBytes, unsigned fromPosition);
  unsigned extractWord(unsigned fromPosition);

  void skipBytes(unsigned numBytes);

  Boolean isPreferredSize() const {return fCurOffset >= fPreferred;}
  Boolean wouldOverflow(unsigned numBytes) const {
    return (fCurOffset+numBytes) > fMax;
  }
  unsigned numOverflowBytes(unsigned numBytes) const {
    return (fCurOffset+numBytes) - fMax;
  }
  Boolean isTooBigForAPacket(unsigned numBytes) const {
    return numBytes > fMax;
  }

  void setOverflowData(unsigned overflowDataOffset,
		       unsigned overflowDataSize,
		       struct timeval const& presentationTime,
		       unsigned durationInMicroseconds);
  unsigned overflowDataSize() const {return fOverflowDataSize;}
  struct timeval overflowPresentationTime() const {return fOverflowPresentationTime;}
  unsigned overflowDurationInMicroseconds() const {return fOverflowDurationInMicroseconds;}
  Boolean haveOverflowData() const {return fOverflowDataSize > 0;}
  void useOverflowData();

  void adjustPacketStart(unsigned numBytes);
  void resetPacketStart();
  void resetOffset() { fCurOffset = 0; }
  void resetOverflowData() { fOverflowDataOffset = fOverflowDataSize = 0; }

private:
  unsigned fPacketStart, fCurOffset, fPreferred, fMax, fLimit;
  unsigned char* fBuf;

  unsigned fOverflowDataOffset, fOverflowDataSize;
  struct timeval fOverflowPresentationTime;
  unsigned fOverflowDurationInMicroseconds;
};

#endif
