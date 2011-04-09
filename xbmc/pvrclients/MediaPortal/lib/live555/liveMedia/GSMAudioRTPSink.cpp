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
// RTP sink for GSM audio
// Implementation

#include "GSMAudioRTPSink.hh"

GSMAudioRTPSink::GSMAudioRTPSink(UsageEnvironment& env, Groupsock* RTPgs)
  : AudioRTPSink(env, RTPgs, 3, 8000, "GSM") {
}

GSMAudioRTPSink::~GSMAudioRTPSink() {
}

GSMAudioRTPSink*
GSMAudioRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs) {
  return new GSMAudioRTPSink(env, RTPgs);
}

Boolean GSMAudioRTPSink
::frameCanAppearAfterPacketStart(unsigned char const* /*frameStart*/,
				 unsigned /*numBytesInFrame*/) const {
  // Allow at most 5 frames in a single packet:
  return numFramesUsedSoFar() < 5;
}
