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
// Interleaving of MP3 ADUs
// C++ header

#ifndef _MP3_ADU_INTERLEAVING_HH
#define _MP3_ADU_INTERLEAVING_HH

#ifndef _FRAMED_FILTER_HH
#include "FramedFilter.hh"
#endif

// A data structure used to represent an interleaving
#define MAX_CYCLE_SIZE 256
class Interleaving {
public:
  Interleaving(unsigned cycleSize, unsigned char const* cycleArray);
  virtual ~Interleaving();

  unsigned cycleSize() const {return fCycleSize;}
  unsigned char lookupInverseCycle(unsigned char index) const {
    return fInverseCycle[index];
  }

private:
  unsigned fCycleSize;
  unsigned char fInverseCycle[MAX_CYCLE_SIZE];
};

// This class is used only as a base for the following two:

class MP3ADUinterleaverBase: public FramedFilter {
protected:
  MP3ADUinterleaverBase(UsageEnvironment& env,
			FramedSource* inputSource);
      // abstract base class
  virtual ~MP3ADUinterleaverBase();

  static FramedSource* getInputSource(UsageEnvironment& env,
				      char const* inputSourceName);
  static void afterGettingFrame(void* clientData,
				unsigned numBytesRead,
				unsigned numTruncatedBytes,
				struct timeval presentationTime,
				unsigned durationInMicroseconds);
  virtual void afterGettingFrame(unsigned numBytesRead,
				 struct timeval presentationTime,
				 unsigned durationInMicroseconds) = 0;
};

// This class is used to convert an ADU sequence from non-interleaved
// to interleaved form:

class MP3ADUinterleaver: public MP3ADUinterleaverBase {
public:
  static MP3ADUinterleaver* createNew(UsageEnvironment& env,
				      Interleaving const& interleaving,
				      FramedSource* inputSource);

protected:
  MP3ADUinterleaver(UsageEnvironment& env,
		    Interleaving const& interleaving,
		    FramedSource* inputSource);
      // called only by createNew()
  virtual ~MP3ADUinterleaver();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void afterGettingFrame(unsigned numBytesRead,
				 struct timeval presentationTime,
				 unsigned durationInMicroseconds);

private:
  void releaseOutgoingFrame();

private:
  Interleaving const fInterleaving;
  class InterleavingFrames* fFrames;
  unsigned char fPositionOfNextIncomingFrame;
  unsigned fII, fICC;
};

// This class is used to convert an ADU sequence from interleaved
// to non-interleaved form:

class MP3ADUdeinterleaver: public MP3ADUinterleaverBase {
public:
  static MP3ADUdeinterleaver* createNew(UsageEnvironment& env,
					FramedSource* inputSource);

protected:
  MP3ADUdeinterleaver(UsageEnvironment& env,
		      FramedSource* inputSource);
      // called only by createNew()
  virtual ~MP3ADUdeinterleaver();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void afterGettingFrame(unsigned numBytesRead,
				 struct timeval presentationTime,
				 unsigned durationInMicroseconds);

private:
  void releaseOutgoingFrame();

private:
  class DeinterleavingFrames* fFrames;
  unsigned fIIlastSeen, fICClastSeen;
};

#endif

