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
// C++ header

#ifndef _MP3_TRANSCODER_HH
#define _MP3_TRANSCODER_HH

#ifndef _MP3_ADU_HH
#include "MP3ADU.hh"
#endif
#ifndef _MP3_ADU_TRANSCODER_HH
#include "MP3ADUTranscoder.hh"
#endif

class MP3Transcoder: public MP3FromADUSource {
public:
  static MP3Transcoder* createNew(UsageEnvironment& env,
				  unsigned outBitrate /* in kbps */,
				  FramedSource* inputSource);

protected:
  MP3Transcoder(UsageEnvironment& env,
		MP3ADUTranscoder* aduTranscoder);
      // called only by createNew()
  virtual ~MP3Transcoder();
};

#endif
