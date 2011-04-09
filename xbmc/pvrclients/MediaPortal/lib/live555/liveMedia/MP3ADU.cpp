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
// 'ADU' MP3 streams (for improved loss-tolerance)
// Implementation

#include "MP3ADU.hh"
#include "MP3ADUdescriptor.hh"
#include "MP3Internals.hh"
#include <string.h>

#ifdef TEST_LOSS
#include "GroupsockHelper.hh"
#endif

// Segment data structures, used in the implementation below:

#define SegmentBufSize 2000	/* conservatively high */

class Segment {
public:
  unsigned char buf[SegmentBufSize];
  unsigned char* dataStart() { return &buf[descriptorSize]; }
  unsigned frameSize; // if it's a non-ADU frame
  unsigned dataHere(); // if it's a non-ADU frame

  unsigned descriptorSize;
  static unsigned const headerSize;
  unsigned sideInfoSize, aduSize;
  unsigned backpointer;

  struct timeval presentationTime;
  unsigned durationInMicroseconds;
};

unsigned const Segment::headerSize = 4;

#define SegmentQueueSize 10

class SegmentQueue {
public:
  SegmentQueue(Boolean directionIsToADU, Boolean includeADUdescriptors)
    : fDirectionIsToADU(directionIsToADU),
      fIncludeADUdescriptors(includeADUdescriptors) {
    reset();
  }

  Segment s[SegmentQueueSize];

  unsigned headIndex() {return fHeadIndex;}
  Segment& headSegment() {return s[fHeadIndex];}

  unsigned nextFreeIndex() {return fNextFreeIndex;}
  Segment& nextFreeSegment() {return s[fNextFreeIndex];}
  Boolean isEmpty() {return isEmptyOrFull() && totalDataSize() == 0;}
  Boolean isFull() {return isEmptyOrFull() && totalDataSize() > 0;}

  static unsigned nextIndex(unsigned ix) {return (ix+1)%SegmentQueueSize;}
  static unsigned prevIndex(unsigned ix) {return (ix+SegmentQueueSize-1)%SegmentQueueSize;}

  unsigned totalDataSize() {return fTotalDataSize;}

  void enqueueNewSegment(FramedSource* inputSource, FramedSource* usingSource);

  Boolean dequeue();

  Boolean insertDummyBeforeTail(unsigned backpointer);

  void reset() { fHeadIndex = fNextFreeIndex = fTotalDataSize = 0; }

private:
  static void sqAfterGettingSegment(void* clientData,
				    unsigned numBytesRead,
				    unsigned numTruncatedBytes,
				    struct timeval presentationTime,
				    unsigned durationInMicroseconds);

  Boolean sqAfterGettingCommon(Segment& seg, unsigned numBytesRead);
  Boolean isEmptyOrFull() {return headIndex() == nextFreeIndex();}

  unsigned fHeadIndex, fNextFreeIndex, fTotalDataSize;

  // The following is used for asynchronous reads:
  FramedSource* fUsingSource;

  // This tells us whether the direction in which we're being used
  // is MP3->ADU, or vice-versa.  (This flag is used for debugging output.)
  Boolean fDirectionIsToADU;

  // The following is true iff we're used to enqueue incoming
  // ADU frames, and these have an ADU descriptor in front
  Boolean fIncludeADUdescriptors;
};

////////// ADUFromMP3Source //////////

ADUFromMP3Source::ADUFromMP3Source(UsageEnvironment& env,
				   FramedSource* inputSource,
				   Boolean includeADUdescriptors)
  : FramedFilter(env, inputSource),
    fAreEnqueueingMP3Frame(False),
    fSegments(new SegmentQueue(True /* because we're MP3->ADU */,
			       False /*no descriptors in incoming frames*/)),
    fIncludeADUdescriptors(includeADUdescriptors),
    fTotalDataSizeBeforePreviousRead(0), fScale(1), fFrameCounter(0) {
}

ADUFromMP3Source::~ADUFromMP3Source() {
  delete fSegments;
}


char const* ADUFromMP3Source::MIMEtype() const {
  return "audio/MPA-ROBUST";
}

ADUFromMP3Source* ADUFromMP3Source::createNew(UsageEnvironment& env,
                                              FramedSource* inputSource,
                                              Boolean includeADUdescriptors) {
  // The source must be a MPEG audio source:
  if (strcmp(inputSource->MIMEtype(), "audio/MPEG") != 0) {
    env.setResultMsg(inputSource->name(), " is not an MPEG audio source");
    return NULL;
  }

  return new ADUFromMP3Source(env, inputSource, includeADUdescriptors);
}

void ADUFromMP3Source::resetInput() {
  fSegments->reset();
}

Boolean ADUFromMP3Source::setScaleFactor(int scale) {
  if (scale < 1) return False;
  fScale = scale;
  return True;
}

void ADUFromMP3Source::doGetNextFrame() {
  if (!fAreEnqueueingMP3Frame) {
    // Arrange to enqueue a new MP3 frame:
    fTotalDataSizeBeforePreviousRead = fSegments->totalDataSize();
    fAreEnqueueingMP3Frame = True;
    fSegments->enqueueNewSegment(fInputSource, this);
  } else {
    // Deliver an ADU from a previously-read MP3 frame:
    fAreEnqueueingMP3Frame = False;

    if (!doGetNextFrame1()) {
      // An internal error occurred; act as if our source went away:
      FramedSource::handleClosure(this);
    }
  }
}

Boolean ADUFromMP3Source::doGetNextFrame1() {
  // First, check whether we have enough previously-read data to output an
  // ADU for the last-read MP3 frame:
  unsigned tailIndex;
  Segment* tailSeg;
  Boolean needMoreData;

  if (fSegments->isEmpty()) {
    needMoreData = True;
    tailSeg = NULL; tailIndex = 0; // unneeded, but stops compiler warnings
  } else {
    tailIndex = SegmentQueue::prevIndex(fSegments->nextFreeIndex());
    tailSeg = &(fSegments->s[tailIndex]);

    needMoreData
	  = fTotalDataSizeBeforePreviousRead < tailSeg->backpointer // bp points back too far
      || tailSeg->backpointer + tailSeg->dataHere() < tailSeg->aduSize; // not enough data
  }

  if (needMoreData) {
    // We don't have enough data to output an ADU from the last-read MP3
    // frame, so need to read another one and try again:
    doGetNextFrame();
    return True;
  }

  // Output an ADU from the tail segment:
  fFrameSize = tailSeg->headerSize+tailSeg->sideInfoSize+tailSeg->aduSize;
  fPresentationTime = tailSeg->presentationTime;
  fDurationInMicroseconds = tailSeg->durationInMicroseconds;
  unsigned descriptorSize
    = fIncludeADUdescriptors ? ADUdescriptor::computeSize(fFrameSize) : 0;
#ifdef DEBUG
  fprintf(stderr, "m->a:outputting ADU %d<-%d, nbr:%d, sis:%d, dh:%d, (descriptor size: %d)\n", tailSeg->aduSize, tailSeg->backpointer, fFrameSize, tailSeg->sideInfoSize, tailSeg->dataHere(), descriptorSize);
#endif
  if (descriptorSize + fFrameSize > fMaxSize) {
    envir() << "ADUFromMP3Source::doGetNextFrame1(): not enough room ("
	    << descriptorSize + fFrameSize << ">"
	    << fMaxSize << ")\n";
    fFrameSize = 0;
    return False;
  }

  unsigned char* toPtr = fTo;
  // output the ADU descriptor:
  if (fIncludeADUdescriptors) {
    fFrameSize += ADUdescriptor::generateDescriptor(toPtr, fFrameSize);
  }

  // output header and side info:
  memmove(toPtr, tailSeg->dataStart(),
	  tailSeg->headerSize + tailSeg->sideInfoSize);
  toPtr += tailSeg->headerSize + tailSeg->sideInfoSize;

  // go back to the frame that contains the start of our data:
  unsigned offset = 0;
  unsigned i = tailIndex;
  unsigned prevBytes = tailSeg->backpointer;
  while (prevBytes > 0) {
    i = SegmentQueue::prevIndex(i);
    unsigned dataHere = fSegments->s[i].dataHere();
    if (dataHere < prevBytes) {
      prevBytes -= dataHere;
    } else {
      offset = dataHere - prevBytes;
      break;
    }
  }

  // dequeue any segments that we no longer need:
  while (fSegments->headIndex() != i) {
    fSegments->dequeue(); // we're done with it
  }

  unsigned bytesToUse = tailSeg->aduSize;
  while (bytesToUse > 0) {
    Segment& seg = fSegments->s[i];
    unsigned char* fromPtr
      = &seg.dataStart()[seg.headerSize + seg.sideInfoSize + offset];
    unsigned dataHere = seg.dataHere() - offset;
    unsigned bytesUsedHere = dataHere < bytesToUse ? dataHere : bytesToUse;
    memmove(toPtr, fromPtr, bytesUsedHere);
    bytesToUse -= bytesUsedHere;
    toPtr += bytesUsedHere;
    offset = 0;
    i = SegmentQueue::nextIndex(i);
  }


  if (fFrameCounter++%fScale == 0) {
    // Call our own 'after getting' function.  Because we're not a 'leaf'
    // source, we can call this directly, without risking infinite recursion.
    afterGetting(this);
  } else {
    // Don't use this frame; get another one:
    doGetNextFrame();
  }

  return True;
}


////////// MP3FromADUSource //////////

MP3FromADUSource::MP3FromADUSource(UsageEnvironment& env,
				   FramedSource* inputSource,
				   Boolean includeADUdescriptors)
  : FramedFilter(env, inputSource),
    fAreEnqueueingADU(False),
    fSegments(new SegmentQueue(False /* because we're ADU->MP3 */,
			       includeADUdescriptors)),
    fIncludeADUdescriptors(includeADUdescriptors) {
}

MP3FromADUSource::~MP3FromADUSource() {
  delete fSegments;
}

char const* MP3FromADUSource::MIMEtype() const {
  return "audio/MPEG";
}

MP3FromADUSource* MP3FromADUSource::createNew(UsageEnvironment& env,
					      FramedSource* inputSource,
					      Boolean includeADUdescriptors) {
  // The source must be an MP3 ADU source:
  if (strcmp(inputSource->MIMEtype(), "audio/MPA-ROBUST") != 0) {
    env.setResultMsg(inputSource->name(), " is not an MP3 ADU source");
    return NULL;
  }

  return new MP3FromADUSource(env, inputSource, includeADUdescriptors);
}


void MP3FromADUSource::doGetNextFrame() {
  if (fAreEnqueueingADU) insertDummyADUsIfNecessary();
  fAreEnqueueingADU = False;

  if (needToGetAnADU()) {
    // Before returning a frame, we must enqueue at least one ADU:
#ifdef TEST_LOSS
  NOTE: This code no longer works, because it uses synchronous reads,
  which are no longer supported.
    static unsigned const framesPerPacket = 10;
    static unsigned const frameCount = 0;
    static Boolean packetIsLost;
    while (1) {
      if ((frameCount++)%framesPerPacket == 0) {
	packetIsLost = (our_random()%10 == 0); // simulate 10% packet loss #####
      }

      if (packetIsLost) {
	// Read and discard the next input frame (that would be part of
	// a lost packet):
	Segment dummySegment;
	unsigned numBytesRead;
	struct timeval presentationTime;
	// (this works only if the source can be read synchronously)
	fInputSource->syncGetNextFrame(dummySegment.buf,
				       sizeof dummySegment.buf, numBytesRead,
				       presentationTime);
      } else {
	break; // from while (1)
      }
    }
#endif

    fAreEnqueueingADU = True;
    fSegments->enqueueNewSegment(fInputSource, this);
  } else {
    // Return a frame now:
    generateFrameFromHeadADU();
        // sets fFrameSize, fPresentationTime, and fDurationInMicroseconds

    // Call our own 'after getting' function.  Because we're not a 'leaf'
    // source, we can call this directly, without risking infinite recursion.
    afterGetting(this);
  }
}

Boolean MP3FromADUSource::needToGetAnADU() {
  // Check whether we need to first enqueue a new ADU before we
  // can generate a frame for our head ADU.
  Boolean needToEnqueue = True;

  if (!fSegments->isEmpty()) {
    unsigned index = fSegments->headIndex();
    Segment* seg = &(fSegments->headSegment());
    int const endOfHeadFrame = (int) seg->dataHere();
    unsigned frameOffset = 0;

    while (1) {
      int endOfData = frameOffset - seg->backpointer + seg->aduSize;
      if (endOfData >= endOfHeadFrame) {
	// We already have enough data to generate a frame
	needToEnqueue = False;
	break;
      }

      frameOffset += seg->dataHere();
      index = SegmentQueue::nextIndex(index);
      if (index == fSegments->nextFreeIndex()) break;
      seg = &(fSegments->s[index]);
    }
  }

  return needToEnqueue;
}

void MP3FromADUSource::insertDummyADUsIfNecessary() {
  if (fSegments->isEmpty()) return; // shouldn't happen

  // The tail segment (ADU) is assumed to have been recently
  // enqueued.  If its backpointer would overlap the data
  // of the previous ADU, then we need to insert one or more
  // empty, 'dummy' ADUs ahead of it.  (This situation should occur
  // only if an intermediate ADU was lost.)

  unsigned tailIndex
    = SegmentQueue::prevIndex(fSegments->nextFreeIndex());
  Segment* tailSeg = &(fSegments->s[tailIndex]);

  while (1) {
    unsigned prevADUend; // relative to the start of the new ADU
    if (fSegments->headIndex() != tailIndex) {
      // there is a previous segment
      unsigned prevIndex = SegmentQueue::prevIndex(tailIndex);
      Segment& prevSegment = fSegments->s[prevIndex];
      prevADUend = prevSegment.dataHere() + prevSegment.backpointer;
      if (prevSegment.aduSize > prevADUend) {
	// shouldn't happen if the previous ADU was well-formed
	prevADUend = 0;
      } else {
	prevADUend -= prevSegment.aduSize;
      }
    } else {
      prevADUend = 0;
    }

    if (tailSeg->backpointer > prevADUend) {
      // We need to insert a dummy ADU in front of the tail
#ifdef DEBUG
      fprintf(stderr, "a->m:need to insert a dummy ADU (%d, %d, %d) [%d, %d]\n", tailSeg->backpointer, prevADUend, tailSeg->dataHere(), fSegments->headIndex(), fSegments->nextFreeIndex());
#endif
      tailIndex = fSegments->nextFreeIndex();
      if (!fSegments->insertDummyBeforeTail(prevADUend)) return;
      tailSeg = &(fSegments->s[tailIndex]);
    } else {
      break; // no more dummy ADUs need to be inserted
    }
  }
}

Boolean MP3FromADUSource::generateFrameFromHeadADU() {
    // Output a frame for the head ADU:
    if (fSegments->isEmpty()) return False;
    unsigned index = fSegments->headIndex();
    Segment* seg = &(fSegments->headSegment());
#ifdef DEBUG
    fprintf(stderr, "a->m:outputting frame for %d<-%d (fs %d, dh %d), (descriptorSize: %d)\n", seg->aduSize, seg->backpointer, seg->frameSize, seg->dataHere(), seg->descriptorSize);
#endif
    unsigned char* toPtr = fTo;

    // output header and side info:
    fFrameSize = seg->frameSize;
    fPresentationTime = seg->presentationTime;
    fDurationInMicroseconds = seg->durationInMicroseconds;
    memmove(toPtr, seg->dataStart(), seg->headerSize + seg->sideInfoSize);
    toPtr += seg->headerSize + seg->sideInfoSize;

    // zero out the rest of the frame, in case ADU data doesn't fill it all in
    unsigned bytesToZero = seg->dataHere();
    for (unsigned i = 0; i < bytesToZero; ++i) {
      toPtr[i] = '\0';
    }

    // Fill in the frame with appropriate ADU data from this and
    // subsequent ADUs:
    unsigned frameOffset = 0;
    unsigned toOffset = 0;
    unsigned const endOfHeadFrame = seg->dataHere();

    while (toOffset < endOfHeadFrame) {
      int startOfData = frameOffset - seg->backpointer;
      if (startOfData > (int)endOfHeadFrame) break; // no more ADUs needed

      int endOfData = startOfData + seg->aduSize;
      if (endOfData > (int)endOfHeadFrame) {
	endOfData = endOfHeadFrame;
      }

      unsigned fromOffset;
      if (startOfData <= (int)toOffset) {
	fromOffset = toOffset - startOfData;
	startOfData = toOffset;
	if (endOfData < startOfData) endOfData = startOfData;
      } else {
	fromOffset = 0;

	// we may need some padding bytes beforehand
	unsigned bytesToZero = startOfData - toOffset;
#ifdef DEBUG
	if (bytesToZero > 0) fprintf(stderr, "a->m:outputting %d zero bytes (%d, %d, %d, %d)\n", bytesToZero, startOfData, toOffset, frameOffset, seg->backpointer);
#endif
	toOffset += bytesToZero;
      }

      unsigned char* fromPtr
	= &seg->dataStart()[seg->headerSize + seg->sideInfoSize + fromOffset];
      unsigned bytesUsedHere = endOfData - startOfData;
#ifdef DEBUG
      if (bytesUsedHere > 0) fprintf(stderr, "a->m:outputting %d bytes from %d<-%d\n", bytesUsedHere, seg->aduSize, seg->backpointer);
#endif
      memmove(toPtr + toOffset, fromPtr, bytesUsedHere);
      toOffset += bytesUsedHere;

      frameOffset += seg->dataHere();
      index = SegmentQueue::nextIndex(index);
      if (index == fSegments->nextFreeIndex()) break;
      seg = &(fSegments->s[index]);
    }

    fSegments->dequeue();

    return True;
}


////////// Segment //////////

unsigned Segment::dataHere() {
  int result = frameSize - (headerSize + sideInfoSize);
  if (result < 0) {
    return 0;
  }

  return (unsigned)result;
}

////////// SegmentQueue //////////

void SegmentQueue::enqueueNewSegment(FramedSource* inputSource,
				     FramedSource* usingSource) {
  if (isFull()) {
    usingSource->envir() << "SegmentQueue::enqueueNewSegment() overflow\n";
    FramedSource::handleClosure(usingSource);
    return;
  }

  fUsingSource = usingSource;

  Segment& seg = nextFreeSegment();
  inputSource->getNextFrame(seg.buf, sizeof seg.buf,
			    sqAfterGettingSegment, this,
			    FramedSource::handleClosure, usingSource);
}

void SegmentQueue::sqAfterGettingSegment(void* clientData,
					 unsigned numBytesRead,
					 unsigned /*numTruncatedBytes*/,
					 struct timeval presentationTime,
					 unsigned durationInMicroseconds) {
  SegmentQueue* segQueue = (SegmentQueue*)clientData;
  Segment& seg = segQueue->nextFreeSegment();

  seg.presentationTime = presentationTime;
  seg.durationInMicroseconds = durationInMicroseconds;

  if (segQueue->sqAfterGettingCommon(seg, numBytesRead)) {
#ifdef DEBUG
    char const* direction = segQueue->fDirectionIsToADU ? "m->a" : "a->m";
    fprintf(stderr, "%s:read frame %d<-%d, fs:%d, sis:%d, dh:%d, (descriptor size: %d)\n", direction, seg.aduSize, seg.backpointer, seg.frameSize, seg.sideInfoSize, seg.dataHere(), seg.descriptorSize);
#endif
  }

  // Continue our original calling source where it left off:
  segQueue->fUsingSource->doGetNextFrame();
}

// Common code called after a new segment is enqueued
Boolean SegmentQueue::sqAfterGettingCommon(Segment& seg,
					   unsigned numBytesRead) {
  unsigned char* fromPtr = seg.buf;

  if (fIncludeADUdescriptors) {
    // The newly-read data is assumed to be an ADU with a descriptor
    // in front
    (void)ADUdescriptor::getRemainingFrameSize(fromPtr);
    seg.descriptorSize = (unsigned)(fromPtr-seg.buf);
  } else {
    seg.descriptorSize = 0;
  }

  // parse the MP3-specific info in the frame to get the ADU params
  unsigned hdr;
  MP3SideInfo sideInfo;
  if (!GetADUInfoFromMP3Frame(fromPtr, numBytesRead,
			      hdr, seg.frameSize,
			      sideInfo, seg.sideInfoSize,
			      seg.backpointer, seg.aduSize)) {
    return False;
  }

  // If we've just read an ADU (rather than a regular MP3 frame), then use the
  // entire "numBytesRead" data for the 'aduSize', so that we include any
  // 'ancillary data' that may be present at the end of the ADU:
  if (!fDirectionIsToADU) {
    unsigned newADUSize
      = numBytesRead - seg.descriptorSize - 4/*header size*/ - seg.sideInfoSize;
    if (newADUSize > seg.aduSize) seg.aduSize = newADUSize;
  }
  fTotalDataSize += seg.dataHere();
  fNextFreeIndex = nextIndex(fNextFreeIndex);

  return True;
}

Boolean SegmentQueue::dequeue() {
  if (isEmpty()) {
    fUsingSource->envir() << "SegmentQueue::dequeue(): underflow!\n";
    return False;
  }

  Segment& seg = s[headIndex()];
  fTotalDataSize -= seg.dataHere();
  fHeadIndex = nextIndex(fHeadIndex);
  return True;
}

Boolean SegmentQueue::insertDummyBeforeTail(unsigned backpointer) {
  if (isEmptyOrFull()) return False;

  // Copy the current tail segment to its new position, then modify the
  // old tail segment to be a 'dummy' ADU

  unsigned newTailIndex = nextFreeIndex();
  Segment& newTailSeg = s[newTailIndex];

  unsigned oldTailIndex = prevIndex(newTailIndex);
  Segment& oldTailSeg = s[oldTailIndex];

  newTailSeg = oldTailSeg; // structure copy

  // Begin by setting (replacing) the ADU descriptor of the dummy ADU:
  unsigned char* ptr = oldTailSeg.buf;
  if (fIncludeADUdescriptors) {
    unsigned remainingFrameSize
      = oldTailSeg.headerSize + oldTailSeg.sideInfoSize + 0 /* 0-size ADU */;
    unsigned currentDescriptorSize = oldTailSeg.descriptorSize;

    if (currentDescriptorSize == 2) {
      ADUdescriptor::generateTwoByteDescriptor(ptr, remainingFrameSize);
    } else {
      (void)ADUdescriptor::generateDescriptor(ptr, remainingFrameSize);
    }
  }

  // Then zero out the side info of the dummy frame:
  if (!ZeroOutMP3SideInfo(ptr, oldTailSeg.frameSize,
			  backpointer)) return False;

  unsigned dummyNumBytesRead
    = oldTailSeg.descriptorSize + 4/*header size*/ + oldTailSeg.sideInfoSize;
  return sqAfterGettingCommon(oldTailSeg, dummyNumBytesRead);
}
