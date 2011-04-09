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
// A filter that converts a MPEG Transport Stream file - with corresponding index file
// - to a corresponding Video Elementary Stream.  It also uses a "scale" parameter
// to implement 'trick mode' (fast forward or reverse play, using I-frames) on
// the video stream.
// Implementation

#include "MPEG2TransportStreamTrickModeFilter.hh"
#include <ByteStreamFileSource.hh>

// Define the following to be True if we want the output file to have the same frame rate as the original file.
//    (Because the output file contains I-frames only, this means that each I-frame will appear in the output file
//     several times, and therefore the output file's bitrate will be significantly higher than that of the original.)
// Define the following to be False if we want the output file to include each I-frame no more than once.
//    (This means that - except for high 'scale' values - both the output frame rate and the output bit rate
//     will be less than that of the original.)
#define KEEP_ORIGINAL_FRAME_RATE False

MPEG2TransportStreamTrickModeFilter* MPEG2TransportStreamTrickModeFilter
::createNew(UsageEnvironment& env, FramedSource* inputSource,
	    MPEG2TransportStreamIndexFile* indexFile, int scale) {
  return new MPEG2TransportStreamTrickModeFilter(env, inputSource, indexFile, scale);
}

MPEG2TransportStreamTrickModeFilter
::MPEG2TransportStreamTrickModeFilter(UsageEnvironment& env, FramedSource* inputSource,
				      MPEG2TransportStreamIndexFile* indexFile, int scale)
  : FramedFilter(env, inputSource),
    fHaveStarted(False), fIndexFile(indexFile), fScale(scale), fDirection(1),
    fState(SKIPPING_FRAME), fFrameCount(0),
    fNextIndexRecordNum(0), fNextTSPacketNum(0),
    fCurrentTSPacketNum((unsigned long)(-1)), fUseSavedFrameNextTime(False) {
  if (fScale < 0) { // reverse play
    fScale = -fScale;
    fDirection = -1;
  }
}

MPEG2TransportStreamTrickModeFilter::~MPEG2TransportStreamTrickModeFilter() {
}

Boolean MPEG2TransportStreamTrickModeFilter::seekTo(unsigned long tsPacketNumber,
						    unsigned long indexRecordNumber) {
  seekToTransportPacket(tsPacketNumber);
  fNextIndexRecordNum = indexRecordNumber;
  return True;
}

#define isIFrameStart(type) ((type) == 0x81) // actually, a Video Sequence Header
// This relies upon the fact that I-frames are always preceded by VSH and GOP
#define isNonIFrameStart(type) ((type) == 0x83)

void MPEG2TransportStreamTrickModeFilter::doGetNextFrame() {
  //  fprintf(stderr, "#####DGNF1\n");
  // If our client's buffer size is too small, then deliver
  // a 0-byte 'frame', to tell it to process all of the data that it has
  // already read, before asking for more data from us:
  if (fMaxSize < TRANSPORT_PACKET_SIZE) {
    fFrameSize = 0;
    afterGetting(this);
    return;
  }

  while (1) {
    // Get the next record from our index file.
    // This tells us the type of frame this data is, which Transport Stream packet
    // (from the input source) the data comes from, and where in the Transport Stream
    // packet it comes from:
    u_int8_t recordType;
    float recordPCR;
    Boolean endOfIndexFile = False;
    if (!fIndexFile->readIndexRecordValues(fNextIndexRecordNum,
					   fDesiredTSPacketNum, fDesiredDataOffset,
					   fDesiredDataSize, recordPCR,
					   recordType)) {
      // We ran off the end of the index file.  If we're not delivering a
      // pre-saved frame, then handle this the same way as if the
      // input Transport Stream source ended.
      if (fState != DELIVERING_SAVED_FRAME) {
	onSourceClosure1();
	return;
      }
      endOfIndexFile = True;
    } else if (!fHaveStarted) {
      fFirstPCR = recordPCR;
      fHaveStarted = True;
    }
    //    fprintf(stderr, "#####read index record %ld: ts %ld: %c, PCR %f\n", fNextIndexRecordNum, fDesiredTSPacketNum, isIFrameStart(recordType) ? 'I' : isNonIFrameStart(recordType) ? 'j' : 'x', recordPCR);
    fNextIndexRecordNum
      += (fState == DELIVERING_SAVED_FRAME) ? 1 : fDirection;

    // Handle this index record, depending on the record type and our current state:
    switch (fState) {
    case SKIPPING_FRAME:
    case SAVING_AND_DELIVERING_FRAME: {
      //      if (fState == SKIPPING_FRAME) fprintf(stderr, "\tSKIPPING_FRAME\n"); else fprintf(stderr, "\tSAVING_AND_DELIVERING_FRAME\n");//#####
      if (isIFrameStart(recordType)) {
	// Save a record of this frame:
	fSavedFrameIndexRecordStart = fNextIndexRecordNum - fDirection;
	fUseSavedFrameNextTime = True;
	//	fprintf(stderr, "\trecording\n");//#####
	if ((fFrameCount++)%fScale == 0 && fUseSavedFrameNextTime) {
	  // A frame is due now.
	  fFrameCount = 1; // reset to avoid overflow
	  if (fDirection > 0) {
	    // Begin delivering this frame, as we're scanning it:
	    fState = SAVING_AND_DELIVERING_FRAME;
	    //	    fprintf(stderr, "\tdelivering\n");//#####
	    fDesiredDataPCR = recordPCR; // use this frame's PCR
	    attemptDeliveryToClient();
	    return;
	  } else {
	    // Deliver this frame, then resume normal scanning:
	    // (This relies on the index records having begun with an I-frame.)
	    fState = DELIVERING_SAVED_FRAME;
	    fSavedSequentialIndexRecordNum = fNextIndexRecordNum;
	    fDesiredDataPCR = recordPCR;
	    // use this frame's (not the saved frame's) PCR
	    fNextIndexRecordNum = fSavedFrameIndexRecordStart;
	    //	    fprintf(stderr, "\tbeginning delivery of saved frame\n");//#####
	  }
	} else {
	  // No frame is needed now:
	  fState = SKIPPING_FRAME;
	}
      } else if (isNonIFrameStart(recordType)) {
	if ((fFrameCount++)%fScale == 0 && fUseSavedFrameNextTime) {
	  // A frame is due now, so begin delivering the one that we had saved:
	  // (This relies on the index records having begun with an I-frame.)
	  fFrameCount = 1; // reset to avoid overflow
	  fState = DELIVERING_SAVED_FRAME;
	  fSavedSequentialIndexRecordNum = fNextIndexRecordNum;
	  fDesiredDataPCR = recordPCR;
	  // use this frame's (not the saved frame's) PCR
	  fNextIndexRecordNum = fSavedFrameIndexRecordStart;
	  //	  fprintf(stderr, "\tbeginning delivery of saved frame\n");//#####
	} else {
	  // No frame is needed now:
	  fState = SKIPPING_FRAME;
	}
      } else {
	// Not the start of a frame, but deliver it, if it's needed:
	if (fState == SAVING_AND_DELIVERING_FRAME) {
	  //	  fprintf(stderr, "\tdelivering\n");//#####
	  fDesiredDataPCR = recordPCR; // use this frame's PCR
	  attemptDeliveryToClient();
	  return;
	}
      }
      break;
    }
    case DELIVERING_SAVED_FRAME: {
      //      fprintf(stderr, "\tDELIVERING_SAVED_FRAME\n");//#####
      if (endOfIndexFile
	  || (isIFrameStart(recordType)
	      && fNextIndexRecordNum-1 != fSavedFrameIndexRecordStart)
	  || isNonIFrameStart(recordType)) {
	//	fprintf(stderr, "\tended delivery of saved frame\n");//#####
	// We've reached the end of the saved frame, so revert to the
	// original sequence of index records:
	fNextIndexRecordNum = fSavedSequentialIndexRecordNum;
	fUseSavedFrameNextTime = KEEP_ORIGINAL_FRAME_RATE;
	fState = SKIPPING_FRAME;
      } else {
	// Continue delivering:
	//	fprintf(stderr, "\tdelivering\n");//#####
	attemptDeliveryToClient();
	return;
      }
      break;
    }
    }
  }
}

void MPEG2TransportStreamTrickModeFilter::doStopGettingFrames() {
  FramedFilter::doStopGettingFrames();
  fIndexFile->stopReading();
}

void MPEG2TransportStreamTrickModeFilter::attemptDeliveryToClient() {
  if (fCurrentTSPacketNum == fDesiredTSPacketNum) {
    //    fprintf(stderr, "\t\tdelivering ts %d:%d, %d bytes, PCR %f\n", fCurrentTSPacketNum, fDesiredDataOffset, fDesiredDataSize, fDesiredDataPCR);//#####
    // We already have the Transport Packet that we want.  Deliver its data:
    memmove(fTo, &fInputBuffer[fDesiredDataOffset], fDesiredDataSize);
    fFrameSize = fDesiredDataSize;
    float deliveryPCR = fDirection*(fDesiredDataPCR - fFirstPCR)/fScale;
    if (deliveryPCR < 0.0) deliveryPCR = 0.0;
    fPresentationTime.tv_sec = (unsigned long)deliveryPCR;
    fPresentationTime.tv_usec
      = (unsigned long)((deliveryPCR - fPresentationTime.tv_sec)*1000000.0f);
    //    fprintf(stderr, "#####DGNF9\n");

    afterGetting(this);
  } else {
    // Arrange to read the Transport Packet that we want:
    readTransportPacket(fDesiredTSPacketNum);
  }
}

void MPEG2TransportStreamTrickModeFilter::seekToTransportPacket(unsigned long tsPacketNum) {
  if (tsPacketNum == fNextTSPacketNum) return; // we're already there

  ByteStreamFileSource* tsFile = (ByteStreamFileSource*)fInputSource;
  u_int64_t tsPacketNum64 = (u_int64_t)tsPacketNum;
  tsFile->seekToByteAbsolute(tsPacketNum64*TRANSPORT_PACKET_SIZE);

  fNextTSPacketNum = tsPacketNum;
}

void MPEG2TransportStreamTrickModeFilter::readTransportPacket(unsigned long tsPacketNum) {
  seekToTransportPacket(tsPacketNum);
  fInputSource->getNextFrame(fInputBuffer, TRANSPORT_PACKET_SIZE,
			     afterGettingFrame, this,
			     onSourceClosure, this);
}

void MPEG2TransportStreamTrickModeFilter
::afterGettingFrame(void* clientData, unsigned frameSize,
		    unsigned /*numTruncatedBytes*/,
		    struct timeval presentationTime,
		    unsigned /*durationInMicroseconds*/) {
  MPEG2TransportStreamTrickModeFilter* filter = (MPEG2TransportStreamTrickModeFilter*)clientData;
  filter->afterGettingFrame1(frameSize);
}

void MPEG2TransportStreamTrickModeFilter::afterGettingFrame1(unsigned frameSize) {
  if (frameSize != TRANSPORT_PACKET_SIZE) {
    // Treat this as if the input source ended:
    onSourceClosure1();
    return;
  }

  fCurrentTSPacketNum = fNextTSPacketNum; // i.e., the one that we just read
  ++fNextTSPacketNum;

  // Attempt deliver again:
  attemptDeliveryToClient();
}

void MPEG2TransportStreamTrickModeFilter::onSourceClosure(void* clientData) {
  MPEG2TransportStreamTrickModeFilter* filter = (MPEG2TransportStreamTrickModeFilter*)clientData;
  filter->onSourceClosure1();
}

void MPEG2TransportStreamTrickModeFilter::onSourceClosure1() {
  fIndexFile->stopReading();
  FramedSource::handleClosure(this);
}
