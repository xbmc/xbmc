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
// Transcoder for ADUized MP3 frames
// C++ header

#ifndef _MP3_ADU_TRANSCODER_HH
#define _MP3_ADU_TRANSCODER_HH

#ifndef _FRAMED_FILTER_HH
#include "FramedFilter.hh"
#endif

class MP3ADUTranscoder: public FramedFilter {
public:
  static MP3ADUTranscoder* createNew(UsageEnvironment& env,
				  unsigned outBitrate /* in kbps */,
				  FramedSource* inputSource);

  unsigned outBitrate() const { return fOutBitrate; }
protected:
  MP3ADUTranscoder(UsageEnvironment& env,
		unsigned outBitrate /* in kbps */,
		FramedSource* inputSource);
      // called only by createNew()
  virtual ~MP3ADUTranscoder();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void getAttributes() const;

private:
  static void afterGettingFrame(void* clientData,
				unsigned numBytesRead, unsigned numTruncatedBytes,
				struct timeval presentationTime,
				unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned numBytesRead, unsigned numTruncatedBytes,
			  struct timeval presentationTime,
			  unsigned durationInMicroseconds);

private:
  unsigned fOutBitrate; // in kbps
  unsigned fAvailableBytesForBackpointer;

  unsigned char* fOrigADU;
      // used to store incoming ADU prior to transcoding
};

#endif
