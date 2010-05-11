#pragma once

#include "DVDPlayer/Codecs/DllAvCodec.h"
/*extern "C"
{
#include "libavcodec/avcodec.h"
}*/

class CMediaType;

class CDSGuidHelper
{
public:
  CDSGuidHelper(void){};
  CMediaType initAudioType(CodecID codecId);
  CMediaType initVideoType(CodecID codecId);
  
};

extern CDSGuidHelper g_GuidHelper;