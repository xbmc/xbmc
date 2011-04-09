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
// NOTE: Samples are returned in little-endian order (the same order in which
// they were stored in the file).
// C++ header

#ifndef _WAV_AUDIO_FILE_SOURCE_HH
#define _WAV_AUDIO_FILE_SOURCE_HH

#ifndef _AUDIO_INPUT_DEVICE_HH
#include "AudioInputDevice.hh"
#endif

typedef enum {
  WA_PCM = 1,
  WA_PCMA = 6,
  WA_PCMU = 7,
  WA_UNKNOWN
}WAV_AUDIO_FORMAT;


class WAVAudioFileSource: public AudioInputDevice {
public:

  static WAVAudioFileSource* createNew(UsageEnvironment& env,
					char const* fileName);

  unsigned numPCMBytes() const;
  void setScaleFactor(int scale);
  void seekToPCMByte(unsigned byteNumber);

  unsigned char getAudioFormat();

protected:
  WAVAudioFileSource(UsageEnvironment& env, FILE* fid);
	// called only by createNew()

  virtual ~WAVAudioFileSource();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual Boolean setInputPort(int portIndex);
  virtual double getAverageLevel() const;

private:
  FILE* fFid;
  double fPlayTimePerSample; // useconds
  unsigned fPreferredFrameSize;
  unsigned fLastPlayTime; // useconds
  unsigned fWAVHeaderSize;
  unsigned fFileSize;
  int fScaleFactor;

  unsigned char fAudioFormat;
};

#endif
