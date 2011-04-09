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
// A filter for converting one or more MPEG Elementary Streams
// to a MPEG-2 Transport Stream
// Implementation

#include "MPEG2TransportStreamFromESSource.hh"

#define MAX_INPUT_ES_FRAME_SIZE 50000
#define SIMPLE_PES_HEADER_SIZE 14
#define LOW_WATER_MARK 1000 // <= MAX_INPUT_ES_FRAME_SIZE
#define INPUT_BUFFER_SIZE (SIMPLE_PES_HEADER_SIZE + 2*MAX_INPUT_ES_FRAME_SIZE)

////////// InputESSourceRecord definition //////////

class InputESSourceRecord {
public:
  InputESSourceRecord(MPEG2TransportStreamFromESSource& parent,
		      FramedSource* inputSource,
		      u_int8_t streamId, int mpegVersion,
		      InputESSourceRecord* next);
  virtual ~InputESSourceRecord();

  InputESSourceRecord* next() const { return fNext; }
  FramedSource* inputSource() const { return fInputSource; }

  void askForNewData();
  Boolean deliverBufferToClient();

  unsigned char* buffer() const { return fInputBuffer; }
  void reset() {
    // Reset the buffer for future use:
    fInputBufferBytesAvailable = 0;
    fInputBufferInUse = False;
  }

private:
  static void afterGettingFrame(void* clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
                                struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned frameSize,
                          unsigned numTruncatedBytes,
                          struct timeval presentationTime);

private:
  InputESSourceRecord* fNext;
  MPEG2TransportStreamFromESSource& fParent;
  FramedSource* fInputSource;
  u_int8_t fStreamId;
  int fMPEGVersion;
  unsigned char* fInputBuffer;
  unsigned fInputBufferBytesAvailable;
  Boolean fInputBufferInUse;
  MPEG1or2Demux::SCR fSCR;
};


////////// MPEG2TransportStreamFromESSource implementation //////////

MPEG2TransportStreamFromESSource* MPEG2TransportStreamFromESSource
::createNew(UsageEnvironment& env) {
  return new MPEG2TransportStreamFromESSource(env);
}

void MPEG2TransportStreamFromESSource
::addNewVideoSource(FramedSource* inputSource, int mpegVersion) {
  u_int8_t streamId = 0xE0 | (fVideoSourceCounter++&0x0F);
  addNewInputSource(inputSource, streamId, mpegVersion);
  fHaveVideoStreams = True;
}

void MPEG2TransportStreamFromESSource
::addNewAudioSource(FramedSource* inputSource, int mpegVersion) {
  u_int8_t streamId = 0xC0 | (fAudioSourceCounter++&0x0F);
  addNewInputSource(inputSource, streamId, mpegVersion);
}

MPEG2TransportStreamFromESSource
::MPEG2TransportStreamFromESSource(UsageEnvironment& env)
  : MPEG2TransportStreamMultiplexor(env),
    fInputSources(NULL), fVideoSourceCounter(0), fAudioSourceCounter(0) {
  fHaveVideoStreams = False; // unless we add a video source
}

MPEG2TransportStreamFromESSource::~MPEG2TransportStreamFromESSource() {
  delete fInputSources;
}

void MPEG2TransportStreamFromESSource::doStopGettingFrames() {
  // Stop each input source:
  for (InputESSourceRecord* sourceRec = fInputSources; sourceRec != NULL;
       sourceRec = sourceRec->next()) {
    sourceRec->inputSource()->stopGettingFrames();
  }
}

void MPEG2TransportStreamFromESSource
::awaitNewBuffer(unsigned char* oldBuffer) {
  InputESSourceRecord* sourceRec;
  // Begin by resetting the old buffer:
  if (oldBuffer != NULL) {
    for (sourceRec = fInputSources; sourceRec != NULL;
	 sourceRec = sourceRec->next()) {
      if (sourceRec->buffer() == oldBuffer) {
	sourceRec->reset();
	break;
      }
    }
  }

  if (isCurrentlyAwaitingData()) {
    // Try to deliver one filled-in buffer to the client:
    for (sourceRec = fInputSources; sourceRec != NULL;
	 sourceRec = sourceRec->next()) {
      if (sourceRec->deliverBufferToClient()) break;
    }
  }

  // No filled-in buffers are available. Ask each of our inputs for data:
  for (sourceRec = fInputSources; sourceRec != NULL;
       sourceRec = sourceRec->next()) {
    sourceRec->askForNewData();
  }

}

void MPEG2TransportStreamFromESSource
::addNewInputSource(FramedSource* inputSource,
		    u_int8_t streamId, int mpegVersion) {
  if (inputSource == NULL) return;
  fInputSources = new InputESSourceRecord(*this, inputSource, streamId,
					  mpegVersion, fInputSources);
}


////////// InputESSourceRecord implementation //////////

InputESSourceRecord
::InputESSourceRecord(MPEG2TransportStreamFromESSource& parent,
		      FramedSource* inputSource,
		      u_int8_t streamId, int mpegVersion,
		      InputESSourceRecord* next)
  : fNext(next), fParent(parent), fInputSource(inputSource),
    fStreamId(streamId), fMPEGVersion(mpegVersion) {
  fInputBuffer = new unsigned char[INPUT_BUFFER_SIZE];
  reset();
}

InputESSourceRecord::~InputESSourceRecord() {
  Medium::close(fInputSource);
  delete[] fInputBuffer;
  delete fNext;
}

void InputESSourceRecord::askForNewData() {
  if (fInputBufferInUse) return;

  if (fInputBufferBytesAvailable == 0) {
    // Reset our buffer, by adding a simple PES header at the start:
    fInputBuffer[0] = 0; fInputBuffer[1] = 0; fInputBuffer[2] = 1;
    fInputBuffer[3] = fStreamId;
    fInputBuffer[4] = 0; fInputBuffer[5] = 0; // fill in later with the length
    fInputBuffer[6] = 0x80;
    fInputBuffer[7] = 0x80; // include a PTS
    fInputBuffer[8] = 5; // PES_header_data_length (enough for a PTS)
    // fInputBuffer[9..13] will be the PTS; fill this in later
    fInputBufferBytesAvailable = SIMPLE_PES_HEADER_SIZE;
  }
  if (fInputBufferBytesAvailable < LOW_WATER_MARK &&
      !fInputSource->isCurrentlyAwaitingData()) {
    // We don't yet have enough data in our buffer.  Arrange to read more:
    fInputSource->getNextFrame(&fInputBuffer[fInputBufferBytesAvailable],
                               INPUT_BUFFER_SIZE-fInputBufferBytesAvailable,
                               afterGettingFrame, this,
                               FramedSource::handleClosure, &fParent);
  }
}

Boolean InputESSourceRecord::deliverBufferToClient() {
  if (fInputBufferInUse || fInputBufferBytesAvailable < LOW_WATER_MARK) return False;

  // Fill in the PES_packet_length field that we left unset before:
  unsigned PES_packet_length = fInputBufferBytesAvailable - 6;
  if (PES_packet_length > 0xFFFF) {
    // Set the PES_packet_length field to 0.  This indicates an unbounded length (see ISO 13818-1, 2.4.3.7)
    PES_packet_length = 0;
  }
  fInputBuffer[4] = PES_packet_length>>8;
  fInputBuffer[5] = PES_packet_length;

  // Fill in the PES PTS (from our SCR):
  fInputBuffer[9] = 0x20|(fSCR.highBit<<3)|(fSCR.remainingBits>>29)|0x01;
  fInputBuffer[10] = fSCR.remainingBits>>22;
  fInputBuffer[11] = (fSCR.remainingBits>>14)|0x01;
  fInputBuffer[12] = fSCR.remainingBits>>7;
  fInputBuffer[13] = (fSCR.remainingBits<<1)|0x01;

  fInputBufferInUse = True;

  // Do the delivery:
  fParent.handleNewBuffer(fInputBuffer, fInputBufferBytesAvailable,
			 fMPEGVersion, fSCR);

  return True;
}

void InputESSourceRecord
::afterGettingFrame(void* clientData, unsigned frameSize,
		    unsigned numTruncatedBytes,
		    struct timeval presentationTime,
		    unsigned /*durationInMicroseconds*/) {
  InputESSourceRecord* source = (InputESSourceRecord*)clientData;
  source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime);
}
void InputESSourceRecord
::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
		     struct timeval presentationTime) {
  if (numTruncatedBytes > 0) {
    fParent.envir() << "MPEG2TransportStreamFromESSource: input buffer too small; increase \"MAX_INPUT_ES_FRAME_SIZE\" in \"MPEG2TransportStreamFromESSource\" by at least "
		    << numTruncatedBytes << " bytes!\n";
  }

  if (fInputBufferBytesAvailable == SIMPLE_PES_HEADER_SIZE) {
    // Use this presentationTime for our SCR:
    fSCR.highBit
      = ((presentationTime.tv_sec*45000 + (presentationTime.tv_usec*9)/200)&
	 0x80000000) != 0;
    fSCR.remainingBits
      = presentationTime.tv_sec*90000 + (presentationTime.tv_usec*9)/100;
    fSCR.extension = (presentationTime.tv_usec*9)%100;
#ifdef DEBUG_SCR
    fprintf(stderr, "PES header: stream_id 0x%02x, pts: %u.%06u => SCR 0x%x%08x:%03x\n", fStreamId, (unsigned)presentationTime.tv_sec, (unsigned)presentationTime.tv_usec, fSCR.highBit, fSCR.remainingBits, fSCR.extension);
#endif
  }

  fInputBufferBytesAvailable += frameSize;

  fParent.fPresentationTime = presentationTime;

  // Now that we have new input data, check if we can deliver to the client:
  fParent.awaitNewBuffer(NULL);
}
