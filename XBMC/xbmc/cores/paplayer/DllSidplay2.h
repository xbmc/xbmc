#pragma once
#include "../../DynamicDll.h"

class DllSidplay2Interface
{
public:
    virtual ~DllSidplay2Interface() {}
    virtual int Init()=0;
    virtual int LoadSID(const char* szFileName)=0;
    virtual void FreeSID(int sid)=0;
    virtual void StartPlayback(int sid, int track)=0;
    virtual int FillBuffer(int sid, void* buffer, int length)=0;
    virtual int GetNumberOfSongs(const char* szFileName)=0;
    virtual void SetSpeed(int sid, int speed)=0;
};

class DllSidplay2 : public DllDynamic, DllSidplay2Interface
{
#ifdef _LINUX
  DECLARE_DLL_WRAPPER(DllSidplay2, q:\\system\\players\\paplayer\\libsidplay2-i486-linux.so)
#else
  DECLARE_DLL_WRAPPER(DllSidplay2, q:\\system\\players\\paplayer\\libsidplay2.dll)
#endif
  DEFINE_METHOD0(int, Init)
  DEFINE_METHOD1(int, LoadSID, (const char* p1))
  DEFINE_METHOD1(void, FreeSID, (int p1))
  DEFINE_METHOD2(void, StartPlayback,(int p1, int p2))
  DEFINE_METHOD3(int, FillBuffer, (int p1, void* p2, int p3))
  DEFINE_METHOD1(int, GetNumberOfSongs, (const char* p1))
  DEFINE_METHOD2(void, SetSpeed, (int p1, int p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_Init, Init)
    RESOLVE_METHOD_RENAME(DLL_LoadSID, LoadSID)
    RESOLVE_METHOD_RENAME(DLL_FreeSID, FreeSID)
    RESOLVE_METHOD_RENAME(DLL_StartPlayback,StartPlayback)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
    RESOLVE_METHOD_RENAME(DLL_GetNumberOfSongs, GetNumberOfSongs)
    RESOLVE_METHOD_RENAME(DLL_SetSpeed, SetSpeed)
  END_METHOD_RESOLVE()
};

