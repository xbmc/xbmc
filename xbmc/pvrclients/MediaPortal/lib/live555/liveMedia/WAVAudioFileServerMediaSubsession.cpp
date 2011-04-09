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
// on demand, from an WAV audio file.
// Implementation

#include "WAVAudioFileServerMediaSubsession.hh"
#include "WAVAudioFileSource.hh"
#include "uLawAudioFilter.hh"
#include "SimpleRTPSink.hh"

WAVAudioFileServerMediaSubsession* WAVAudioFileServerMediaSubsession
::createNew(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource,
	    Boolean convertToULaw) {
  return new WAVAudioFileServerMediaSubsession(env, fileName,
					       reuseFirstSource, convertToULaw);
}

WAVAudioFileServerMediaSubsession
::WAVAudioFileServerMediaSubsession(UsageEnvironment& env, char const* fileName,
				    Boolean reuseFirstSource, Boolean convertToULaw)
  : FileServerMediaSubsession(env, fileName, reuseFirstSource),
    fConvertToULaw(convertToULaw) {
}

WAVAudioFileServerMediaSubsession
::~WAVAudioFileServerMediaSubsession() {
}

void WAVAudioFileServerMediaSubsession
::seekStreamSource(FramedSource* inputSource, double seekNPT) {
  WAVAudioFileSource* wavSource;
  if (fBitsPerSample == 16) {
    // "inputSource" is a filter; its input source is the original WAV file source:
    wavSource = (WAVAudioFileSource*)(((FramedFilter*)inputSource)->inputSource());
  } else {
    // "inputSource" is the original WAV file source:
    wavSource = (WAVAudioFileSource*)inputSource;
  }

  unsigned seekSampleNumber = (unsigned)(seekNPT*fSamplingFrequency);
  unsigned seekByteNumber = (seekSampleNumber*fNumChannels*fBitsPerSample)/8;

  wavSource->seekToPCMByte(seekByteNumber);
}

void WAVAudioFileServerMediaSubsession
::setStreamSourceScale(FramedSource* inputSource, float scale) {
  int iScale = (int)scale;
  WAVAudioFileSource* wavSource;
  if (fBitsPerSample == 16) {
    // "inputSource" is a filter; its input source is the original WAV file source:
    wavSource = (WAVAudioFileSource*)(((FramedFilter*)inputSource)->inputSource());
  } else {
    // "inputSource" is the original WAV file source:
    wavSource = (WAVAudioFileSource*)inputSource;
  }

  wavSource->setScaleFactor(iScale);
}

FramedSource* WAVAudioFileServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  FramedSource* resultSource = NULL;
  do {
    WAVAudioFileSource* wavSource
      = WAVAudioFileSource::createNew(envir(), fFileName);
    if (wavSource == NULL) break;

    // Get attributes of the audio source:

    fAudioFormat = wavSource->getAudioFormat();
    fBitsPerSample = wavSource->bitsPerSample();
    if (fBitsPerSample != 8 && fBitsPerSample !=  16) {
      envir() << "The input file contains " << fBitsPerSample
	      << " bit-per-sample audio, which we don't handle\n";
      break;
    }
    fSamplingFrequency = wavSource->samplingFrequency();
    fNumChannels = wavSource->numChannels();
    unsigned bitsPerSecond
      = fSamplingFrequency*fBitsPerSample*fNumChannels;

    fFileDuration = (float)((8.0*wavSource->numPCMBytes())
      /(fSamplingFrequency*fNumChannels*fBitsPerSample));

    // Add in any filter necessary to transform the data prior to streaming:
    if (fBitsPerSample == 16) {
      // Note that samples in the WAV audio file are in little-endian order.
      if (fConvertToULaw) {
	// Add a filter that converts from raw 16-bit PCM audio
	// to 8-bit u-law audio:
	resultSource
	  = uLawFromPCMAudioSource::createNew(envir(), wavSource, 1/*little-endian*/);
	bitsPerSecond /= 2;
      } else {
	// Add a filter that converts from little-endian to network (big-endian) order:
	resultSource = EndianSwap16::createNew(envir(), wavSource);
      }
    } else { // fBitsPerSample == 8
      // Don't do any transformation; send the 8-bit PCM data 'as is':
      resultSource = wavSource;
    }

    estBitrate = (bitsPerSecond+500)/1000; // kbps
    return resultSource;
  } while (0);

  // An error occurred:
  Medium::close(resultSource);
  return NULL;
}

RTPSink* WAVAudioFileServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* /*inputSource*/) {
  do {
    char const* mimeType;
    unsigned char payloadFormatCode;
    if (fAudioFormat == WA_PCM) {
      if (fBitsPerSample == 16) {
	if (fConvertToULaw) {
	  mimeType = "PCMU";
	  if (fSamplingFrequency == 8000 && fNumChannels == 1) {
	    payloadFormatCode = 0; // a static RTP payload type
	  } else {
	    payloadFormatCode = rtpPayloadTypeIfDynamic;
	  }
	} else {
	  mimeType = "L16";
	  if (fSamplingFrequency == 44100 && fNumChannels == 2) {
	    payloadFormatCode = 10; // a static RTP payload type
	  } else if (fSamplingFrequency == 44100 && fNumChannels == 1) {
	    payloadFormatCode = 11; // a static RTP payload type
	  } else {
	    payloadFormatCode = rtpPayloadTypeIfDynamic;
	  }
	}
      } else { // fBitsPerSample == 8
	mimeType = "L8";
	payloadFormatCode = rtpPayloadTypeIfDynamic;
      }
    } else if (fAudioFormat == WA_PCMU) {
      mimeType = "PCMU";
      if (fSamplingFrequency == 8000 && fNumChannels == 1) {
	payloadFormatCode = 0; // a static RTP payload type
      } else {
	payloadFormatCode = rtpPayloadTypeIfDynamic;
      }
    } else if (fAudioFormat == WA_PCMA) {
      mimeType = "PCMA";
      if (fSamplingFrequency == 8000 && fNumChannels == 1) {
	payloadFormatCode = 8; // a static RTP payload type
      } else {
	payloadFormatCode = rtpPayloadTypeIfDynamic;
      }
    } else { //unknown format
		break;
	}

    return SimpleRTPSink::createNew(envir(), rtpGroupsock,
				    payloadFormatCode, fSamplingFrequency,
				    "audio", mimeType, fNumChannels);
  } while (0);

  // An error occurred:
  return NULL;
}

void WAVAudioFileServerMediaSubsession::testScaleFactor(float& scale) {
  if (fFileDuration <= 0.0) {
    // The file is non-seekable, so is probably a live input source.
    // We don't support scale factors other than 1
    scale = 1;
  } else {
    // We support any integral scale, other than 0
    int iScale = scale < 0.0 ? (int)(scale - 0.5) : (int)(scale + 0.5); // round
    if (iScale == 0) iScale = 1;
    scale = (float)iScale;
  }
}

float WAVAudioFileServerMediaSubsession::duration() const {
  return fFileDuration;
}
