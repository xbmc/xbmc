#include "StSoundLibrary.h"
#include "YmMusic.h"

#include <stdio.h>
#include <stdint.h>

#ifdef __linux__
#define __declspec(x)
#endif

extern "C"
{ 
  int __declspec(dllexport) DLL_LoadYM(const char* szFileName)
  {
    YMMUSIC *pMusic = ymMusicCreate();

    if (ymMusicLoad(pMusic,szFileName))
    {
      ymMusicSetLoopMode(pMusic,YMFALSE);
      ymMusicPlay(pMusic);
      return (intptr_t)pMusic;
    }
  
    ymMusicDestroy(pMusic);

    return 0;
  }

  void __declspec(dllexport) DLL_FreeYM(int ym)
  {
    ymMusicStop((YMMUSIC*)ym);
    ymMusicDestroy((YMMUSIC*)ym);
  }

  int __declspec(dllexport) DLL_FillBuffer(int ym, char* szBuffer, int iSize)
  {
    if (ymMusicCompute((YMMUSIC*)ym,(ymsample*)szBuffer,iSize/2))
      return iSize;
    else
      return 0;
  }

  unsigned long __declspec(dllexport) DLL_Seek(int ym, unsigned long timepos)
  {
    if (ymMusicIsSeekable((YMMUSIC*)ym))
    {
      ymMusicSeek((YMMUSIC*)ym,timepos);
      return timepos;
    }

    return 0;
  }

  const char __declspec(dllexport) *DLL_GetTitle(int ym)
  {
    ymMusicInfo_t info;
    ymMusicGetInfo((YMMUSIC*)ym,&info);
    return info.pSongName;
  }

  const char __declspec(dllexport) *DLL_GetArtist(int ym)
  {
    ymMusicInfo_t info;
    ymMusicGetInfo((YMMUSIC*)ym,&info);
    return info.pSongAuthor;
  }

  unsigned long __declspec(dllexport) DLL_GetLength(int ym)
  {
    ymMusicInfo_t info;
    ymMusicGetInfo((YMMUSIC*)ym,&info);
    return info.musicTimeInSec;
  }
}

