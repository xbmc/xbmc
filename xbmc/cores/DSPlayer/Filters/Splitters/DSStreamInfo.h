#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "../../../dvdplayer/Codecs/DllAvCodec.h"
#include "../../../dvdplayer/Codecs/DllAvFormat.h"
#include "streams.h"
enum StreamType;
enum CodecID;


class CDemuxStream;

class CDSStreamInfo
{
public:
  CDSStreamInfo();
  CDSStreamInfo(const CDSStreamInfo &right, bool withextradata = true);
  CDSStreamInfo(const CDemuxStream &right, bool withextradata = true);

  ~CDSStreamInfo();

  void Clear(); // clears current information
  bool Equal(const CDSStreamInfo &right, bool withextradata);
  bool Equal(const CDemuxStream &right, bool withextradata);

  void Assign(const CDSStreamInfo &right, bool withextradata);
  void Assign(const CDemuxStream &right, bool withextradata);

  CodecID codec_id;
  StreamType type;
  bool software;  //force software decoding

  CMediaType mtype;
  // VIDEO
  int fpsscale; // scale of 1000 and a rate of 29970 will result in 29.97 fps
  int fpsrate;
  REFERENCE_TIME avgtimeperframe;
  int height; // height of the stream reported by the demuxer
  int width; // width of the stream reported by the demuxer
  float aspect; // display aspect as reported by demuxer
  bool vfr; // variable framerate
  bool stills; // there may be odd still frames in video

  // AUDIO
  unsigned int formatextraaudio;
  int channels;
  int samplerate;
  int bitrate;
  int blockalign;
  int bitspersample;

  // SUBTITLE
  int identifier;

  // CODEC EXTRADATA
  void*        extradata; // extra data for codec_id to use
  unsigned int extrasize; // size of extra data

  bool operator==(const CDSStreamInfo& right)      { return Equal(right, true);}
  bool operator!=(const CDSStreamInfo& right)      { return !Equal(right, true);}
  void operator=(const CDSStreamInfo& right)       { Assign(right, true); }

  bool operator==(const CDemuxStream& right)      { return Equal( CDSStreamInfo(right, true), true);}
  bool operator!=(const CDemuxStream& right)      { return !Equal( CDSStreamInfo(right, true), true);}
  void operator=(const CDemuxStream& right)      { Assign(right, true); }
protected:
  DllAvCodec  m_dllAvCodec;
  DllAvFormat   m_dllAvFormat;

};
