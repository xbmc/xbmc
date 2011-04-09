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
// A source object for AMR audio files (as defined in RFC 3267, section 5)
// C++ header

#ifndef _AMR_AUDIO_FILE_SOURCE_HH
#define _AMR_AUDIO_FILE_SOURCE_HH

#ifndef _AMR_AUDIO_SOURCE_HH
#include "AMRAudioSource.hh"
#endif

class AMRAudioFileSource: public AMRAudioSource {
public:
  static AMRAudioFileSource* createNew(UsageEnvironment& env,
				       char const* fileName);

private:
  AMRAudioFileSource(UsageEnvironment& env, FILE* fid,
		     Boolean isWideband, unsigned numChannels);
	// called only by createNew()

  virtual ~AMRAudioFileSource();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();

private:
  FILE* fFid;
};

#endif
