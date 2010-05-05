#pragma once
#include "ffmpeg_mediaguids.h"
extern "C"
{
#include "libavcodec/avcodec.h"
}

class CDSGuidHelper
{
public:
  CDSGuidHelper(void){};
  const FOURCC* getCodecFOURCCs(CodecID codecId);
};

extern CDSGuidHelper g_GuidHelper;