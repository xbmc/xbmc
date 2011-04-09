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
// on demand, from a MPEG-1 or 2 demuxer.
// C++ header

#ifndef _MPEG_1OR2_DEMUXED_SERVER_MEDIA_SUBSESSION_HH
#define _MPEG_1OR2_DEMUXED_SERVER_MEDIA_SUBSESSION_HH

#ifndef _ON_DEMAND_SERVER_MEDIA_SUBSESSION_HH
#include "OnDemandServerMediaSubsession.hh"
#endif
#ifndef _MPEG_1OR2_FILE_SERVER_DEMUX_HH
#include "MPEG1or2FileServerDemux.hh"
#endif

class MPEG1or2DemuxedServerMediaSubsession: public OnDemandServerMediaSubsession{
public:
  static MPEG1or2DemuxedServerMediaSubsession*
  createNew(MPEG1or2FileServerDemux& demux, u_int8_t streamIdTag,
	    Boolean reuseFirstSource,
	    Boolean iFramesOnly = False, double vshPeriod = 5.0);
  // The last two parameters are relevant for video streams only

private:
  MPEG1or2DemuxedServerMediaSubsession(MPEG1or2FileServerDemux& demux,
				       u_int8_t streamIdTag, Boolean reuseFirstSource,
				       Boolean iFramesOnly, double vshPeriod);
      // called only by createNew();
  virtual ~MPEG1or2DemuxedServerMediaSubsession();

private: // redefined virtual functions
  virtual void seekStreamSource(FramedSource* inputSource, double seekNPT);
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);
  virtual float duration() const;

private:
  MPEG1or2FileServerDemux& fOurDemux;
  u_int8_t fStreamIdTag;
  Boolean fIFramesOnly; // for video streams
  double fVSHPeriod; // for video streams
};

#endif
