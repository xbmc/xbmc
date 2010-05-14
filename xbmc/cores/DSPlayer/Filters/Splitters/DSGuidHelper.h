#pragma once
#include "streams.h"
#include "dvdmedia.h"
#include "DVDPlayer/Codecs/DllAvCodec.h"

class CMediaType;

class CDSGuidHelper
{
public:
  CDSGuidHelper(void){};
  CMediaType initAudioType(CodecID codecId);
  CMediaType initVideoType(CodecID codecId);
};

extern CDSGuidHelper g_GuidHelper;