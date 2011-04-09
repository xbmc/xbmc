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
// Demultiplexer for a MPEG 1 or 2 Program Stream
// C++ header

#ifndef _MPEG_1OR2_DEMUX_HH
#define _MPEG_1OR2_DEMUX_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

class MPEG1or2DemuxedElementaryStream; // forward

class MPEG1or2Demux: public Medium {
public:
  static MPEG1or2Demux* createNew(UsageEnvironment& env,
				  FramedSource* inputSource,
				  Boolean reclaimWhenLastESDies = False);
  // If "reclaimWhenLastESDies" is True, the the demux is deleted when
  // all "MPEG1or2DemuxedElementaryStream"s that we created get deleted.

  MPEG1or2DemuxedElementaryStream* newElementaryStream(u_int8_t streamIdTag);

  // Specialized versions of the above for audio and video:
  MPEG1or2DemuxedElementaryStream* newAudioStream();
  MPEG1or2DemuxedElementaryStream* newVideoStream();

  // A hack for getting raw, undemuxed PES packets from the Program Stream:
  MPEG1or2DemuxedElementaryStream* newRawPESStream();

  void getNextFrame(u_int8_t streamIdTag,
		    unsigned char* to, unsigned maxSize,
		    FramedSource::afterGettingFunc* afterGettingFunc,
		    void* afterGettingClientData,
		    FramedSource::onCloseFunc* onCloseFunc,
		    void* onCloseClientData);
      // similar to FramedSource::getNextFrame(), except that it also
      // takes a stream id tag as parameter.

  void stopGettingFrames(u_int8_t streamIdTag);
      // similar to FramedSource::stopGettingFrames(), except that it also
      // takes a stream id tag as parameter.

  static void handleClosure(void* clientData);
      // This should be called (on ourself) if the source is discovered
      // to be closed (i.e., no longer readable)

  FramedSource* inputSource() const { return fInputSource; }

  class SCR {
  public:
    SCR();

    u_int8_t highBit;
    u_int32_t remainingBits;
    u_int16_t extension;

    Boolean isValid;
  };
  SCR& lastSeenSCR() { return fLastSeenSCR; }

  unsigned char mpegVersion() const { return fMPEGversion; }

  void flushInput(); // should be called before any 'seek' on the underlying source

private:
  MPEG1or2Demux(UsageEnvironment& env,
		FramedSource* inputSource, Boolean reclaimWhenLastESDies);
      // called only by createNew()
  virtual ~MPEG1or2Demux();

  void registerReadInterest(u_int8_t streamIdTag,
			    unsigned char* to, unsigned maxSize,
			    FramedSource::afterGettingFunc* afterGettingFunc,
			    void* afterGettingClientData,
			    FramedSource::onCloseFunc* onCloseFunc,
			    void* onCloseClientData);

  Boolean useSavedData(u_int8_t streamIdTag,
		       unsigned char* to, unsigned maxSize,
		       FramedSource::afterGettingFunc* afterGettingFunc,
		       void* afterGettingClientData);

  static void continueReadProcessing(void* clientData,
				     unsigned char* ptr, unsigned size,
				     struct timeval presentationTime);
  void continueReadProcessing();

private:
  friend class MPEG1or2DemuxedElementaryStream;
  void noteElementaryStreamDeletion(MPEG1or2DemuxedElementaryStream* es);

private:
  FramedSource* fInputSource;
  SCR fLastSeenSCR;
  unsigned char fMPEGversion;

  unsigned char fNextAudioStreamNumber;
  unsigned char fNextVideoStreamNumber;
  Boolean fReclaimWhenLastESDies;
  unsigned fNumOutstandingESs;

  // A descriptor for each possible stream id tag:
  typedef struct OutputDescriptor {
    // input parameters
    unsigned char* to; unsigned maxSize;
    FramedSource::afterGettingFunc* fAfterGettingFunc;
    void* afterGettingClientData;
    FramedSource::onCloseFunc* fOnCloseFunc;
    void* onCloseClientData;

    // output parameters
    unsigned frameSize; struct timeval presentationTime;
    class SavedData; // forward
    SavedData* savedDataHead;
    SavedData* savedDataTail;
    unsigned savedDataTotalSize;

    // status parameters
    Boolean isPotentiallyReadable;
    Boolean isCurrentlyActive;
    Boolean isCurrentlyAwaitingData;
  } OutputDescriptor_t;
  OutputDescriptor_t fOutput[256];

  unsigned fNumPendingReads;
  Boolean fHaveUndeliveredData;

private: // parsing state
  class MPEGProgramStreamParser* fParser;
  friend class MPEGProgramStreamParser; // hack
};

#endif
