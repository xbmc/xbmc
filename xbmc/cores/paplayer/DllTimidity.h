#pragma once
#include "../../DynamicDll.h"

class DllTimidityInterface
{
public:
    virtual ~DllTimidityInterface() {} 
    virtual int Init()=0;
    virtual int LoadMID(const char* szFileName)=0;
    virtual void FreeMID(int mid)=0;
    virtual int FillBuffer(int mid, char* buffer, int length)=0;
    virtual unsigned long Seek(int mid, unsigned long iTimePos)=0;
    virtual const char* GetTitle(int mid)=0;
    virtual const char* GetArtist(int mid)=0;
    virtual unsigned long GetLength(int mid)=0;
};

class DllTimidity : public DllDynamic, DllTimidityInterface
{
#ifdef _LINUX
  DECLARE_DLL_WRAPPER(DllTimidity, q:\\system\\players\\paplayer\\timidity.dll)
#else
  DECLARE_DLL_WRAPPER(DllTimidity, q:\\system\\players\\paplayer\\timidity-i486-linux.so)
#endif
  DEFINE_METHOD0(int, Init)
  DEFINE_METHOD1(int, LoadMID, (const char* p1))
  DEFINE_METHOD1(void, FreeMID, (int p1))
  DEFINE_METHOD3(int, FillBuffer, (int p1, char* p2, int p3))
  DEFINE_METHOD2(unsigned long, Seek, (int p1, unsigned long p2))
  DEFINE_METHOD1(const char*, GetTitle, (int p1))
  DEFINE_METHOD1(const char*, GetArtist, (int p1))
  DEFINE_METHOD1(unsigned long, GetLength, (int p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_Init, Init)
    RESOLVE_METHOD_RENAME(DLL_LoadMID, LoadMID)
    RESOLVE_METHOD_RENAME(DLL_FreeMID, FreeMID)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
    RESOLVE_METHOD_RENAME(DLL_Seek, Seek)
    RESOLVE_METHOD_RENAME(DLL_GetTitle, GetTitle)
    RESOLVE_METHOD_RENAME(DLL_GetArtist, GetArtist)
    RESOLVE_METHOD_RENAME(DLL_GetLength, GetLength)
  END_METHOD_RESOLVE()
};

