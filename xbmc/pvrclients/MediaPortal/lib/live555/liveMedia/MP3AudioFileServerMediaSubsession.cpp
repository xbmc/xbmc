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
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from a MP3 audio file.
// (Actually, any MPEG-1 or MPEG-2 audio file should work.)
// Implementation

#include "MP3AudioFileServerMediaSubsession.hh"
#include "MPEG1or2AudioRTPSink.hh"
#include "MP3ADURTPSink.hh"
#include "MP3FileSource.hh"
#include "MP3ADU.hh"

MP3AudioFileServerMediaSubsession* MP3AudioFileServerMediaSubsession
::createNew(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource,
	    Boolean generateADUs, Interleaving* interleaving) {
  return new MP3AudioFileServerMediaSubsession(env, fileName, reuseFirstSource,
					       generateADUs, interleaving);
}

MP3AudioFileServerMediaSubsession
::MP3AudioFileServerMediaSubsession(UsageEnvironment& env,
				    char const* fileName, Boolean reuseFirstSource,
				    Boolean generateADUs,
				    Interleaving* interleaving)
  : FileServerMediaSubsession(env, fileName, reuseFirstSource),
    fGenerateADUs(generateADUs), fInterleaving(interleaving), fFileDuration(0.0) {
}

MP3AudioFileServerMediaSubsession
::~MP3AudioFileServerMediaSubsession() {
  delete fInterleaving;
}

void MP3AudioFileServerMediaSubsession
::seekStreamSource(FramedSource* inputSource, double seekNPT) {
  MP3FileSource* mp3Source;
  if (fGenerateADUs) {
    // "inputSource" is a filter; use its input source instead.
    ADUFromMP3Source* filter;
    if (fInterleaving != NULL) {
      // There's another filter as well.
      filter = (ADUFromMP3Source*)(((FramedFilter*)inputSource)->inputSource());
    } else {
      filter = (ADUFromMP3Source*)inputSource;
    }
    filter->resetInput(); // because we're about to seek within its source
    mp3Source = (MP3FileSource*)(filter->inputSource());
  } else if (fFileDuration > 0.0) {
    // There are a pair of filters - MP3->ADU and ADU->MP3 - in front of the
    // original MP3 source:
    MP3FromADUSource* filter2 = (MP3FromADUSource*)inputSource;
    ADUFromMP3Source* filter1 = (ADUFromMP3Source*)(filter2->inputSource());
    filter1->resetInput(); // because we're about to seek within its source
    mp3Source = (MP3FileSource*)(filter1->inputSource());
  } else {
    // "inputSource" is the original MP3 source:
    mp3Source = (MP3FileSource*)inputSource;
  }

  mp3Source->seekWithinFile(seekNPT);
}

void MP3AudioFileServerMediaSubsession
::setStreamSourceScale(FramedSource* inputSource, float scale) {
  int iScale = (int)scale;
  MP3FileSource* mp3Source;
  ADUFromMP3Source* aduSource = NULL;
  if (fGenerateADUs) {
    if (fInterleaving != NULL) {
      // There's an interleaving filter in front.
      aduSource = (ADUFromMP3Source*)(((FramedFilter*)inputSource)->inputSource());
    } else {
      aduSource = (ADUFromMP3Source*)inputSource;
    }
    mp3Source = (MP3FileSource*)(aduSource->inputSource());
  } else if (fFileDuration > 0.0) {
    // There are a pair of filters - MP3->ADU and ADU->MP3 - in front of the
    // original MP3 source.  So, go back one, to reach the ADU source:
    aduSource = (ADUFromMP3Source*)(((FramedFilter*)inputSource)->inputSource());

    // Then, go back one more, to reach the MP3 source:
    mp3Source = (MP3FileSource*)(aduSource->inputSource());
  } else return; // the stream is not scalable

  aduSource->setScaleFactor(iScale);
  mp3Source->setPresentationTimeScale(iScale);
}

FramedSource* MP3AudioFileServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 128; // kbps, estimate

  FramedSource* streamSource;
  do {
    MP3FileSource* mp3Source;
    streamSource = mp3Source = MP3FileSource::createNew(envir(), fFileName);
    if (streamSource == NULL) break;
    fFileDuration = mp3Source->filePlayTime();

    if (fGenerateADUs) {
      // Add a filter that converts the source MP3s to ADUs:
      streamSource = ADUFromMP3Source::createNew(envir(), streamSource);
      if (streamSource == NULL) break;

      if (fInterleaving != NULL) {
	// Add another filter that interleaves the ADUs before packetizing:
	streamSource = MP3ADUinterleaver::createNew(envir(), *fInterleaving,
						    streamSource);
	if (streamSource == NULL) break;
      }
    } else if (fFileDuration > 0.0) {
      // Because this is a seekable file, insert a pair of filters: one that
      // converts the input MP3 stream to ADUs; another that converts these
      // ADUs back to MP3.  This allows us to seek within the input stream without
      // tripping over the MP3 'bit reservoir':
      streamSource = ADUFromMP3Source::createNew(envir(), streamSource);
      if (streamSource == NULL) break;

      streamSource = MP3FromADUSource::createNew(envir(), streamSource);
      if (streamSource == NULL) break;
    }
  } while (0);

  return streamSource;
}

RTPSink* MP3AudioFileServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* /*inputSource*/) {
  if (fGenerateADUs) {
    return MP3ADURTPSink::createNew(envir(), rtpGroupsock,
				    rtpPayloadTypeIfDynamic);
  } else {
    return MPEG1or2AudioRTPSink::createNew(envir(), rtpGroupsock);
  }
}

void MP3AudioFileServerMediaSubsession::testScaleFactor(float& scale) {
  if (fFileDuration <= 0.0) {
    // The file is non-seekable, so is probably a live input source.
    // We don't support scale factors other than 1
    scale = 1;
  } else {
    // We support any integral scale >= 1
    int iScale = (int)(scale + 0.5); // round
    if (iScale < 1) iScale = 1;
    scale = (float)iScale;
  }
}

float MP3AudioFileServerMediaSubsession::duration() const {
  return fFileDuration;
}
