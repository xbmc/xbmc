#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#ifndef _LINUX
#include <windows.h>
#else
#include "PlatformInclude.h"
#undef __declspec
#define __declspec(x)
#endif

#include "gym_play.h"
#include "ym2612.h"
#include "psg.h"

HANDLE hMutex = NULL;

extern "C"
{
  extern struct ym2612__ YM2612;
  extern struct _psg PSG;

  struct GYMSong
  {
    unsigned char* gym; // this is the one alloced
    unsigned char* gymStart;
    unsigned char* gymPos;
    unsigned int gymSize;
    struct GYMTAG
    {
      char gym_id[4];
      char song_title[32];
      char game_title[32];
      char game_publisher[32];
      char dumper_emu[32];
      char dumper_person[32];
      char comments[256];
      unsigned int looped;
      unsigned int compressed;
    }; 
    GYMTAG* gymTag;
    __int64 iLength;
    int Seg_L[1600], Seg_R[1600];
    struct ym2612__ YM2612;
    struct _psg PSG;
  };

  __int64 calc_gym_time_length(GYMSong* gym)
  {
    if (gym->gymStart == 0 || gym->gymSize == 0)
      return 0;

    unsigned int loop, num_zeros = 0;

    for(loop = 0; loop < gym->gymSize; loop++)
    {
      switch(gym->gymStart[loop])
      {
        case(0x00):
          num_zeros++;
          continue;
        case(0x01):
          loop += 2;
          continue;
        case(0x02):
          loop += 2;
          continue;
        case(0x03):
          loop += 1;
          continue;
      }
    }

    return (__int64)(num_zeros)*1000/60;
  }
  
  int __declspec(dllexport) DLL_Init()
  {
    if (!hMutex)
    {
      hMutex = CreateMutex(NULL,true,NULL);
      YM2612_Enable = true;
      YM2612_Improv = true;

      Chan_Enable[0] = true;
      Chan_Enable[1] = true;
      Chan_Enable[2] = true;
      Chan_Enable[3] = true;
      Chan_Enable[4] = true;
      Chan_Enable[5] = true;
      DAC_Enable = true;

      PSG_Enable = true;
      PSG_Improv = true;

      PSG_Chan_Enable[0] = true;
      PSG_Chan_Enable[1] = true;
      PSG_Chan_Enable[2] = true;
      PSG_Chan_Enable[3] = true;

      ReleaseMutex(hMutex);
    }
    return (int) ceil(48000/60.0) << 2;
  }

  void __declspec(dllexport) DLL_DeInit()
  {
  }

  long __declspec(dllexport) DLL_LoadGYM(const char *szFileName)
  {    
    GYMSong* result = new GYMSong;
    FILE* f = fopen(szFileName,"rb");
    int iResult = 0;
    if (f)
    {
      WaitForSingleObject(hMutex,INFINITE);
      fseek(f,0,SEEK_END);
      result->gymSize = ftell(f);
      fseek(f,0,SEEK_SET);
      result->gym = (unsigned char*)malloc(result->gymSize*sizeof(unsigned char));
      unsigned int iRead = 0;
      result->gymPos = result->gym;
      while (iRead < result->gymSize)
      {
        if (fread(result->gym,1,result->gymSize,f) != 1)
          break;

        int iCurrRead = fread(result->gymPos,1,16384,f);
        if (iCurrRead > 0)
        {
          iRead += iCurrRead;
          ReleaseMutex(hMutex);
          Sleep(10); // prevent starving pap during xfade
          WaitForSingleObject(hMutex,INFINITE);
        }
        else
          break;
      }

      fclose(f);
      result->gymTag = (GYMSong::GYMTAG*)result->gym;
      if (strncmp(((GYMSong::GYMTAG*)result->gym)->gym_id, "GYMX", 4) == 0)
      {
        result->gymStart = result->gymPos = result->gym+sizeof(GYMSong::GYMTAG);
        result->gymSize -= sizeof(GYMSong::GYMTAG);
        result->iLength = calc_gym_time_length(result);
      }
      else
      {
        result->gymStart = result->gymPos = result->gym;
        result->gymTag = NULL;
      }

      
      result->YM2612 = YM2612;
      result->PSG = PSG;
      Start_Play_GYM(48000);
      iResult = (long)result;
      ReleaseMutex(hMutex);
    }

    return (int)iResult;
  }

  void __declspec(dllexport) DLL_FreeGYM(int gym)
  {
    GYMSong* song = (GYMSong*)gym;
    free(song->gym);
    free(song);
  }
  
  int __declspec(dllexport) DLL_FillBuffer(int gym, char* szBuffer)
  {
    WaitForSingleObject(hMutex,INFINITE);
    GYMSong* song = (GYMSong*)gym;
    Seg_L = song->Seg_L;
    Seg_R = song->Seg_R;
    YM2612 = song->YM2612;
    PSG = song->PSG;
    song->gymPos = Play_GYM(szBuffer,song->gymStart,song->gymPos,song->gymSize,0);
    song->YM2612 = YM2612;
    song->PSG = PSG;
    ReleaseMutex(hMutex);
    if (!song->gymPos)
      return 0;

    return 1;
  }

  void __declspec(dllexport) DLL_Seek(int gym, unsigned int iPos)
  {
    WaitForSingleObject(hMutex,INFINITE);
    GYMSong* song = (GYMSong*)gym;
    Seg_L = song->Seg_L;
    Seg_R = song->Seg_R;
    YM2612 = song->YM2612;
    PSG = song->PSG;
    jump_gym_time_pos(song->gymStart,song->gymSize,iPos);
    song->YM2612 = YM2612;
    song->PSG = PSG;
    ReleaseMutex(hMutex);
  }

  long __declspec(dllexport) DLL_GetArtist(int gym)
  {
    GYMSong* song = (GYMSong*)gym;
    if (song->gymTag)
      return (long)song->gymTag->game_publisher;
    
    return 0;
  }
  
  long __declspec(dllexport) DLL_GetTitle(int gym)
  {
    GYMSong* song = (GYMSong*)gym;
    if (song->gymTag)
      return (long)song->gymTag->song_title;
    
    return 0;
  }

  __int64 __declspec(dllexport) DLL_GetLength(int gym)
  {
    GYMSong* song = (GYMSong*)gym;
    return song->iLength;
  }
}
