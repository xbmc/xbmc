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
// A WAV audio file source
// Implementation

#include "WAVAudioFileSource.hh"
#include "InputFile.hh"
#include "GroupsockHelper.hh"

////////// WAVAudioFileSource //////////

WAVAudioFileSource*
WAVAudioFileSource::createNew(UsageEnvironment& env, char const* fileName) {
  do {
    FILE* fid = OpenInputFile(env, fileName);
    if (fid == NULL) break;

    WAVAudioFileSource* newSource = new WAVAudioFileSource(env, fid);
    if (newSource != NULL && newSource->bitsPerSample() == 0) {
      // The WAV file header was apparently invalid.
      Medium::close(newSource);
      break;
    }

    newSource->fFileSize = (unsigned)GetFileSize(fileName, fid);

    return newSource;
  } while (0);

  return NULL;
}

unsigned WAVAudioFileSource::numPCMBytes() const {
  if (fFileSize < fWAVHeaderSize) return 0;
  return fFileSize - fWAVHeaderSize;
}

void WAVAudioFileSource::setScaleFactor(int scale) {
  fScaleFactor = scale;

  if (fScaleFactor < 0 && ftell(fFid) > 0) {
    // Because we're reading backwards, seek back one sample, to ensure that
    // (i)  we start reading the last sample before the start point, and
    // (ii) we don't hit end-of-file on the first read.
    int const bytesPerSample = (fNumChannels*fBitsPerSample)/8;
    fseek(fFid, -bytesPerSample, SEEK_CUR);
  }
}

void WAVAudioFileSource::seekToPCMByte(unsigned byteNumber) {
  byteNumber += fWAVHeaderSize;
  if (byteNumber > fFileSize) byteNumber = fFileSize;

  fseek(fFid, byteNumber, SEEK_SET);
}

unsigned char WAVAudioFileSource::getAudioFormat() {
  return fAudioFormat;
}


#define nextc fgetc(fid)

static Boolean get4Bytes(FILE* fid, unsigned& result) { // little-endian
  int c0, c1, c2, c3;
  if ((c0 = nextc) == EOF || (c1 = nextc) == EOF ||
      (c2 = nextc) == EOF || (c3 = nextc) == EOF) return False;
  result = (c3<<24)|(c2<<16)|(c1<<8)|c0;
  return True;
}

static Boolean get2Bytes(FILE* fid, unsigned short& result) {//little-endian
  int c0, c1;
  if ((c0 = nextc) == EOF || (c1 = nextc) == EOF) return False;
  result = (c1<<8)|c0;
  return True;
}

static Boolean skipBytes(FILE* fid, int num) {
  while (num-- > 0) {
    if (nextc == EOF) return False;
  }
  return True;
}

WAVAudioFileSource::WAVAudioFileSource(UsageEnvironment& env, FILE* fid)
  : AudioInputDevice(env, 0, 0, 0, 0)/* set the real parameters later */,
    fFid(fid), fLastPlayTime(0), fWAVHeaderSize(0), fFileSize(0), fScaleFactor(1),
    fAudioFormat(WA_UNKNOWN) {
  // Check the WAV file header for validity.
  // Note: The following web pages contain info about the WAV format:
  // http://www.technology.niagarac.on.ca/courses/comp630/WavFileFormat.html
  // http://ccrma-www.stanford.edu/CCRMA/Courses/422/projects/WaveFormat/
  // http://www.ringthis.com/dev/wave_format.htm
  // http://www.lightlink.com/tjweber/StripWav/Canon.html
  // http://www.borg.com/~jglatt/tech/wave.htm
  // http://www.wotsit.org/download.asp?f=wavecomp

  Boolean success = False; // until we learn otherwise
  do {
    // RIFF Chunk:
    if (nextc != 'R' || nextc != 'I' || nextc != 'F' || nextc != 'F') break;
    if (!skipBytes(fid, 4)) break;
    if (nextc != 'W' || nextc != 'A' || nextc != 'V' || nextc != 'E') break;

    // FORMAT Chunk:
    if (nextc != 'f' || nextc != 'm' || nextc != 't' || nextc != ' ') break;
    unsigned formatLength;
    if (!get4Bytes(fid, formatLength)) break;
    unsigned short audioFormat;
    if (!get2Bytes(fid, audioFormat)) break;

    fAudioFormat = (unsigned char)audioFormat;
    if (fAudioFormat != WA_PCM && fAudioFormat != WA_PCMA && fAudioFormat != WA_PCMU) {
      // not PCM/PCMU/PCMA - we can't handle this
      env.setResultMsg("Audio format is not PCM/PCMU/PCMA");
      break;
    }
    unsigned short numChannels;
    if (!get2Bytes(fid, numChannels)) break;
    fNumChannels = (unsigned char)numChannels;
    if (fNumChannels < 1 || fNumChannels > 2) { // invalid # channels
      char errMsg[100];
      sprintf(errMsg, "Bad # channels: %d", fNumChannels);
      env.setResultMsg(errMsg);
      break;
    }
    if (!get4Bytes(fid, fSamplingFrequency)) break;
    if (fSamplingFrequency == 0) {
      env.setResultMsg("Bad sampling frequency: 0");
      break;
    }
    if (!skipBytes(fid, 6)) break;
    unsigned short bitsPerSample;
    if (!get2Bytes(fid, bitsPerSample)) break;
    fBitsPerSample = (unsigned char)bitsPerSample;
    if (fBitsPerSample == 0) {
      env.setResultMsg("Bad bits-per-sample: 0");
      break;
    }
    if (!skipBytes(fid, formatLength - 16)) break;

    // FACT chunk (optional):
    int c = nextc;
    if (c == 'f') {
      if (nextc != 'a' || nextc != 'c' || nextc != 't') break;
      unsigned factLength;
      if (!get4Bytes(fid, factLength)) break;
      if (!skipBytes(fid, factLength)) break;
      c = nextc;
    }

    // DATA Chunk:
    if (c != 'd' || nextc != 'a' || nextc != 't' || nextc != 'a') break;
    if (!skipBytes(fid, 4)) break;

    // The header is good; the remaining data are the sample bytes.
    fWAVHeaderSize = ftell(fid);
    success = True;
  } while (0);

  if (!success) {
    env.setResultMsg("Bad WAV file format");
    // Set "fBitsPerSample" to zero, to indicate failure:
    fBitsPerSample = 0;
    return;
  }

  fPlayTimePerSample = 1e6/(double)fSamplingFrequency;

  // Although PCM is a sample-based format, we group samples into
  // 'frames' for efficient delivery to clients.  Set up our preferred
  // frame size to be close to 20 ms, if possible, but always no greater
  // than 1400 bytes (to ensure that it will fit in a single RTP packet)
  unsigned maxSamplesPerFrame = (1400*8)/(fNumChannels*fBitsPerSample);
  unsigned desiredSamplesPerFrame = (unsigned)(0.02*fSamplingFrequency);
  unsigned samplesPerFrame = desiredSamplesPerFrame < maxSamplesPerFrame
    ? desiredSamplesPerFrame : maxSamplesPerFrame;
  fPreferredFrameSize = (samplesPerFrame*fNumChannels*fBitsPerSample)/8;
}

WAVAudioFileSource::~WAVAudioFileSource() {
  CloseInputFile(fFid);
}

// Note: We should change the following to use asynchronous file reading, #####
// as we now do with ByteStreamFileSource. #####
void WAVAudioFileSource::doGetNextFrame() {
  if (feof(fFid) || ferror(fFid)) {
    handleClosure(this);
    return;
  }

  // Try to read as many bytes as will fit in the buffer provided
  // (or "fPreferredFrameSize" if less)
  if (fPreferredFrameSize < fMaxSize) {
    fMaxSize = fPreferredFrameSize;
  }
  unsigned const bytesPerSample = (fNumChannels*fBitsPerSample)/8;
  unsigned bytesToRead = fMaxSize - fMaxSize%bytesPerSample;
  if (fScaleFactor == 1) {
    // Common case - read samples in bulk:
    fFrameSize = fread(fTo, 1, bytesToRead, fFid);
  } else {
    // We read every 'fScaleFactor'th sample:
    fFrameSize = 0;
    while (bytesToRead > 0) {
      size_t bytesRead = fread(fTo, 1, bytesPerSample, fFid);
      if (bytesRead <= 0) break;
      fTo += bytesRead;
      fFrameSize += bytesRead;
      bytesToRead -= bytesRead;

      // Seek to the appropriate place for the next sample:
      fseek(fFid, (fScaleFactor-1)*bytesPerSample, SEEK_CUR);
    }
  }

  // Set the 'presentation time' and 'duration' of this frame:
  if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
    // This is the first frame, so use the current time:
    gettimeofday(&fPresentationTime, NULL);
  } else {
    // Increment by the play time of the previous data:
    unsigned uSeconds	= fPresentationTime.tv_usec + fLastPlayTime;
    fPresentationTime.tv_sec += uSeconds/1000000;
    fPresentationTime.tv_usec = uSeconds%1000000;
  }

  // Remember the play time of this data:
  fDurationInMicroseconds = fLastPlayTime
    = (unsigned)((fPlayTimePerSample*fFrameSize)/bytesPerSample);

  // Switch to another task, and inform the reader that he has data:
#if defined(__WIN32__) || defined(_WIN32)
  // HACK: One of our applications that uses this source uses an
  // implementation of scheduleDelayedTask() that performs very badly
  // (chewing up lots of CPU time, apparently polling) on Windows.
  // Until this is fixed, we just call our "afterGetting()" function
  // directly.  This avoids infinite recursion, as long as our sink
  // is discontinuous, which is the case for the RTP sink that
  // this application uses. #####
  afterGetting(this);
#else
  nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
			(TaskFunc*)FramedSource::afterGetting, this);
#endif
}

Boolean WAVAudioFileSource::setInputPort(int /*portIndex*/) {
  return True;
}

double WAVAudioFileSource::getAverageLevel() const {
  return 0.0;//##### fix this later
}
