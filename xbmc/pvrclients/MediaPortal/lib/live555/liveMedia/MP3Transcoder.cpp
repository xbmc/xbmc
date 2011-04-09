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
// MP3 Transcoder
// Implementation

#include "MP3Transcoder.hh"

MP3Transcoder::MP3Transcoder(UsageEnvironment& env,
			     MP3ADUTranscoder* aduTranscoder)
  : MP3FromADUSource(env, aduTranscoder, False) {
}

MP3Transcoder::~MP3Transcoder() {
}

MP3Transcoder* MP3Transcoder::createNew(UsageEnvironment& env,
					unsigned outBitrate /* in kbps */,
					FramedSource* inputSource) {
  MP3Transcoder* newSource = NULL;

  do {
    // Create the intermediate filters that help implement the transcoder:
    ADUFromMP3Source* aduFromMP3
      = ADUFromMP3Source::createNew(env, inputSource, False);
    // Note: This also checks that "inputSource" is an MP3 source
    if (aduFromMP3 == NULL) break;

    MP3ADUTranscoder* aduTranscoder
      = MP3ADUTranscoder::createNew(env, outBitrate, aduFromMP3);
    if (aduTranscoder == NULL) break;

    // Then create the transcoder itself:
    newSource = new MP3Transcoder(env, aduTranscoder);
  } while (0);

  return newSource;
}
