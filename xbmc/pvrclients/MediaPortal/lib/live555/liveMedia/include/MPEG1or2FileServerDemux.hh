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
// A server demultiplexer for a MPEG 1 or 2 Program Stream
// C++ header

#ifndef _MPEG_1OR2_FILE_SERVER_DEMUX_HH
#define _MPEG_1OR2_FILE_SERVER_DEMUX_HH

#ifndef _SERVER_MEDIA_SESSION_HH
#include "ServerMediaSession.hh"
#endif
#ifndef _MPEG_1OR2_DEMUXED_ELEMENTARY_STREAM_HH
#include "MPEG1or2DemuxedElementaryStream.hh"
#endif

class MPEG1or2FileServerDemux: public Medium {
public:
  static MPEG1or2FileServerDemux*
  createNew(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource);

  ServerMediaSubsession* newAudioServerMediaSubsession(); // MPEG-1 or 2 audio
  ServerMediaSubsession* newVideoServerMediaSubsession(Boolean iFramesOnly = False,
						       double vshPeriod = 5.0
		       /* how often (in seconds) to inject a Video_Sequence_Header,
			  if one doesn't already appear in the stream */);
  ServerMediaSubsession* newAC3AudioServerMediaSubsession(); // AC-3 audio (from VOB)

  unsigned fileSize() const { return fFileSize; }
  float fileDuration() const { return fFileDuration; }

private:
  MPEG1or2FileServerDemux(UsageEnvironment& env, char const* fileName,
			  Boolean reuseFirstSource);
      // called only by createNew();
  virtual ~MPEG1or2FileServerDemux();

private:
  friend class MPEG1or2DemuxedServerMediaSubsession;
  MPEG1or2DemuxedElementaryStream* newElementaryStream(unsigned clientSessionId,
						       u_int8_t streamIdTag);

private:
  char const* fFileName;
  unsigned fFileSize;
  float fFileDuration;
  Boolean fReuseFirstSource;
  MPEG1or2Demux* fSession0Demux;
  MPEG1or2Demux* fLastCreatedDemux;
  u_int8_t fLastClientSessionId;
};

#endif
