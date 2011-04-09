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
// on demand, from a MPEG-4 video file.
// C++ header

#ifndef _MPEG4_VIDEO_FILE_SERVER_MEDIA_SUBSESSION_HH
#define _MPEG4_VIDEO_FILE_SERVER_MEDIA_SUBSESSION_HH

#ifndef _FILE_SERVER_MEDIA_SUBSESSION_HH
#include "FileServerMediaSubsession.hh"
#endif

class MPEG4VideoFileServerMediaSubsession: public FileServerMediaSubsession{
public:
  static MPEG4VideoFileServerMediaSubsession*
  createNew(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource);

  // Used to implement "getAuxSDPLine()":
  void checkForAuxSDPLine1();
  void afterPlayingDummy1();

private:
  MPEG4VideoFileServerMediaSubsession(UsageEnvironment& env,
				      char const* fileName, Boolean reuseFirstSource);
      // called only by createNew();
  virtual ~MPEG4VideoFileServerMediaSubsession();

  void setDoneFlag() { fDoneFlag = ~0; }

private: // redefined virtual functions
  virtual char const* getAuxSDPLine(RTPSink* rtpSink,
				    FramedSource* inputSource);
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);

private:
  char fDoneFlag; // used when setting up "fSDPLines"
  RTPSink* fDummyRTPSink; // ditto
};

#endif
