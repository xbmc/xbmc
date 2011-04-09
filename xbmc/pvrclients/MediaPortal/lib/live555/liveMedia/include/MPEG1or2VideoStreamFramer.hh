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
// A filter that breaks up an MPEG 1 or 2 video elementary stream into
//   frames for: Video_Sequence_Header, GOP_Header, Picture_Header
// C++ header

#ifndef _MPEG_1OR2_VIDEO_STREAM_FRAMER_HH
#define _MPEG_1OR2_VIDEO_STREAM_FRAMER_HH

#ifndef _MPEG_VIDEO_STREAM_FRAMER_HH
#include "MPEGVideoStreamFramer.hh"
#endif

class MPEG1or2VideoStreamFramer: public MPEGVideoStreamFramer {
public:
  static MPEG1or2VideoStreamFramer*
      createNew(UsageEnvironment& env, FramedSource* inputSource,
		Boolean iFramesOnly = False,
		double vshPeriod = 5.0
		/* how often (in seconds) to inject a Video_Sequence_Header,
		   if one doesn't already appear in the stream */);

protected:
  MPEG1or2VideoStreamFramer(UsageEnvironment& env,
			    FramedSource* inputSource,
			    Boolean iFramesOnly, double vshPeriod,
			    Boolean createParser = True);
      // called only by createNew(), or by subclass constructors
  virtual ~MPEG1or2VideoStreamFramer();

private:
  // redefined virtual functions:
  virtual Boolean isMPEG1or2VideoStreamFramer() const;

private:
  double getCurrentPTS() const;

  friend class MPEG1or2VideoStreamParser; // hack
};

#endif
