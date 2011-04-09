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
// A filter that parses a DV input stream into DV frames to deliver to the downstream object
// Implementation
// (Thanks to Ben Hutchings for his help, including a prototype implementation.)

#include "DVVideoStreamFramer.hh"
#include "GroupsockHelper.hh"

////////// DVVideoStreamFramer implementation //////////

DVVideoStreamFramer::DVVideoStreamFramer(UsageEnvironment& env, FramedSource* inputSource, Boolean sourceIsSeekable)
  : FramedFilter(env, inputSource),
    fOurProfile(NULL), fInitialBlocksPresent(False), fSourceIsSeekable(sourceIsSeekable) {
  fTo = NULL; // hack used when reading "fSavedInitialBlocks"
  // Use the current wallclock time as the initial 'presentation time':
  gettimeofday(&fNextFramePresentationTime, NULL);
}

DVVideoStreamFramer::~DVVideoStreamFramer() {
}

DVVideoStreamFramer*
DVVideoStreamFramer::createNew(UsageEnvironment& env, FramedSource* inputSource, Boolean sourceIsSeekable) {
  return new DVVideoStreamFramer(env, inputSource, sourceIsSeekable);
}

// Define the parameters for the profiles that we understand:
struct DVVideoProfile {
  char const* name;
  unsigned apt;
  unsigned sType;
  unsigned sequenceCount;
  unsigned channelCount;
  unsigned dvFrameSize; // in bytes (== sequenceCount*channelCount*(DV_NUM_BLOCKS_PER_SEQUENCE*DV_DIF_BLOCK_SIZE i.e. 12000))
  double frameDuration; // duration of the above, in microseconds.  (1000000/this == frame rate)
};

static DVVideoProfile const profiles[] = {
   { "SD-VCR/525-60",  0, 0x00, 10, 1, 120000, (1000000*1001)/30000.0 },
   { "SD-VCR/625-50",  0, 0x00, 12, 1, 144000, 1000000/25.0 },
   { "314M-25/525-60", 1, 0x00, 10, 1, 120000, (1000000*1001)/30000.0 },
   { "314M-25/625-50", 1, 0x00, 12, 1, 144000, 1000000/25.0 },
   { "314M-50/525-60", 1, 0x04, 10, 2, 240000, (1000000*1001)/30000.0 },
   { "314M-50/625-50", 1, 0x04, 12, 2, 288000, 1000000/25.0 },
   { "370M/1080-60i",  1, 0x14, 10, 4, 480000, (1000000*1001)/30000.0 },
   { "370M/1080-50i",  1, 0x14, 12, 4, 576000, 1000000/25.0 },
   { "370M/720-60p",   1, 0x18, 10, 2, 240000, (1000000*1001)/60000.0 },
   { "370M/720-50p",   1, 0x18, 12, 2, 288000, 1000000/50.0 },
   { NULL, 0, 0, 0, 0, 0, 0.0 }
  };


char const* DVVideoStreamFramer::profileName() {
  if (fOurProfile == NULL) getProfile();

  return fOurProfile != NULL ? ((DVVideoProfile const*)fOurProfile)->name : NULL;
}

Boolean DVVideoStreamFramer::getFrameParameters(unsigned& frameSize, double& frameDuration) {
  if (fOurProfile == NULL) getProfile();
  if (fOurProfile == NULL) return False;

  frameSize = ((DVVideoProfile const*)fOurProfile)->dvFrameSize;
  frameDuration = ((DVVideoProfile const*)fOurProfile)->frameDuration;
  return True;
}

void DVVideoStreamFramer::getProfile() {
  // To determine the stream's profile, we need to first read a chunk of data that we can parse:
  fInputSource->getNextFrame(fSavedInitialBlocks, DV_SAVED_INITIAL_BLOCKS_SIZE,
			     afterGettingFrame, this, FramedSource::handleClosure, this);
  
  // Handle events until the requested data arrives:
  envir().taskScheduler().doEventLoop(&fInitialBlocksPresent);
}

Boolean DVVideoStreamFramer::isDVVideoStreamFramer() const {
  return True;
}

void DVVideoStreamFramer::doGetNextFrame() {
  fFrameSize = 0; // initially, until we deliver data

  // If we have saved initial blocks (and won't be seeking back to re-read this data), so use this data first.
  if (fInitialBlocksPresent && !fSourceIsSeekable) {
    // For simplicity, we require the downstream object's buffer to be >= this data's size:
    if (fMaxSize < DV_SAVED_INITIAL_BLOCKS_SIZE) {
      fNumTruncatedBytes = fMaxSize;
      afterGetting(this);
      return;
    }

    memmove(fTo, fSavedInitialBlocks, DV_SAVED_INITIAL_BLOCKS_SIZE);
    fFrameSize = DV_SAVED_INITIAL_BLOCKS_SIZE;
    fTo += DV_SAVED_INITIAL_BLOCKS_SIZE;
    fInitialBlocksPresent = False; // for the future
  }
    
  // Arrange to read the (rest of the) requested data.
  // (But first, make sure that we read an integral multiple of the DV block size.)
  fMaxSize -= fMaxSize%DV_DIF_BLOCK_SIZE;
  getAndDeliverData();
}

#define DV_SMALLEST_POSSIBLE_FRAME_SIZE 120000

void DVVideoStreamFramer::getAndDeliverData() {
  unsigned const totFrameSize
    = fOurProfile != NULL ? ((DVVideoProfile const*)fOurProfile)->dvFrameSize : DV_SMALLEST_POSSIBLE_FRAME_SIZE;
  unsigned totBytesToDeliver = totFrameSize < fMaxSize ? totFrameSize : fMaxSize;
  unsigned numBytesToRead = totBytesToDeliver - fFrameSize;

  fInputSource->getNextFrame(fTo, numBytesToRead, afterGettingFrame, this, FramedSource::handleClosure, this);
}

void DVVideoStreamFramer::afterGettingFrame(void* clientData, unsigned frameSize,
					    unsigned numTruncatedBytes,
					    struct timeval /*presentationTime*/, unsigned /*durationInMicroseconds*/) {
  DVVideoStreamFramer* source = (DVVideoStreamFramer*)clientData;
  source->afterGettingFrame1(frameSize, numTruncatedBytes);
}

#define DVSectionId(n) ptr[(n)*DV_DIF_BLOCK_SIZE + 0]
#define DVData(n,i) ptr[(n)*DV_DIF_BLOCK_SIZE + 3+(i)]

#define DV_SECTION_HEADER 0x1F
#define DV_PACK_HEADER_10 0x3F
#define DV_PACK_HEADER_12 0xBF
#define DV_SECTION_VAUX_MIN 0x50
#define DV_SECTION_VAUX_MAX 0x5F
#define DV_PACK_VIDEO_SOURCE 60
#ifndef MILLION
#define MILLION 1000000
#endif

void DVVideoStreamFramer::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes) {
  if (fOurProfile == NULL && frameSize >= DV_SAVED_INITIAL_BLOCKS_SIZE) {
    // (Try to) parse this data enough to figure out its profile.
    // We assume that the data begins on a (80-byte) block boundary, but not necessarily on a (150-block) sequence boundary.
    // We therefore scan each 80-byte block, until we find the 6-block header that begins a sequence:
    u_int8_t const* data = (fTo == NULL) ? fSavedInitialBlocks : fTo;
    for (u_int8_t const* ptr = data; ptr + 6*DV_DIF_BLOCK_SIZE <= &data[DV_SAVED_INITIAL_BLOCKS_SIZE]; ptr += DV_DIF_BLOCK_SIZE) {
      // Check whether "ptr" points to an appropriate header:
      u_int8_t const sectionHeader = DVSectionId(0);
      u_int8_t const sectionVAUX = DVSectionId(5);
      u_int8_t const packHeaderNum = DVData(0,0);

      if (sectionHeader == DV_SECTION_HEADER
	  && (packHeaderNum == DV_PACK_HEADER_10 || packHeaderNum == DV_PACK_HEADER_12)
	  && (sectionVAUX >= DV_SECTION_VAUX_MIN && sectionVAUX <= DV_SECTION_VAUX_MAX)) {
	// This data begins a sequence; look up the DV profile from this:
	u_int8_t const apt = DVData(0,1)&0x07;
	u_int8_t const sType = DVData(5,48)&0x1F;
	u_int8_t const sequenceCount = (packHeaderNum == DV_PACK_HEADER_10) ? 10 : 12;

	// Use these three parameters (apt, sType, sequenceCount) to look up the DV profile:
	for (DVVideoProfile const* profile = profiles; profile->name != NULL; ++profile) {
	  if (profile->apt == apt && profile->sType == sType && profile->sequenceCount == sequenceCount) {
	    fOurProfile = profile;
	    break;
	  }
	}
	break; // because we found a correct sequence header (even if we don't happen to define a profile for it)
      }
    }
  }

  if (fTo != NULL) { // There is a downstream object; complete delivery to it (or read more data, if necessary)
    unsigned const totFrameSize
      = fOurProfile != NULL ? ((DVVideoProfile const*)fOurProfile)->dvFrameSize : DV_SMALLEST_POSSIBLE_FRAME_SIZE;
    fFrameSize += frameSize;
    fTo += frameSize;

    if (fFrameSize < totFrameSize && fFrameSize < fMaxSize && numTruncatedBytes == 0) {
      // We have more data to deliver; get it now:
      getAndDeliverData();
    } else {
      // We're done delivering this DV frame (but check for truncation):
      fNumTruncatedBytes = totFrameSize - fFrameSize;

      if (fOurProfile != NULL) {
	// Also set the presentation time, and increment it for next time,
	// based on the length of this frame:
	fPresentationTime = fNextFramePresentationTime;

	DVVideoProfile const* ourProfile =(DVVideoProfile const*)fOurProfile;
	double durationInMicroseconds = (fFrameSize*ourProfile->frameDuration)/ourProfile->dvFrameSize;
	fDurationInMicroseconds = (unsigned)durationInMicroseconds;
	fNextFramePresentationTime.tv_usec += fDurationInMicroseconds;
	fNextFramePresentationTime.tv_sec += fNextFramePresentationTime.tv_usec/MILLION;
	fNextFramePresentationTime.tv_usec %= MILLION;
      }

      afterGetting(this);
    }
  } else {
    // We read data into our special buffer; signal that it has arrived:
    fInitialBlocksPresent = True;
  }
}
