#pragma once
#include "../../DynamicDll.h"
#include "aac/AACCodec.h"

class DllAACCodecInterface
{
public:
  virtual ~DllAACCodecInterface() {}
  virtual AACHandle AACOpen(const char *fn, AACIOCallbacks callbacks)=0;
  virtual int AACRead(AACHandle handle, BYTE* pBuffer, int iSize)=0;
  virtual int AACSeek(AACHandle handle, int iTimeMs)=0;
  virtual void AACClose(AACHandle handle)=0;
  virtual const char* AACGetErrorMessage()=0;
  virtual int AACGetInfo(AACHandle handle, AACInfo* info)=0;
};

class DllAACCodec : public DllDynamic, DllAACCodecInterface
{
#ifndef _LINUX
  DECLARE_DLL_WRAPPER(DllAACCodec, Q:\\system\\players\\paplayer\\aaccodec.dll)
#else
  DECLARE_DLL_WRAPPER(DllAACCodec, Q:\\system\\players\\paplayer\\AACCodec-i486-linux.so)
#endif
  DEFINE_METHOD2(AACHandle, AACOpen, (const char *p1, AACIOCallbacks p2))
  DEFINE_METHOD3(int, AACRead, (AACHandle p1, BYTE* p2, int p3))
  DEFINE_METHOD2(int, AACSeek, (AACHandle p1, int p2))
  DEFINE_METHOD1(void, AACClose, (AACHandle p1))
  DEFINE_METHOD0(const char*, AACGetErrorMessage)
  DEFINE_METHOD2(int, AACGetInfo, (AACHandle p1, AACInfo* p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(AACOpen)
    RESOLVE_METHOD(AACRead)
    RESOLVE_METHOD(AACSeek)
    RESOLVE_METHOD(AACClose)
    RESOLVE_METHOD(AACGetErrorMessage)
    RESOLVE_METHOD(AACGetInfo)
  END_METHOD_RESOLVE()
};
