#pragma once
#include "../../DynamicDll.h"

class DllWMAInterface
{
public:
    virtual ~DllWMAInterface() { }
    virtual int Init()=0;
    virtual void* LoadFile(const char* szFileName, long long* totalTime, int *sampleRate, int* bitsPerSample, int *channels)=0;
    virtual void UnloadFile(void* hnd)=0;
    virtual int FillBuffer(void* hnd, char* buffer, int length)=0;
    virtual unsigned long Seek(void* hnd, unsigned long iTimePos)=0;
};

class DllWMA : public DllDynamic, DllWMAInterface
{
#ifdef _LINUX
  DECLARE_DLL_WRAPPER(DllWMA, q:\\system\\players\\paplayer\\wma-i486-linux.so)
#else
  DECLARE_DLL_WRAPPER(DllWMA, q:\\system\\players\\paplayer\\ffwma.dll)
#endif
  DEFINE_METHOD0(int, Init)
  DEFINE_METHOD5(void*, LoadFile, (const char* p1, long long* p2, int *p3, int* p4, int *p5))
  DEFINE_METHOD1(void, UnloadFile, (void* p1))
  DEFINE_METHOD3(int, FillBuffer, (void* p1, char* p2, int p3))
  DEFINE_METHOD2(unsigned long, Seek, (void* p1, unsigned long p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_Init, Init)
    RESOLVE_METHOD_RENAME(DLL_LoadFile, LoadFile)
    RESOLVE_METHOD_RENAME(DLL_UnloadFile, UnloadFile)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
    RESOLVE_METHOD_RENAME(DLL_Seek, Seek)
  END_METHOD_RESOLVE()
};
