// timidity.cpp : Defines the entry point for the console application.
//

extern "C"  
{
#include "timidity.h"
}

typedef struct TimiditySong
{
  MidiSong* song;
  char szBuf[1024*2*2];
  char* szStartOfBuf;
  unsigned long iStartSample;
} TimiditySong;

#include <stdio.h>
#include <string.h>

#ifdef _LINUX
#define __declspec(x)
#endif

extern "C"
{
  int __declspec(dllexport) DLL_Init()
  {
    Timidity_Init(48000,3,2,1024);
    return 1;
  }

  int __declspec(dllexport) DLL_LoadMID(const char* szFileName)
  {
    TimiditySong* result = new TimiditySong;
    result->song = Timidity_LoadSong(const_cast<char*>(szFileName));
    result->szStartOfBuf = result->szBuf+1024*4;
    result->iStartSample = -1024;
    return (int)result;
  }

  void __declspec(dllexport) DLL_FreeMID(int mid)
  {
    TimiditySong* song = (TimiditySong*)mid;
    Timidity_FreeSong(song->song);
    delete song;
  }

  int __declspec(dllexport) DLL_FillBuffer(int mid, char* szBuffer, int iSize)
  {
    TimiditySong* song = (TimiditySong*)mid;
    if (!Timidity_Active())
      Timidity_Start(song->song);

    int iCurrSize = iSize;
    while (iCurrSize > 0)
    {
      if (song->szStartOfBuf >= song->szBuf+1024*4)
      {
        if (Timidity_PlaySome(song->szBuf,1024))
          return -1;

        song->szStartOfBuf = song->szBuf;
        song->iStartSample += 1024;
      }
      int iCopy=0;
      if (iCurrSize > song->szBuf+1024*4-song->szStartOfBuf)
        iCopy = song->szBuf+1024*4-song->szStartOfBuf;
      else
        iCopy = iCurrSize;

      memcpy(szBuffer,song->szStartOfBuf,iCopy);
      szBuffer += iCopy;
      iCurrSize -= iCopy;
      song->szStartOfBuf += iCopy;
    }
    return iSize-iCurrSize;
  }

  unsigned long __declspec(dllexport) DLL_Seek(int mid, unsigned long iTimePos)
  {
    TimiditySong* song = (TimiditySong*)mid;
    if (iTimePos/1000*48000 < song->iStartSample)
    {
      song->szStartOfBuf = song->szBuf+1024*4;
      song->iStartSample = -1024;
      Timidity_Start(song->song);
    }
    
    char buffer[1024*4];
    while (song->iStartSample < iTimePos/1000*48000)
    {
      int iCopy = iTimePos/1000*48000-song->iStartSample>1024?1024:iTimePos/1000*48000-song->iStartSample;
      DLL_FillBuffer(mid,buffer,iCopy);
    }
      
     return iTimePos;
  }

  const char __declspec(dllexport) *DLL_GetTitle(int mid)
  {
    return "";
  }

  const char __declspec(dllexport) *DLL_GetArtist(int mid)
  {
     return "";
  }

  unsigned long __declspec(dllexport) DLL_GetLength(int mid)
  { 
    TimiditySong* song = (TimiditySong*)mid;
    return (*(int*)song->song)/48000*1000;
  }
}
