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
// AMR Audio RTP Sources (RFC 3267)
// C++ header

#ifndef _AMR_AUDIO_RTP_SOURCE_HH
#define _AMR_AUDIO_RTP_SOURCE_HH

#ifndef _RTP_SOURCE_HH
#include "RTPSource.hh"
#endif
#ifndef _AMR_AUDIO_SOURCE_HH
#include "AMRAudioSource.hh"
#endif

class AMRAudioRTPSource {
public:
  static AMRAudioSource* createNew(UsageEnvironment& env,
				   Groupsock* RTPgs,
				   RTPSource*& resultRTPSource,
				   unsigned char rtpPayloadFormat,
				   Boolean isWideband = False,
				   unsigned numChannels = 1,
				   Boolean isOctetAligned = True,
				   unsigned interleaving = 0,
				     // relevant only if "isOctetAligned"
				     // The maximum # of frame-blocks in a group
				     // 0 means: no interleaving
				   Boolean robustSortingOrder = False,
				     // relevant only if "isOctetAligned"
				   Boolean CRCsArePresent = False
				     // relevant only if "isOctetAligned"
				   );
      // This returns a source to read from, but "resultRTPSource" will
      // point to RTP-related state.
};

#endif
