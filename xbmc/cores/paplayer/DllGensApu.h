#pragma once
#include "../../DynamicDll.h"

class DllGensApuInterface
{
public:
    virtual ~DllGensApuInterface() {}
    virtual int Init()=0;
    virtual int LoadGYM(const char* szFileName)=0;
    virtual void FreeGYM(int gym)=0;
    virtual int FillBuffer(int gym, void* buffer)=0;
    virtual void Seek(int gym, unsigned int iPos)=0;
    virtual int GetTitle(int spc)=0;
    virtual int GetArtist(int spc)=0;
    virtual __int64 GetLength(int gym)=0;
};

class DllGensApu : public DllDynamic, DllGensApuInterface
{
#ifndef _LINUX
  DECLARE_DLL_WRAPPER(DllGensApu, q:\\system\\players\\paplayer\\gensapu.dll)
#else
  DECLARE_DLL_WRAPPER(DllGensApu, q:\\system\\players\\paplayer\\gensapu-i486-linux.so)
#endif
  DEFINE_METHOD0(int, Init)
  DEFINE_METHOD1(int, LoadGYM, (const char* p1))
  DEFINE_METHOD1(void, FreeGYM, (int p1))
  DEFINE_METHOD2(int, FillBuffer, (int p1, void* p2))
  DEFINE_METHOD2(void, Seek, (int p1, unsigned int p2))
  DEFINE_METHOD1(int, GetTitle, (int p1))
  DEFINE_METHOD1(int, GetArtist, (int p1))
  DEFINE_METHOD1(__int64, GetLength, (int p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_Init, Init)
    RESOLVE_METHOD_RENAME(DLL_LoadGYM, LoadGYM)
    RESOLVE_METHOD_RENAME(DLL_FreeGYM, FreeGYM)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
    RESOLVE_METHOD_RENAME(DLL_Seek, Seek)
    RESOLVE_METHOD_RENAME(DLL_GetTitle, GetTitle)
    RESOLVE_METHOD_RENAME(DLL_GetArtist, GetArtist)
    RESOLVE_METHOD_RENAME(DLL_GetLength, GetLength)
  END_METHOD_RESOLVE()
};
