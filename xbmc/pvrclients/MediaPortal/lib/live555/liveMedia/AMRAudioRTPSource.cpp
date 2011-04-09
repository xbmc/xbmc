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
// AMR Audio RTP Sources (RFC 3267)
// Implementation

#include "AMRAudioRTPSource.hh"
#include "MultiFramedRTPSource.hh"
#include "BitVector.hh"
#include <string.h>
#include <stdlib.h>

// This source is implemented internally by two separate sources:
// (i) a RTP source for the raw (and possibly interleaved) AMR frames, and
// (ii) a deinterleaving filter that reads from this.
// Define these two new classes here:

class RawAMRRTPSource: public MultiFramedRTPSource {
public:
  static RawAMRRTPSource*
  createNew(UsageEnvironment& env,
	    Groupsock* RTPgs, unsigned char rtpPayloadFormat,
	    Boolean isWideband, Boolean isOctetAligned,
	    Boolean isInterleaved, Boolean CRCsArePresent);

  Boolean isWideband() const { return fIsWideband; }
  unsigned char ILL() const { return fILL; }
  unsigned char ILP() const { return fILP; }
  unsigned TOCSize() const { return fTOCSize; } // total # of frames in the last pkt
  unsigned char* TOC() const { return fTOC; } // FT+Q value for each TOC entry
  unsigned& frameIndex() { return fFrameIndex; } // index of frame-block within pkt
  Boolean& isSynchronized() { return fIsSynchronized; }

private:
  RawAMRRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
		  unsigned char rtpPayloadFormat,
		  Boolean isWideband, Boolean isOctetAligned,
		  Boolean isInterleaved, Boolean CRCsArePresent);
      // called only by createNew()

  virtual ~RawAMRRTPSource();

private:
  // redefined virtual functions:
  virtual Boolean hasBeenSynchronizedUsingRTCP();

  virtual Boolean processSpecialHeader(BufferedPacket* packet,
                                       unsigned& resultSpecialHeaderSize);
  virtual char const* MIMEtype() const;

private:
  Boolean fIsWideband, fIsOctetAligned, fIsInterleaved, fCRCsArePresent;
  unsigned char fILL, fILP;
  unsigned fTOCSize;
  unsigned char* fTOC;
  unsigned fFrameIndex, fNumSuccessiveSyncedPackets;
  Boolean fIsSynchronized;
};

class AMRDeinterleaver: public AMRAudioSource {
public:
  static AMRDeinterleaver*
  createNew(UsageEnvironment& env,
	    Boolean isWideband, unsigned numChannels, unsigned maxInterleaveGroupSize,
	    RawAMRRTPSource* inputSource);

private:
  AMRDeinterleaver(UsageEnvironment& env,
		   Boolean isWideband, unsigned numChannels,
		   unsigned maxInterleaveGroupSize, RawAMRRTPSource* inputSource);
      // called only by "createNew()"

  virtual ~AMRDeinterleaver();

  static void afterGettingFrame(void* clientData, unsigned frameSize,
				unsigned numTruncatedBytes,
                                struct timeval presentationTime,
				unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned frameSize, struct timeval presentationTime);

private:
  // Redefined virtual functions:
  void doGetNextFrame();
  virtual void doStopGettingFrames();

private:
  RawAMRRTPSource* fInputSource;
  class AMRDeinterleavingBuffer* fDeinterleavingBuffer;
  Boolean fNeedAFrame;

};


////////// AMRAudioRTPSource implementation //////////

#define MAX_NUM_CHANNELS 20 // far larger than ever expected...
#define MAX_INTERLEAVING_GROUP_SIZE 1000 // far larger than ever expected...

AMRAudioSource*
AMRAudioRTPSource::createNew(UsageEnvironment& env,
			     Groupsock* RTPgs,
			     RTPSource*& resultRTPSource,
			     unsigned char rtpPayloadFormat,
			     Boolean isWideband,
			     unsigned numChannels,
			     Boolean isOctetAligned,
			     unsigned interleaving,
			     Boolean robustSortingOrder,
			     Boolean CRCsArePresent) {
  // Perform sanity checks on the input parameters:
  if (robustSortingOrder) {
    env << "AMRAudioRTPSource::createNew(): 'Robust sorting order' was specified, but we don't yet support this!\n";
    return NULL;
  } else if (numChannels > MAX_NUM_CHANNELS) {
    env << "AMRAudioRTPSource::createNew(): The \"number of channels\" parameter ("
	<< numChannels << ") is much too large!\n";
    return NULL;
  } else if (interleaving > MAX_INTERLEAVING_GROUP_SIZE) {
    env << "AMRAudioRTPSource::createNew(): The \"interleaving\" parameter ("
	<< interleaving << ") is much too large!\n";
    return NULL;
  }

  // 'Bandwidth-efficient mode' precludes some other options:
  if (!isOctetAligned) {
    if (interleaving > 0 || robustSortingOrder || CRCsArePresent) {
      env << "AMRAudioRTPSource::createNew(): 'Bandwidth-efficient mode' was specified, along with interleaving, 'robust sorting order', and/or CRCs, so we assume 'octet-aligned mode' instead.\n";
      isOctetAligned = True;
    }
  }

  Boolean isInterleaved;
  unsigned maxInterleaveGroupSize; // in frames (not frame-blocks)
  if (interleaving > 0) {
    isInterleaved = True;
    maxInterleaveGroupSize = interleaving*numChannels;
  } else {
    isInterleaved = False;
    maxInterleaveGroupSize = numChannels;
  }

  RawAMRRTPSource* rawRTPSource;
  resultRTPSource = rawRTPSource
    = RawAMRRTPSource::createNew(env, RTPgs, rtpPayloadFormat,
				 isWideband, isOctetAligned,
				 isInterleaved, CRCsArePresent);
  if (resultRTPSource == NULL) return NULL;

  AMRDeinterleaver* deinterleaver
    = AMRDeinterleaver::createNew(env, isWideband, numChannels,
				  maxInterleaveGroupSize, rawRTPSource);
  if (deinterleaver == NULL) {
    Medium::close(resultRTPSource);
    resultRTPSource = NULL;
  }

  return deinterleaver;
}


////////// AMRBufferedPacket and AMRBufferedPacketFactory //////////

// A subclass of BufferedPacket, used to separate out AMR frames.

class AMRBufferedPacket: public BufferedPacket {
public:
  AMRBufferedPacket(RawAMRRTPSource& ourSource);
  virtual ~AMRBufferedPacket();

private: // redefined virtual functions
  virtual unsigned nextEnclosedFrameSize(unsigned char*& framePtr,
					 unsigned dataSize);
private:
  RawAMRRTPSource& fOurSource;
};

class AMRBufferedPacketFactory: public BufferedPacketFactory {
private: // redefined virtual functions
  virtual BufferedPacket* createNewPacket(MultiFramedRTPSource* ourSource);
};


///////// RawAMRRTPSource implementation ////////

RawAMRRTPSource*
RawAMRRTPSource::createNew(UsageEnvironment& env, Groupsock* RTPgs,
			   unsigned char rtpPayloadFormat,
			   Boolean isWideband, Boolean isOctetAligned,
			   Boolean isInterleaved, Boolean CRCsArePresent) {
  return new RawAMRRTPSource(env, RTPgs, rtpPayloadFormat,
			     isWideband, isOctetAligned,
			     isInterleaved, CRCsArePresent);
}

RawAMRRTPSource
::RawAMRRTPSource(UsageEnvironment& env,
		  Groupsock* RTPgs, unsigned char rtpPayloadFormat,
		  Boolean isWideband, Boolean isOctetAligned,
		  Boolean isInterleaved, Boolean CRCsArePresent)
  : MultiFramedRTPSource(env, RTPgs, rtpPayloadFormat,
			 isWideband ? 16000 : 8000,
                         new AMRBufferedPacketFactory),
  fIsWideband(isWideband), fIsOctetAligned(isOctetAligned),
  fIsInterleaved(isInterleaved), fCRCsArePresent(CRCsArePresent),
  fILL(0), fILP(0), fTOCSize(0), fTOC(NULL), fFrameIndex(0),
    fNumSuccessiveSyncedPackets(0), fIsSynchronized(false) {
}

RawAMRRTPSource::~RawAMRRTPSource() {
  delete[] fTOC;
}

#define FT_SPEECH_LOST 14
#define FT_NO_DATA 15

static void unpackBandwidthEfficientData(BufferedPacket* packet,
					 Boolean isWideband); // forward

Boolean RawAMRRTPSource
::processSpecialHeader(BufferedPacket* packet,
		       unsigned& resultSpecialHeaderSize) {
  // If the data is 'bandwidth-efficient', first unpack it so that it's
  // 'octet-aligned':
  if (!fIsOctetAligned) unpackBandwidthEfficientData(packet, fIsWideband);

  unsigned char* headerStart = packet->data();
  unsigned packetSize = packet->dataSize();

  // First, check whether this packet's RTP timestamp is synchronized:
  if (RTPSource::hasBeenSynchronizedUsingRTCP()) {
    ++fNumSuccessiveSyncedPackets;
  } else {
    fNumSuccessiveSyncedPackets = 0;
  }

  // There's at least a 1-byte header, containing the CMR:
  if (packetSize < 1) return False;
  resultSpecialHeaderSize = 1;

  if (fIsInterleaved) {
    // There's an extra byte, containing the interleave parameters:
    if (packetSize < 2) return False;

    // Get the interleaving parameters, and check them for validity:
    unsigned char const secondByte = headerStart[1];
    fILL = (secondByte&0xF0)>>4;
    fILP = secondByte&0x0F;
    if (fILP > fILL) return False; // invalid
    ++resultSpecialHeaderSize;
  }
#ifdef DEBUG
  fprintf(stderr, "packetSize: %d, ILL: %d, ILP: %d\n", packetSize, fILL, fILP);
#endif
  fFrameIndex = 0; // initially

  // Next, there's a "Payload Table of Contents" (one byte per entry):
  unsigned numFramesPresent = 0, numNonEmptyFramesPresent = 0;
  unsigned tocStartIndex = resultSpecialHeaderSize;
  Boolean F;
  do {
    if (resultSpecialHeaderSize >= packetSize) return False;
    unsigned char const tocByte = headerStart[resultSpecialHeaderSize++];
    F = (tocByte&0x80) != 0;
    unsigned char const FT = (tocByte&0x78) >> 3;
#ifdef DEBUG
    unsigned char Q = (tocByte&0x04)>>2;
    fprintf(stderr, "\tTOC entry: F %d, FT %d, Q %d\n", F, FT, Q);
#endif
    ++numFramesPresent;
    if (FT != FT_SPEECH_LOST && FT != FT_NO_DATA) ++numNonEmptyFramesPresent;
  } while (F);
#ifdef DEBUG
  fprintf(stderr, "TOC contains %d entries (%d non-empty)\n", numFramesPresent, numNonEmptyFramesPresent);
#endif

  // Now that we know the size of the TOC, fill in our copy:
  if (numFramesPresent > fTOCSize) {
    delete[] fTOC;
    fTOC = new unsigned char[numFramesPresent];
  }
  fTOCSize = numFramesPresent;
  for (unsigned i = 0; i < fTOCSize; ++i) {
    unsigned char const tocByte = headerStart[tocStartIndex + i];
    fTOC[i] = tocByte&0x7C; // clear everything except the F and Q fields
  }

  if (fCRCsArePresent) {
    // 'numNonEmptyFramesPresent' CRC bytes will follow.
    // Note: we currently don't check the CRCs for validity #####
    resultSpecialHeaderSize += numNonEmptyFramesPresent;
#ifdef DEBUG
    fprintf(stderr, "Ignoring %d following CRC bytes\n", numNonEmptyFramesPresent);
#endif
    if (resultSpecialHeaderSize > packetSize) return False;
  }
#ifdef DEBUG
  fprintf(stderr, "Total special header size: %d\n", resultSpecialHeaderSize);
#endif

  return True;
}

char const* RawAMRRTPSource::MIMEtype() const {
  return fIsWideband ? "audio/AMR-WB" : "audio/AMR";
}

Boolean RawAMRRTPSource::hasBeenSynchronizedUsingRTCP() {
  return fIsSynchronized;
}


///// AMRBufferedPacket and AMRBufferedPacketFactory implementation

AMRBufferedPacket::AMRBufferedPacket(RawAMRRTPSource& ourSource)
  : fOurSource(ourSource) {
}

AMRBufferedPacket::~AMRBufferedPacket() {
}

// The mapping from the "FT" field to frame size.
// Values of 65535 are invalid.
#define FT_INVALID 65535
static unsigned short frameBytesFromFT[16] = {
  12, 13, 15, 17,
  19, 20, 26, 31,
  5, FT_INVALID, FT_INVALID, FT_INVALID,
  FT_INVALID, FT_INVALID, FT_INVALID, 0
};
static unsigned short frameBytesFromFTWideband[16] = {
  17, 23, 32, 36,
  40, 46, 50, 58,
  60, 5, FT_INVALID, FT_INVALID,
  FT_INVALID, FT_INVALID, 0, 0
};

unsigned AMRBufferedPacket::
  nextEnclosedFrameSize(unsigned char*& framePtr, unsigned dataSize) {
  if (dataSize == 0) return 0; // sanity check

  // The size of the AMR frame is determined by the corresponding 'FT' value
  // in the packet's Table of Contents.
  unsigned const tocIndex = fOurSource.frameIndex();
  if (tocIndex >= fOurSource.TOCSize()) return 0; // sanity check

  unsigned char const tocByte = fOurSource.TOC()[tocIndex];
  unsigned char const FT = (tocByte&0x78) >> 3;
  // ASSERT: FT < 16
  unsigned short frameSize
    = fOurSource.isWideband() ? frameBytesFromFTWideband[FT] : frameBytesFromFT[FT];
  if (frameSize == FT_INVALID) {
    // Strange TOC entry!
    fOurSource.envir() << "AMRBufferedPacket::nextEnclosedFrameSize(): invalid FT: " << FT << "\n";
    frameSize = 0; // This probably messes up the rest of this packet, but...
  }
#ifdef DEBUG
  fprintf(stderr, "AMRBufferedPacket::nextEnclosedFrameSize(): frame #: %d, FT: %d, isWideband: %d => frameSize: %d (dataSize: %d)\n", tocIndex, FT, fOurSource.isWideband(), frameSize, dataSize);
#endif
  ++fOurSource.frameIndex();

  if (dataSize < frameSize) return 0;
  return frameSize;
}

BufferedPacket* AMRBufferedPacketFactory
::createNewPacket(MultiFramedRTPSource* ourSource) {
  return new AMRBufferedPacket((RawAMRRTPSource&)(*ourSource));
}

///////// AMRDeinterleavingBuffer /////////
// (used to implement AMRDeinterleaver)

#define AMR_MAX_FRAME_SIZE 60

class AMRDeinterleavingBuffer {
public:
  AMRDeinterleavingBuffer(unsigned numChannels, unsigned maxInterleaveGroupSize);
  virtual ~AMRDeinterleavingBuffer();

  void deliverIncomingFrame(unsigned frameSize, RawAMRRTPSource* source,
			    struct timeval presentationTime);
  Boolean retrieveFrame(unsigned char* to, unsigned maxSize,
			unsigned& resultFrameSize, unsigned& resultNumTruncatedBytes,
			u_int8_t& resultFrameHeader,
			struct timeval& resultPresentationTime,
			Boolean& resultIsSynchronized);

  unsigned char* inputBuffer() { return fInputBuffer; }
  unsigned inputBufferSize() const { return AMR_MAX_FRAME_SIZE; }

private:
  unsigned char* createNewBuffer();

  class FrameDescriptor {
  public:
    FrameDescriptor();
    virtual ~FrameDescriptor();

    unsigned frameSize;
    unsigned char* frameData;
    u_int8_t frameHeader;
    struct timeval presentationTime;
    Boolean fIsSynchronized;
  };

  unsigned fNumChannels, fMaxInterleaveGroupSize;
  FrameDescriptor* fFrames[2];
  unsigned char fIncomingBankId; // toggles between 0 and 1
  unsigned char fIncomingBinMax; // in the incoming bank
  unsigned char fOutgoingBinMax; // in the outgoing bank
  unsigned char fNextOutgoingBin;
  Boolean fHaveSeenPackets;
  u_int16_t fLastPacketSeqNumForGroup;
  unsigned char* fInputBuffer;
  struct timeval fLastRetrievedPresentationTime;
};


////////// AMRDeinterleaver implementation /////////

AMRDeinterleaver* AMRDeinterleaver
::createNew(UsageEnvironment& env,
	    Boolean isWideband, unsigned numChannels, unsigned maxInterleaveGroupSize,
	    RawAMRRTPSource* inputSource) {
  return new AMRDeinterleaver(env, isWideband, numChannels, maxInterleaveGroupSize, inputSource);
}

AMRDeinterleaver::AMRDeinterleaver(UsageEnvironment& env,
				   Boolean isWideband, unsigned numChannels,
				   unsigned maxInterleaveGroupSize,
				   RawAMRRTPSource* inputSource)
  : AMRAudioSource(env, isWideband, numChannels),
    fInputSource(inputSource), fNeedAFrame(False) {
  fDeinterleavingBuffer
    = new AMRDeinterleavingBuffer(numChannels, maxInterleaveGroupSize);
}

AMRDeinterleaver::~AMRDeinterleaver() {
  delete fDeinterleavingBuffer;
  Medium::close(fInputSource);
}

static unsigned const uSecsPerFrame = 20000; // 20 ms

void AMRDeinterleaver::doGetNextFrame() {
  // First, try getting a frame from the deinterleaving buffer:
  if (fDeinterleavingBuffer->retrieveFrame(fTo, fMaxSize,
					   fFrameSize, fNumTruncatedBytes,
					   fLastFrameHeader, fPresentationTime,
					   fInputSource->isSynchronized())) {

    // Success!
    fNeedAFrame = False;

    fDurationInMicroseconds = uSecsPerFrame;

    // Call our own 'after getting' function.  Because we're not a 'leaf'
    // source, we can call this directly, without risking
    // infinite recursion
    afterGetting(this);
    return;
  }

  // No luck, so ask our source for help:
  fNeedAFrame = True;
  if (!fInputSource->isCurrentlyAwaitingData()) {
    fInputSource->getNextFrame(fDeinterleavingBuffer->inputBuffer(),
			       fDeinterleavingBuffer->inputBufferSize(),
			       afterGettingFrame, this,
			       FramedSource::handleClosure, this);
  }
}

void AMRDeinterleaver::doStopGettingFrames() {
  fInputSource->stopGettingFrames();
}

void AMRDeinterleaver
::afterGettingFrame(void* clientData, unsigned frameSize,
		    unsigned /*numTruncatedBytes*/,
		    struct timeval presentationTime,
		    unsigned /*durationInMicroseconds*/) {
  AMRDeinterleaver* deinterleaver = (AMRDeinterleaver*)clientData;
  deinterleaver->afterGettingFrame1(frameSize, presentationTime);
}

void AMRDeinterleaver
::afterGettingFrame1(unsigned frameSize, struct timeval presentationTime) {
  RawAMRRTPSource* source = (RawAMRRTPSource*)fInputSource;

  // First, put the frame into our deinterleaving buffer:
  fDeinterleavingBuffer->deliverIncomingFrame(frameSize, source, presentationTime);

  // Then, try delivering a frame to the client (if he wants one):
  if (fNeedAFrame) doGetNextFrame();
}


////////// AMRDeinterleavingBuffer implementation /////////

AMRDeinterleavingBuffer
::AMRDeinterleavingBuffer(unsigned numChannels, unsigned maxInterleaveGroupSize)
  : fNumChannels(numChannels), fMaxInterleaveGroupSize(maxInterleaveGroupSize),
    fIncomingBankId(0), fIncomingBinMax(0),
    fOutgoingBinMax(0), fNextOutgoingBin(0),
    fHaveSeenPackets(False) {
  // Use two banks of descriptors - one for incoming, one for outgoing
  fFrames[0] = new FrameDescriptor[fMaxInterleaveGroupSize];
  fFrames[1] = new FrameDescriptor[fMaxInterleaveGroupSize];
  fInputBuffer = createNewBuffer();
}

AMRDeinterleavingBuffer::~AMRDeinterleavingBuffer() {
  delete[] fInputBuffer;
  delete[] fFrames[0]; delete[] fFrames[1];
}

void AMRDeinterleavingBuffer
::deliverIncomingFrame(unsigned frameSize, RawAMRRTPSource* source,
		       struct timeval presentationTime) {
  unsigned char const ILL = source->ILL();
  unsigned char const ILP = source->ILP();
  unsigned frameIndex = source->frameIndex();
  unsigned short packetSeqNum = source->curPacketRTPSeqNum();

  // First perform a sanity check on the parameters:
  // (This is overkill, as the source should have already done this.)
  if (ILP > ILL || frameIndex == 0) {
#ifdef DEBUG
    fprintf(stderr, "AMRDeinterleavingBuffer::deliverIncomingFrame() param sanity check failed (%d,%d,%d,%d)\n", frameSize, ILL, ILP, frameIndex);
#endif
    abort();
  }

  --frameIndex; // because it was incremented by the source when this frame was read
  u_int8_t frameHeader;
  if (frameIndex >= source->TOCSize()) { // sanity check
    frameHeader = FT_NO_DATA<<3;
  } else {
    frameHeader = source->TOC()[frameIndex];
  }

  unsigned frameBlockIndex = frameIndex/fNumChannels;
  unsigned frameWithinFrameBlock = frameIndex%fNumChannels;

  // The input "presentationTime" was that of the first frame-block in this
  // packet.  Update it for the current frame:
  unsigned uSecIncrement = frameBlockIndex*(ILL+1)*uSecsPerFrame;
  presentationTime.tv_usec += uSecIncrement;
  presentationTime.tv_sec += presentationTime.tv_usec/1000000;
  presentationTime.tv_usec = presentationTime.tv_usec%1000000;

  // Next, check whether this packet is part of a new interleave group
  if (!fHaveSeenPackets
      || seqNumLT(fLastPacketSeqNumForGroup, packetSeqNum + frameBlockIndex)) {
    // We've moved to a new interleave group
#ifdef DEBUG
    fprintf(stderr, "AMRDeinterleavingBuffer::deliverIncomingFrame(): new interleave group\n");
#endif
    fHaveSeenPackets = True;
    fLastPacketSeqNumForGroup = packetSeqNum + ILL - ILP;

    // Switch the incoming and outgoing banks:
    fIncomingBankId ^= 1;
    unsigned char tmp = fIncomingBinMax;
    fIncomingBinMax = fOutgoingBinMax;
    fOutgoingBinMax = tmp;
    fNextOutgoingBin = 0;
  }

  // Now move the incoming frame into the appropriate bin:
  unsigned const binNumber
    = ((ILP + frameBlockIndex*(ILL+1))*fNumChannels + frameWithinFrameBlock)
      % fMaxInterleaveGroupSize; // the % is for sanity
#ifdef DEBUG
  fprintf(stderr, "AMRDeinterleavingBuffer::deliverIncomingFrame(): frameIndex %d (%d,%d) put in bank %d, bin %d (%d): size %d, header 0x%02x, presentationTime %lu.%06ld\n", frameIndex, frameBlockIndex, frameWithinFrameBlock, fIncomingBankId, binNumber, fMaxInterleaveGroupSize, frameSize, frameHeader, presentationTime.tv_sec, presentationTime.tv_usec);
#endif
  FrameDescriptor& inBin = fFrames[fIncomingBankId][binNumber];
  unsigned char* curBuffer = inBin.frameData;
  inBin.frameData = fInputBuffer;
  inBin.frameSize = frameSize;
  inBin.frameHeader = frameHeader;
  inBin.presentationTime = presentationTime;
  inBin.fIsSynchronized = ((RTPSource*)source)->hasBeenSynchronizedUsingRTCP();

  if (curBuffer == NULL) curBuffer = createNewBuffer();
  fInputBuffer = curBuffer;

  if (binNumber >= fIncomingBinMax) {
    fIncomingBinMax = binNumber + 1;
  }
}

Boolean AMRDeinterleavingBuffer
::retrieveFrame(unsigned char* to, unsigned maxSize,
		unsigned& resultFrameSize, unsigned& resultNumTruncatedBytes,
		u_int8_t& resultFrameHeader,
		struct timeval& resultPresentationTime,
		Boolean& resultIsSynchronized) {

  if (fNextOutgoingBin >= fOutgoingBinMax) return False; // none left

  FrameDescriptor& outBin = fFrames[fIncomingBankId^1][fNextOutgoingBin];
  unsigned char* fromPtr = outBin.frameData;
  unsigned char fromSize = outBin.frameSize;
  outBin.frameSize = 0; // for the next time this bin is used
  resultIsSynchronized = outBin.fIsSynchronized;

  // Check whether this frame is missing; if so, return a FT_NO_DATA frame:
  if (fromSize == 0) {
    resultFrameHeader = FT_NO_DATA<<3;

    // Compute this erasure frame's presentation time via extrapolation:
    resultPresentationTime = fLastRetrievedPresentationTime;
    resultPresentationTime.tv_usec += uSecsPerFrame;
    if (resultPresentationTime.tv_usec >= 1000000) {
      ++resultPresentationTime.tv_sec;
      resultPresentationTime.tv_usec -= 1000000;
    }
  } else {
    // Normal case - a frame exists:
    resultFrameHeader = outBin.frameHeader;
    resultPresentationTime = outBin.presentationTime;
  }

  fLastRetrievedPresentationTime = resultPresentationTime;

  if (fromSize > maxSize) {
    resultNumTruncatedBytes = fromSize - maxSize;
    resultFrameSize = maxSize;
  } else {
    resultNumTruncatedBytes = 0;
    resultFrameSize = fromSize;
  }
  memmove(to, fromPtr, resultFrameSize);
#ifdef DEBUG
  fprintf(stderr, "AMRDeinterleavingBuffer::retrieveFrame(): from bank %d, bin %d: size %d, header 0x%02x, presentationTime %lu.%06ld\n", fIncomingBankId^1, fNextOutgoingBin, resultFrameSize, resultFrameHeader, resultPresentationTime.tv_sec, resultPresentationTime.tv_usec);
#endif

  ++fNextOutgoingBin;
  return True;
}

unsigned char* AMRDeinterleavingBuffer::createNewBuffer() {
  return new unsigned char[inputBufferSize()];
}

AMRDeinterleavingBuffer::FrameDescriptor::FrameDescriptor()
  : frameSize(0), frameData(NULL) {
}

AMRDeinterleavingBuffer::FrameDescriptor::~FrameDescriptor() {
  delete[] frameData;
}

// Unpack bandwidth-aligned data to octet-aligned:
static unsigned short frameBitsFromFT[16] = {
  95, 103, 118, 134,
  148, 159, 204, 244,
  39, 0, 0, 0,
  0, 0, 0, 0
};
static unsigned short frameBitsFromFTWideband[16] = {
  132, 177, 253, 285,
  317, 365, 397, 461,
  477, 40, 0, 0,
  0, 0, 0, 0
};

static void unpackBandwidthEfficientData(BufferedPacket* packet,
					 Boolean isWideband) {
#ifdef DEBUG
  fprintf(stderr, "Unpacking 'bandwidth-efficient' payload (%d bytes):\n", packet->dataSize());
  for (unsigned j = 0; j < packet->dataSize(); ++j) {
    fprintf(stderr, "%02x:", (packet->data())[j]);
  }
  fprintf(stderr, "\n");
#endif
  BitVector fromBV(packet->data(), 0, 8*packet->dataSize());

  unsigned const toBufferSize = 2*packet->dataSize(); // conservatively large
  unsigned char* toBuffer = new unsigned char[toBufferSize];
  unsigned toCount = 0;

  // Begin with the payload header:
  unsigned CMR = fromBV.getBits(4);
  toBuffer[toCount++] = CMR << 4;

  // Then, run through and unpack the TOC entries:
  while (1) {
    unsigned toc = fromBV.getBits(6);
    toBuffer[toCount++] = toc << 2;

    if ((toc&0x20) == 0) break; // the F bit is 0
  }

  // Then, using the TOC data, unpack each frame payload:
  unsigned const tocSize = toCount - 1;
  for (unsigned i = 1; i <= tocSize; ++i) {
    unsigned char tocByte = toBuffer[i];
    unsigned char const FT = (tocByte&0x78) >> 3;
    unsigned short frameSizeBits
      = isWideband ? frameBitsFromFTWideband[FT] : frameBitsFromFT[FT];
    unsigned short frameSizeBytes = (frameSizeBits+7)/8;

    shiftBits(&toBuffer[toCount], 0, // to
	      packet->data(), fromBV.curBitIndex(), // from
	      frameSizeBits // num bits
	      );
#ifdef DEBUG
    if (frameSizeBits > fromBV.numBitsRemaining()) {
      fprintf(stderr, "\tWarning: Unpacking frame %d of %d: want %d bits, but only %d are available!\n", i, tocSize, frameSizeBits, fromBV.numBitsRemaining());
    }
#endif
    fromBV.skipBits(frameSizeBits);
    toCount += frameSizeBytes;
  }

#ifdef DEBUG
  if (fromBV.numBitsRemaining() > 7) {
    fprintf(stderr, "\tWarning: %d bits remain unused!\n", fromBV.numBitsRemaining());
  }
#endif

  // Finally, replace the current packet data with the unpacked data:
  packet->removePadding(packet->dataSize()); // throws away current packet data
  packet->appendData(toBuffer, toCount);
  delete[] toBuffer;
}
