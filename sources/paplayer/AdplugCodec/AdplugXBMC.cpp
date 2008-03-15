#include "adplug.h"
#include "emuopl.h"

struct ADLSong
{
  CPlayer* player;
  CEmuopl* opl;
  char* szBuf;
  char* szStartOfBuf;
  int bufLen;
};

#ifdef __linux__
#define __declspec(dllexport)
#endif

extern "C"
{
  int __declspec(dllexport) DLL_Init()
  {
    return 1;
  }

  int __declspec(dllexport) DLL_LoadADL(const char* szFileName)
  {
    ADLSong* result = new ADLSong;
    result->opl = new CEmuopl(48000,true,true);
    result->opl->init();
    if (result->player = CAdPlug::factory(szFileName,result->opl))
    {
      result->bufLen = 48000/result->player->getrefresh()*4;
      result->szBuf = new char[result->bufLen];
      result->szStartOfBuf = result->szBuf;
      return (int)result;
    }

    return NULL;
  }

  void __declspec(dllexport) DLL_FreeADL(int adl)
  {
    ADLSong* song = (ADLSong*)adl;
    delete song->player;
    delete song->opl;
    delete song->szBuf;
    delete song;
  }

  int __declspec(dllexport) DLL_FillBuffer(int adl, char* szBuffer, int iSize)
  {
    ADLSong* song = (ADLSong*)adl;    
    int iCurrSize = iSize;
    while (iCurrSize > 0)
    {
      if (song->szStartOfBuf >= song->szBuf+song->bufLen)
      {
        if (!song->player->update())
          return -1;
          
        song->opl->update((short*)song->szBuf,song->bufLen/4);
        song->szStartOfBuf = song->szBuf;
      }
      int iCopy=0;
      if (iCurrSize > song->szBuf+song->bufLen-song->szStartOfBuf)
        iCopy = song->szBuf+song->bufLen-song->szStartOfBuf;
      else
        iCopy = iCurrSize;

      memcpy(szBuffer,song->szStartOfBuf,iCopy);
      szBuffer += iCopy;
      iCurrSize -= iCopy;
      song->szStartOfBuf += iCopy;
    }

    return iSize-iCurrSize;
  }

  unsigned long __declspec(dllexport) DLL_Seek(int adl, unsigned long iTimePos)
  {
     ADLSong* song = (ADLSong*)adl;
     song->player->rewind();
     song->player->seek(iTimePos); 
     song->szStartOfBuf = song->szBuf+song->bufLen;

     return iTimePos;
  }

  const char __declspec(dllexport) *DLL_GetTitle(int adl)
  {
     ADLSong* song = (ADLSong*)adl;

     printf("title %s",song->player->gettitle());
     return song->player->gettitle();
  }

  const char __declspec(dllexport) *DLL_GetArtist(int adl)
  {
     ADLSong* song = (ADLSong*)adl;

     printf("artist %s",song->player->getauthor());
     return (const char*)song->player->getauthor();
  }

  unsigned long __declspec(dllexport) DLL_GetLength(int adl)
  {
    ADLSong* song = (ADLSong*)adl;

    return song->player->songlength();
  }
}
