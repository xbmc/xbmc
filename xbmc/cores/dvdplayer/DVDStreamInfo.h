#pragma once

#ifndef _LINUX
enum StreamType;
enum CodecID;
#else
#include "DVDDemuxers/DVDDemux.h"
#include "avcodec.h"
#endif

class CDemuxStream;

class CDVDStreamInfo
{
public:
  CDVDStreamInfo();
  CDVDStreamInfo(const CDVDStreamInfo &right, bool withextradata );
  CDVDStreamInfo(const CDemuxStream &right, bool withextradata );

  ~CDVDStreamInfo();

  void Clear(); // clears current information
  bool Equal(const CDVDStreamInfo &right, bool withextradata);
  bool Equal(const CDemuxStream &right, bool withextradata);

  void Assign(const CDVDStreamInfo &right, bool withextradata);
  void Assign(const CDemuxStream &right, bool withextradata);

  CodecID codec;
  StreamType type;

  // VIDEO
  int fpsscale; // scale of 1000 and a rate of 29970 will result in 29.97 fps
  int fpsrate;
  int height; // height of the stream reported by the demuxer
  int width; // width of the stream reported by the demuxer
  float aspect; // // display aspect as reported by demuxer

  // AUDIO
  int channels;
  int samplerate;

  // CODEC EXTRADATA
  void*        extradata; // extra data for codec to use
  unsigned int extrasize; // size of extra data

  bool operator==(const CDVDStreamInfo& right)      { return Equal(right, true);}
  bool operator!=(const CDVDStreamInfo& right)      { return !Equal(right, true);}
  void operator=(const CDVDStreamInfo& right)       { Assign(right, true); }

  bool operator==(const CDemuxStream& right)      { return Equal( CDVDStreamInfo(right, true), true);}
  bool operator!=(const CDemuxStream& right)      { return !Equal( CDVDStreamInfo(right, true), true);}
  void operator=(const CDemuxStream& right)      { Assign(right, true); }

};
