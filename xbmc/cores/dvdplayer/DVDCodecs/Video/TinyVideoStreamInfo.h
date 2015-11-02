#ifndef TINY_VIDEOSTREAM_INFO_H
#define TINY_VIDEOSTREAM_INFO_H

typedef struct _TinyVideoStreamInfo
{
  // Codec BaseData
  enum AVCodecID codec_id;
  int height;
  int width;

  // Codec ExtraData
  void* extradata;
  unsigned int extrasize;
  bool mvc3d;
  
}TinyVideoStreamInfo;

#endif


