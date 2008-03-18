#pragma once
#include "../../DynamicDll.h"

class DllNosefartInterface
{
public:
    virtual ~DllNosefartInterface() {}
    virtual int LoadNSF(const char* szFileName)=0;
    virtual void FreeNSF(int)=0;
    virtual int StartPlayback(int nsf, int track)=0;
    virtual long FillBuffer(int nsf, char* buffer, int size)=0;
    virtual void FrameAdvance(int nsf)=0;
    virtual int GetPlaybackRate(int nsf)=0;
    virtual int GetNumberOfSongs(int nsf)=0;
    virtual int GetTitle(int nsf)=0;
    virtual int GetArtist(int nsf)=0;
};

class DllNosefart : public DllDynamic, DllNosefartInterface
{
#ifdef _LINUX
  DECLARE_DLL_WRAPPER(DllNosefart, q:\\system\\players\\paplayer\\nosefart-i486-linux.so)
#else
  DECLARE_DLL_WRAPPER(DllNosefart, q:\\system\\players\\paplayer\\nosefart.dll)
#endif
  DEFINE_METHOD1(int, LoadNSF, (const char* p1))
  DEFINE_METHOD1(void, FreeNSF, (int p1))
  DEFINE_METHOD2(int, StartPlayback, (int p1, int p2))
  DEFINE_METHOD3(long, FillBuffer, (int p1, char* p2, int p3))
  DEFINE_METHOD1(void, FrameAdvance, (int p1))
  DEFINE_METHOD1(int, GetPlaybackRate, (int p1))
  DEFINE_METHOD1(int, GetNumberOfSongs, (int p1))
  DEFINE_METHOD1(int, GetTitle, (int p1))
  DEFINE_METHOD1(int, GetArtist, (int p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_LoadNSF, LoadNSF)
    RESOLVE_METHOD_RENAME(DLL_FreeNSF, FreeNSF)
    RESOLVE_METHOD_RENAME(DLL_StartPlayback, StartPlayback)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
    RESOLVE_METHOD_RENAME(DLL_FrameAdvance, FrameAdvance)
    RESOLVE_METHOD_RENAME(DLL_GetPlaybackRate, GetPlaybackRate)
    RESOLVE_METHOD_RENAME(DLL_GetNumberOfSongs, GetNumberOfSongs)
    RESOLVE_METHOD_RENAME(DLL_GetTitle, GetTitle)
    RESOLVE_METHOD_RENAME(DLL_GetArtist, GetArtist)
  END_METHOD_RESOLVE()
};
