#pragma once
#include "DynamicDll.h"

typedef int abool;

typedef struct {
  char author[128];
  char name[128];
  int year;
  int month;
  int day;
  int channels;
  int duration;
} ASAP_SongInfo;

class DllASAPInterface
{
public:
  virtual ~DllASAPInterface() {}
  virtual int asapGetSongs(const char *filename)=0;
  virtual abool asapGetInfo(const char *filename, int song, ASAP_SongInfo *songInfo)=0;
  virtual abool asapLoad(const char *filename, int song, int *channels, int *duration)=0;
  virtual void asapSeek(int position)=0;
  virtual int asapGenerate(void *buffer, int buffer_len)=0;
};

class DllASAP : public DllDynamic, DllASAPInterface
{
#ifdef _LINUX
  DECLARE_DLL_WRAPPER(DllASAP, q:\\system\\players\\paplayer\\xbmc_asap-i486-linux.so)
#else
  DECLARE_DLL_WRAPPER(DllASAP, q:\\system\\players\\paplayer\\xbmc_asap.dll)
#endif
  DEFINE_METHOD1(int, asapGetSongs, (const char *p1))
  DEFINE_METHOD3(abool, asapGetInfo, (const char *p1, int p2, ASAP_SongInfo *p3))
  DEFINE_METHOD4(abool, asapLoad, (const char *p1, int p2, int *p3, int *p4))
  DEFINE_METHOD1(void, asapSeek, (int p1))
  DEFINE_METHOD2(int, asapGenerate, (void *p1, int p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(asapGetSongs)
    RESOLVE_METHOD(asapGetInfo)
    RESOLVE_METHOD(asapLoad)
    RESOLVE_METHOD(asapSeek)
    RESOLVE_METHOD(asapGenerate)
  END_METHOD_RESOLVE()
};
