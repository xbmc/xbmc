/*
 *      Copyright (C) 2008-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "StSoundLibrary.h"
#include "YmMusic.h"

#include <stdio.h>
#include <stdint.h>

#if defined(__linux__) || defined(__FreeBSD__)
#define __declspec(x)
#endif

extern "C"
{ 
  __declspec(dllexport) void* DLL_LoadYM(const char* szFileName)
  {
    YMMUSIC *pMusic = ymMusicCreate();

    if (ymMusicLoad(pMusic,szFileName))
    {
      ymMusicSetLoopMode(pMusic,YMFALSE);
      ymMusicPlay(pMusic);
      return pMusic;
    }
  
    ymMusicDestroy(pMusic);

    return 0;
  }

  void __declspec(dllexport) DLL_FreeYM(void* ym)
  {
    ymMusicStop((YMMUSIC*)ym);
    ymMusicDestroy((YMMUSIC*)ym);
  }

  int __declspec(dllexport) DLL_FillBuffer(void* ym, char* szBuffer, int iSize)
  {
    if (ymMusicCompute((YMMUSIC*)ym,(ymsample*)szBuffer,iSize/2))
      return iSize;
    else
      return 0;
  }

  unsigned long __declspec(dllexport) DLL_Seek(void* ym, unsigned long timepos)
  {
    if (ymMusicIsSeekable((YMMUSIC*)ym))
    {
      ymMusicSeek((YMMUSIC*)ym,timepos);
      return timepos;
    }

    return 0;
  }

  const char __declspec(dllexport) *DLL_GetTitle(void* ym)
  {
    ymMusicInfo_t info;
    ymMusicGetInfo((YMMUSIC*)ym,&info);
    return info.pSongName;
  }

  const char __declspec(dllexport) *DLL_GetArtist(void* ym)
  {
    ymMusicInfo_t info;
    ymMusicGetInfo((YMMUSIC*)ym,&info);
    return info.pSongAuthor;
  }

  unsigned long __declspec(dllexport) DLL_GetLength(void* ym)
  {
    ymMusicInfo_t info;
    ymMusicGetInfo((YMMUSIC*)ym,&info);
    return info.musicTimeInSec;
  }
}

