/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

#include "DVDDemuxers/DVDDemux.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

class CDemuxStream;

class CDVDStreamInfo
{
public:
  CDVDStreamInfo();
  CDVDStreamInfo(const CDVDStreamInfo &right, bool withextradata = true);
  CDVDStreamInfo(const CDemuxStream &right, bool withextradata = true);

  ~CDVDStreamInfo();

  void Clear(); // clears current information
  bool Equal(const CDVDStreamInfo &right, bool withextradata);
  bool Equal(const CDemuxStream &right, bool withextradata);

  void Assign(const CDVDStreamInfo &right, bool withextradata);
  void Assign(const CDemuxStream &right, bool withextradata);

  AVCodecID codec;
  StreamType type;
  int flags;
  bool software;  //force software decoding
  std::string filename;


  // VIDEO
  int fpsscale; // scale of 1001 and a rate of 60000 will result in 59.94 fps
  int fpsrate;
  int rfpsscale;
  int rfpsrate;
  int height; // height of the stream reported by the demuxer
  int width; // width of the stream reported by the demuxer
  float aspect; // display aspect as reported by demuxer
  bool vfr; // variable framerate
  bool stills; // there may be odd still frames in video
  int level; // encoder level of the stream reported by the decoder. used to qualify hw decoders.
  int profile; // encoder profile of the stream reported by the decoder. used to qualify hw decoders.
  bool ptsinvalid;  // pts cannot be trusted (avi's).
  bool forced_aspect; // aspect is forced from container
  int orientation; // orientation of the video in degress counter clockwise
  int bitsperpixel;
  int pid;
  std::string stereo_mode; // stereoscopic 3d mode

  // AUDIO
  int channels;
  int samplerate;
  int bitrate;
  int blockalign;
  int bitspersample;

  // SUBTITLE

  // CODEC EXTRADATA
  void*        extradata; // extra data for codec to use
  unsigned int extrasize; // size of extra data
  unsigned int codec_tag; // extra identifier hints for decoding

  bool operator==(const CDVDStreamInfo& right)      { return Equal(right, true);}
  bool operator!=(const CDVDStreamInfo& right)      { return !Equal(right, true);}

  CDVDStreamInfo& operator=(const CDVDStreamInfo& right)
  {
    if (this != &right)
      Assign(right, true);

    return *this; 
  }

  bool operator==(const CDemuxStream& right)      { return Equal( CDVDStreamInfo(right, true), true);}
  bool operator!=(const CDemuxStream& right)      { return !Equal( CDVDStreamInfo(right, true), true);}

  CDVDStreamInfo& operator=(const CDemuxStream& right)
  { 
    Assign(right, true);
    return *this;
  }
};
