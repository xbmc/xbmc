#pragma once
#include "../../DynamicDll.h"
#include "dec_if.h"

class DllMadCodecInterface
{
public:
  virtual ~DllMadCodecInterface() {}
  virtual IAudioDecoder* CreateAudioDecoder (unsigned int type, IAudioOutput **output)=0;
};

class DllMadCodec : public DllDynamic, DllMadCodecInterface
{
#ifndef _LINUX
  DECLARE_DLL_WRAPPER(DllMadCodec, Q:\\system\\players\\PAPlayer\\MADCodec.dll)
#else
  DECLARE_DLL_WRAPPER(DllMadCodec, Q:\\system\\players\\paplayer\\MADCodec-i486-linux.so)
#endif
  DEFINE_METHOD2(IAudioDecoder*, CreateAudioDecoder, (unsigned int p1, IAudioOutput **p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(CreateAudioDecoder)
  END_METHOD_RESOLVE()
};
