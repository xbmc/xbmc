#pragma once
#include "../../DynamicDll.h"

class DllAdplugInterface
{
public:
    virtual ~DllAdplugInterface() {}
    virtual int Init()=0;
    virtual int LoadADL(const char* szFileName)=0;
    virtual void FreeADL(int adl)=0;
    virtual int FillBuffer(int adl, char* buffer, int length)=0;
    virtual unsigned long Seek(int adl, unsigned long iTimePos)=0;
    virtual const char* GetTitle(int adl)=0;
    virtual const char* GetArtist(int adl)=0;
    virtual unsigned long GetLength(int adl)=0;
};

class DllAdplug : public DllDynamic, DllAdplugInterface
{
  DECLARE_DLL_WRAPPER(DllAdplug, q:\\system\\players\\paplayer\\adplug.dll)
  DEFINE_METHOD0(int, Init)
  DEFINE_METHOD1(int, LoadADL, (const char* p1))
  DEFINE_METHOD1(void, FreeADL, (int p1))
  DEFINE_METHOD3(int, FillBuffer, (int p1, char* p2, int p3))
  DEFINE_METHOD2(unsigned long, Seek, (int p1, unsigned long p2))
  DEFINE_METHOD1(const char*, GetTitle, (int p1))
  DEFINE_METHOD1(const char*, GetArtist, (int p1))
  DEFINE_METHOD1(unsigned long, GetLength, (int p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_Init, Init)
    RESOLVE_METHOD_RENAME(DLL_LoadADL, LoadADL)
    RESOLVE_METHOD_RENAME(DLL_FreeADL, FreeADL)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
    RESOLVE_METHOD_RENAME(DLL_Seek, Seek)
    RESOLVE_METHOD_RENAME(DLL_GetTitle, GetTitle)
    RESOLVE_METHOD_RENAME(DLL_GetArtist, GetArtist)
    RESOLVE_METHOD_RENAME(DLL_GetLength, GetLength)
  END_METHOD_RESOLVE()
};

