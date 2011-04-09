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
// A source object for AMR audio sources
// C++ header

#ifndef _AMR_AUDIO_SOURCE_HH
#define _AMR_AUDIO_SOURCE_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

class AMRAudioSource: public FramedSource {
public:
  Boolean isWideband() const { return fIsWideband; }
  unsigned numChannels() const { return fNumChannels; }

  u_int8_t lastFrameHeader() const { return fLastFrameHeader; }
  // The frame header for the most recently read frame (RFC 3267, sec. 5.3)

protected:
  AMRAudioSource(UsageEnvironment& env, Boolean isWideband, unsigned numChannels);
	// virtual base class
  virtual ~AMRAudioSource();

private:
  // redefined virtual functions:
  virtual Boolean isAMRAudioSource() const;

protected:
  Boolean fIsWideband;
  unsigned fNumChannels;
  u_int8_t fLastFrameHeader;
};

#endif
