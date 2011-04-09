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
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from a MPEG-1 or 2 Elementary Stream video file.
// C++ header

#ifndef _MPEG_1OR2_VIDEO_FILE_SERVER_MEDIA_SUBSESSION_HH
#define _MPEG_1OR2_VIDEO_FILE_SERVER_MEDIA_SUBSESSION_HH

#ifndef _FILE_SERVER_MEDIA_SUBSESSION_HH
#include "FileServerMediaSubsession.hh"
#endif

class MPEG1or2VideoFileServerMediaSubsession: public FileServerMediaSubsession{
public:
  static MPEG1or2VideoFileServerMediaSubsession*
  createNew(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource,
	    Boolean iFramesOnly = False,
	    double vshPeriod = 5.0
	    /* how often (in seconds) to inject a Video_Sequence_Header,
	       if one doesn't already appear in the stream */);

private:
  MPEG1or2VideoFileServerMediaSubsession(UsageEnvironment& env,
					 char const* fileName,
					 Boolean reuseFirstSource,
					 Boolean iFramesOnly,
					 double vshPeriod);
      // called only by createNew();
  virtual ~MPEG1or2VideoFileServerMediaSubsession();

private: // redefined virtual functions
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);

private:
  Boolean fIFramesOnly;
  double fVSHPeriod;
};

#endif
