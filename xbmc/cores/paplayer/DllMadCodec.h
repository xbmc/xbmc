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
  DECLARE_DLL_WRAPPER(DllMadCodec, Q:\\system\\players\\PAPlayer\\MADCodec.dll)
  DEFINE_METHOD2(IAudioDecoder*, CreateAudioDecoder, (unsigned int p1, IAudioOutput **p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(CreateAudioDecoder)
  END_METHOD_RESOLVE()
};
