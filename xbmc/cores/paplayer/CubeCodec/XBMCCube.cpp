#ifndef _LINUX
#include <windows.h>
#else
#include <memory.h>
#define __cdecl
#define __declspec(x)
#endif

extern "C"
{
  #include "cube.h"
  
  struct ADXSong
  {
    CUBEFILE cubefile;
    char m_szSample_buffer[4096*2*2]; // sample buffer
    char* m_szStartOfBuffer;
    int m_iStartOfBuffer;
    int m_iDataInBuffer;
    int m_iDataStart;
  };

  void __cdecl DisplayError (char * Message, ...)
  {
  }

    int looptimes = 2;
    int fadelength = 10;
    int fadedelay = 0;

  int __declspec(dllexport) DLL_Init()
  {
    return 1;
  }

  int get_4096_samples(short *buf, CUBEFILE* cubefile)
  {
    int i;

    for (i=0;i<4096;i++) {
      if (cubefile->ch[0].readloc == cubefile->ch[0].writeloc) {
        fillbuffers(cubefile);
        //if (cubefile.ch[0].readloc != cubefile.ch[0].writeloc) return i*cubefile.NCH*(BPS/8);
      }
      buf[i*cubefile->NCH]=cubefile->ch[0].chanbuf[cubefile->ch[0].readloc++];
      if (cubefile->NCH==2) buf[i*cubefile->NCH+1]=cubefile->ch[1].chanbuf[cubefile->ch[1].readloc++];
      if (cubefile->ch[0].readloc>=0x8000/8*14) cubefile->ch[0].readloc=0;
      if (cubefile->ch[1].readloc>=0x8000/8*14) cubefile->ch[1].readloc=0;
    }
    return 4096*cubefile->NCH*(16/8);
  }

  int skip_4096_samples(CUBEFILE* cubefile)
  {
    int i;
    for (i=0;i<4096;i++) {
      if (cubefile->ch[0].readloc == cubefile->ch[0].writeloc) {
        fillbuffers(cubefile);
        //if (cubefile.ch[0].readloc != cubefile.ch[0].writeloc) return i*cubefile.NCH*(BPS/8);
      }
      cubefile->ch[0].readloc++;
      if (cubefile->NCH==2) cubefile->ch[1].readloc++;
      if (cubefile->ch[0].readloc>=0x8000/8*14) cubefile->ch[0].readloc=0;
      if (cubefile->ch[1].readloc>=0x8000/8*14) cubefile->ch[1].readloc=0;
    }
    return 4096*cubefile->NCH*(16/8);
  }

  long __declspec(dllexport) DLL_LoadADX(const char* szFileName, int* sampleRate, int* sampleSize, int* channels)
  {
    ADXSong* result = new ADXSong;

    if (InitCUBEFILE(const_cast<char*>(szFileName), &result->cubefile))
    {
      delete result;
      return 0;
    }

    result->m_iDataInBuffer = 0;
    result->m_iStartOfBuffer = -1;
    result->m_szStartOfBuffer = result->m_szSample_buffer+4096*4;
    BASE_VOL=0x2000;
    *sampleRate = result->cubefile.ch[0].sample_rate;
    *sampleSize = 16;
    *channels = result->cubefile.NCH;
    result->m_iDataStart = ftell(result->cubefile.ch[0].infile);
    return (long)result;
  }

  void __declspec(dllexport) DLL_FreeADX(int adx)
  {
    ADXSong* song = (ADXSong*)adx;	
    
    CloseCUBEFILE(&song->cubefile);
    delete (ADXSong*)adx;
  }

  int __declspec(dllexport) DLL_FillBuffer(int adx, char* szBuffer, int iSize)
  {
    ADXSong* song = (ADXSong*)adx;
    
    int iCurrSize = iSize;
    while (iCurrSize > 0)
    {
      if (!song->m_iDataInBuffer)
      {
        song->m_iDataInBuffer = get_4096_samples((short*)song->m_szSample_buffer,&song->cubefile);
        if (!song->m_iDataInBuffer)
        {
          return iSize-iCurrSize;
        }
         
        if (song->m_iStartOfBuffer == -1)
          song->m_iStartOfBuffer = 0;
        else
          song->m_iStartOfBuffer += 4096*4;
        song->m_szStartOfBuffer = song->m_szSample_buffer;
      }
      int iCopy=0;
      if (iCurrSize > song->m_iDataInBuffer)
        iCopy = song->m_iDataInBuffer;
      else
        iCopy = iCurrSize;

      memcpy(szBuffer,song->m_szStartOfBuffer,iCopy);
      szBuffer += iCopy;
      iCurrSize -= iCopy;
      song->m_szStartOfBuffer += iCopy;
      song->m_iDataInBuffer -= iCopy;
    }
    
    return iSize-iCurrSize;
  }

  unsigned long __declspec(dllexport) DLL_Seek(int adx, unsigned long timepos)
  {
    ADXSong* song = (ADXSong*)adx;
  
    int iCurrSize = timepos/1000*song->cubefile.NCH*song->cubefile.ch[0].sample_rate*2;
    if (timepos/1000*song->cubefile.NCH*song->cubefile.ch[0].sample_rate*2 < (unsigned)song->m_iStartOfBuffer)
    {
      fseek(song->cubefile.ch[0].infile,song->m_iDataStart,SEEK_SET);
      if (song->cubefile.ch[1].infile) fseek(song->cubefile.ch[1].infile,song->m_iDataStart,SEEK_SET);
      song->m_iStartOfBuffer = -2;
      song->m_iDataInBuffer = 0;
    }
    else
      iCurrSize -= song->m_iStartOfBuffer;

    if (iCurrSize < 4096*4 && song->m_iStartOfBuffer == 0)
    {
      song->m_iDataInBuffer = 4096*4-iCurrSize;
      song->m_szStartOfBuffer = song->m_szSample_buffer+iCurrSize;
      iCurrSize = 0;
    }
    else
      song->m_iStartOfBuffer = -4096*4;
    
    while (iCurrSize > 0)
    {
      song->m_iDataInBuffer = get_4096_samples((short*)song->m_szSample_buffer,&song->cubefile);
      song->m_szStartOfBuffer = song->m_szSample_buffer;
      song->m_iStartOfBuffer += 4096*4;
      if (song->m_iDataInBuffer == 0)
      {
        return song->m_iStartOfBuffer/(song->cubefile.ch[0].sample_rate*song->cubefile.NCH*2);
      }

      iCurrSize -= song->m_iDataInBuffer;
      if (iCurrSize < 4096*4)
      {
        song->m_iDataInBuffer -= iCurrSize;
        song->m_szStartOfBuffer += iCurrSize;
        iCurrSize = 0;
      }
    }

    return timepos;
  }

  const char __declspec(dllexport) *DLL_GetTitle(int adx)
  {
    return "";     
  }

  const char __declspec(dllexport) *DLL_GetArtist(int adx)
  {
    return "";
  }

  unsigned long __declspec(dllexport) DLL_GetLength(int adx)
  {
    ADXSong* song = (ADXSong*)adx;

    return song->cubefile.nrsamples/song->cubefile.ch[0].sample_rate*1000;
  }
}
