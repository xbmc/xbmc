/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "audio.h"
#include "video.h"
#include "cores/DllLoader/DllLoader.h"
#include "DllMPlayer.h"

extern void* fs_seg;

extern void audio_uninit(int);
extern void video_uninit(void);

int (__cdecl* pInitPlayer)(int argc, char* argvp[]);
int (__cdecl* pOpenFile)(const char*);
int (__cdecl* pProcess)();
int (__cdecl* pCloseFile)();
void (__cdecl* pSetAudioFunctions)(ao_functions_t* pFunctions);
ao_functions_t* (__cdecl* pGetAudioFunctions)();
int (__cdecl* pAudioOutFormatBits)(int);
ao_data_t* (__cdecl* pGetAOData)(void);
void (__cdecl* pSetVideoFunctions)(vo_functions_t*);
void (__cdecl* pMplayerPutKey)(int);
void (__cdecl* pVODrawText)(int dxs, int dys, void (*draw_alpha)(int x0, int y0, int w, int h, unsigned char* src, unsigned char *srca, int stride));
__int64 (__cdecl* pGetPTS)();
BOOL (__cdecl* pHasVideo)();
BOOL (__cdecl* pHasAudio)();
void (__cdecl* pSetCacheSize)(int);
void (__cdecl* pSetCacheBackBuffer)(int);
void (__cdecl* pGetAudioInfo)(char* strFourCC, char* strAudioCodec, long* bitrate, long* samplerate, int* channels, int* bVBR);
void (__cdecl* pGetVideoInfo)(char* strFourCC, char* strVideoCodec, float* fps, unsigned int* iWidth, unsigned int* iHeight, long* tooearly, long* toolate);
void (__cdecl* pGetGeneralInfo)(long* lFramesDropped, int* iQuality, int* iCacheFilled, float* fTotalCorrection, float* fAVDelay);

void (__cdecl* psetAVDelay)(float);
void (__cdecl* psetSubtitleDelay)(float);
void (__cdecl* psetPercentage)(int);
void (__cdecl* psetSubtitle)(int);
void (__cdecl* pshowSubtitle)(int);
void (__cdecl* psetAudioLanguage)(int);
void (__cdecl* psetTime)(int);
void (__cdecl* psetTimeMs)(__int64);
void (__cdecl* pToFFRW)(int);

float (__cdecl* pgetAVDelay)();
float (__cdecl* pgetSubtitleDelay)();
int (__cdecl* pgetPercentage)();
int (__cdecl* pgetSubtitle)();
int (__cdecl* pgetSubtitleCount)();
int (__cdecl* pgetSubtitleVisible)();
int (__cdecl* pgetAudioLanguageCount)();
int (__cdecl* pgetAudioLanguage)();
int (__cdecl* pgetAudioStream)();
int (__cdecl* pgetAudioStreamCount)();
int (__cdecl* pgetAudioStreamInfo)(int iStream, stream_language_t* stream_info);
int (__cdecl* pgetSubtitleStreamInfo)(int iStream, stream_language_t* stream_info);
char* (__cdecl* pgetSubtitleInfo)(int iStream, xbmc_subtitle* sub);
int (__cdecl* pgetTime)();
__int64 (__cdecl* pgetCurrentTime)();
void (__cdecl* pGetCurrentModule)(char* s, int n);
void (__cdecl* pExitPlayer)(char* how);
subtitle* (__cdecl* pGetCurrentSubtitle)();
int (__cdecl* pIsTextSubLoaded)();
void (__cdecl* pSlaveCommand)(const char* s);
int (__cdecl* pGetCacheLevel)();
IUnknown* (__cdecl* pMemAllocatorCreate)() = NULL;

extern "C"
{
  __int64 mplayer_getCurrentTime()
  {
    return pgetCurrentTime();
  }

  void mplayer_ToFFRW(int iSpeed)
  {
    pToFFRW(iSpeed);
  }

  void mplayer_setTime(int iTime)
  {
    psetTime(iTime);
  }

  void mplayer_setTimeMs(__int64 iTime)
  {
    if (psetTimeMs)
      psetTimeMs(iTime);
    else
      psetTime((int)((iTime + 500) / 1000));
  }

  int mplayer_getTime()
  {
    return pgetTime();
  }
  int mplayer_getAudioLanguageCount()
  {
    return pgetAudioLanguageCount();
  }

  int mplayer_getAudioLanguage()
  {
    return pgetAudioLanguage();
  }

  int mplayer_getAudioStream()
  {
    return pgetAudioStream();
  }

  int mplayer_getAudioStreamCount()
  {
    return pgetAudioStreamCount();
  }

  int mplayer_getAudioStreamInfo(int iStream, stream_language_t* stream_info)
  {
    return pgetAudioStreamInfo(iStream, stream_info);
  }

  void mplayer_setAudioLanguage(int iAudioLang)
  {
    psetAudioLanguage(iAudioLang);
  }

  int mplayer_getSubtitle()
  {
    return pgetSubtitle();
  }
  int mplayer_getSubtitleCount()
  {
    return pgetSubtitleCount();
  }

  void mplayer_setSubtitle(int iSubtitle)
  {
    psetSubtitle(iSubtitle);
  }

  void mplayer_showSubtitle(int bOnOff)
  {
    pshowSubtitle(bOnOff);
  }

  int mplayer_SubtitleVisible()
  {
    return pgetSubtitleVisible();
  }

  //Obsolete
  int mplayer_getSubtitleStreamInfo(int iStream, stream_language_t* stream_info)
  {
    return pgetSubtitleStreamInfo(iStream, stream_info);
  }

  char* mplayer_getSubtitleInfo(int iStream, xbmc_subtitle* sub)
  {
    if (pgetSubtitleInfo)
      return pgetSubtitleInfo(iStream, sub);
    else if (pgetSubtitleStreamInfo)
    {
      stream_language_t slt;
      pgetSubtitleStreamInfo(iStream, &slt);
      CLog::Log(LOGNOTICE, "mplayer: falling back to old style subtitle interface");

      sub->id = slt.id;
      sub->invalid = 0;
      sub->type = XBMC_SUBTYPE_STANDARD;

      if (slt.language != 0)
      {
        sub->name[2] = 0;
        sub->name[1] = (slt.language & 255);
        sub->name[0] = (slt.language >> 8);
      }
      else
        sub->name[0] = 0;

      return sub->name;
    }
    return NULL;
  }

  void mplayer_setAVDelay(float fDelay)
  {
    psetAVDelay(fDelay);
  }

  float mplayer_getAVDelay()
  {
    return pgetAVDelay();
  }

  void mplayer_setSubtitleDelay(float fDelay)
  {
    psetSubtitleDelay(fDelay);
  }

  float mplayer_getSubtitleDelay()
  {
    return pgetSubtitleDelay();
  }

  void mplayer_setPercentage(int iPercent)
  {
    psetPercentage(iPercent);
  }

  int mplayer_getPercentage()
  {
    return pgetPercentage();
  }

  void mplayer_GetAudioInfo(char* strFourCC, char* strAudioCodec, long* bitrate, long* samplerate, int* channels, int* bVBR)
  {
    pGetAudioInfo(strFourCC, strAudioCodec, bitrate, samplerate, channels, bVBR);
  }

  void mplayer_GetVideoInfo(char* strFourCC, char* strVideoCodec, float* fps, unsigned int* iWidth, unsigned int* iHeight, long* tooearly, long* toolate)
  {
    pGetVideoInfo(strFourCC, strVideoCodec, fps, iWidth, iHeight, tooearly, toolate);
  }

  void mplayer_GetGeneralInfo(long* lFramesDropped, int* iQuality, int* iCacheFilled, float* fTotalCorrection, float* fAVDelay)
  {
    pGetGeneralInfo(lFramesDropped, iQuality, iCacheFilled, fTotalCorrection, fAVDelay);
  }

  void mplayer_setcache_size(int iCacheSize)
  {
    pSetCacheSize(iCacheSize);
  }

  void mplayer_setcache_backbuffer(int iCacheBackBuffer)
  {
    if (pSetCacheBackBuffer)
      pSetCacheBackBuffer(iCacheBackBuffer);
  }

  BOOL mplayer_HasVideo()
  {
    return pHasVideo();
  }

  BOOL mplayer_HasAudio()
  {
    return pHasAudio();
  }

  void mplayer_put_key(int code)
  {
    pMplayerPutKey(code);
  }

  int mplayer_process()
  {
    int result;
    __asm 
    { /* gcc compiled dll's expect stack to be aligned */     
      mov esi,esp;
      and esp,~15; /* align stack */
      call dword ptr [pProcess];      
      mov esp,esi; /* restore stack */
      mov result, eax;
    }
    return result;
  }

  int mplayer_init(int argc, char* argv[])
  {
    int result;
    __asm 
    { /* gcc compiled dll's expect stack to be aligned */
      mov esi,esp;
      sub esp,8;   /* make room for parameters */
      and esp,~15; /* align stack */
      add esp,8;   /* re-add what we need to push */

      mov eax,dword ptr [argv];
      push eax;
      mov eax,dword ptr [argc];
      push eax;
      call dword ptr [pInitPlayer];
      mov result, eax;
      mov esp,esi; /* restore stack */
    }
    return result;    
  }

  int mplayer_open_file(const char* szFile)
  {
    int result;
    __asm 
    { /* gcc compiled dll's expect stack to be aligned */
      mov esi,esp;
      sub esp,4;   /* make room for parameters */
      and esp,~15; /* align stack */
      add esp,4;   /* re-add what we need to push */

      mov eax,dword ptr [szFile];
      push eax;
      call dword ptr [pOpenFile];
      mov result, eax;
      mov esp,esi; /* restore stack */
    }

    return result;
  }

  int mplayer_close_file()
  {
    int hr = pCloseFile();
    audio_uninit(1); //Fix to make sure audio device is deleted
    //    video_uninit(); //Fix to make sure audio device is deleted

    // Free allocated memory for FS segment
    if (fs_seg != NULL)
    {
      CLog::Log(LOGDEBUG, "Freeing FS segment @ 0x%x", fs_seg);
      free(fs_seg);
      fs_seg = NULL;
    }

    return hr;
  }


  ao_data_t* GetAOData()
  {
    return pGetAOData();
  }

  int audio_out_format_bits(int format)
  {
    return pAudioOutFormatBits(format);
  }

  void vo_draw_text(int dxs, int dys, void (*mydrawalpha)(int x0, int y0, int w, int h, unsigned char* src, unsigned char *srca, int stride))
  {
    pVODrawText(dxs, dys, mydrawalpha);
  }

  __int64 mplayer_get_pts()
  {
    return pGetPTS();
  }

  void mplayer_get_current_module(char* s, int n)
  {
    if (pGetCurrentModule)
      pGetCurrentModule(s, n);
    else
      strcpy(s, "unknown");
  }

  void mplayer_exit_player(void)
  {
    if (pExitPlayer)
      pExitPlayer("");
  }

  subtitle* mplayer_GetCurrentSubtitle(void)
  {
    if (pGetCurrentSubtitle)
      return pGetCurrentSubtitle();
    else
      return NULL;
  }

  bool mplayer_isTextSubLoaded(void)
  {
    if (pIsTextSubLoaded)
    {
      return (pIsTextSubLoaded() != 0);
    }
    else
    {
      return false;
    }
  }

  void mplayer_SlaveCommand(const char * s, ... )
  {
    if (pSlaveCommand)
    {
      va_list va;
      va_start(va, s);
      int size = _vscprintf(s, va);
      char *buffer = new char[size+1];
      _vsnprintf(buffer,size+1, s, va);
      va_end(va);

      pSlaveCommand(buffer);
      delete[] buffer;
    }
  }

  int mplayer_GetCacheLevel()
  {
    if (pGetCacheLevel)
      return pGetCacheLevel();
    else
      return -1;
  }

  IUnknown* mplayer_MemAllocatorCreate()
  {
    if(pMemAllocatorCreate)
      return pMemAllocatorCreate();
    else
      return NULL;
  }

  void mplayer_load_dll(DllLoader& dll)
  {
    dll.ResolveExport("audio_out_format_bits", (void**)&pAudioOutFormatBits);
    dll.ResolveExport("SetVideoFunctions", (void**)&pSetVideoFunctions);
    dll.ResolveExport("GetAOData", (void**)&pGetAOData);
    dll.ResolveExport("SetAudioFunctions", (void**)&pSetAudioFunctions);
    dll.ResolveExport("mplayer_init", (void**)&pInitPlayer);
    dll.ResolveExport("mplayer_open_file", (void**)&pOpenFile);
    dll.ResolveExport("mplayer_process", (void**)&pProcess);
    dll.ResolveExport("mplayer_close_file", (void**)&pCloseFile);
    dll.ResolveExport("mplayer_put_key", (void**)&pMplayerPutKey);
    dll.ResolveExport("vo_draw_text", (void**)&pVODrawText);
    dll.ResolveExport("mplayer_get_pts", (void**)&pGetPTS);
    dll.ResolveExport("mplayer_HasVideo", (void**)&pHasVideo);
    dll.ResolveExport("mplayer_HasAudio", (void**)&pHasAudio);
    dll.ResolveExport("mplayer_setcache_size", (void**)&pSetCacheSize);
    dll.ResolveExport("mplayer_setcache_backbuffer", (void**)&pSetCacheBackBuffer);
    dll.ResolveExport("mplayer_GetAudioInfo", (void**)&pGetAudioInfo);
    dll.ResolveExport("mplayer_GetVideoInfo", (void**)&pGetVideoInfo);
    dll.ResolveExport("mplayer_GetGeneralInfo", (void**)&pGetGeneralInfo);
    dll.ResolveExport("mplayer_setAVDelay", (void**)&psetAVDelay);
    dll.ResolveExport("mplayer_setSubtitleDelay", (void**)&psetSubtitleDelay);
    dll.ResolveExport("mplayer_setPercentage", (void**)&psetPercentage);
    dll.ResolveExport("mplayer_getAVDelay", (void**)&pgetAVDelay);
    dll.ResolveExport("mplayer_getSubtitleDelay", (void**)&pgetSubtitleDelay);
    dll.ResolveExport("mplayer_getPercentage", (void**)&pgetPercentage);
    dll.ResolveExport("mplayer_getSubtitle", (void**)&pgetSubtitle);
    dll.ResolveExport("mplayer_getSubtitleCount", (void**)&pgetSubtitleCount);
    dll.ResolveExport("mplayer_getSubtitleInfo", (void**)&pgetSubtitleInfo);
    if (!pgetSubtitleInfo) //New interface missing. Resolve oldstyle
      dll.ResolveExport("mplayer_getSubtitleStreamInfo", (void**)&pgetSubtitleStreamInfo);
    dll.ResolveExport("mplayer_SubtitleVisible", (void**)&pgetSubtitleVisible);
    dll.ResolveExport("mplayer_setPercentage", (void**)&psetPercentage);
    dll.ResolveExport("mplayer_setSubtitle", (void**)&psetSubtitle);
    dll.ResolveExport("mplayer_showSubtitle", (void**)&pshowSubtitle);
    dll.ResolveExport("mplayer_getAudioLanguageCount", (void**)&pgetAudioLanguageCount);
    dll.ResolveExport("mplayer_getAudioLanguage", (void**)&pgetAudioLanguage);
    dll.ResolveExport("mplayer_getAudioStream", (void**)&pgetAudioStream);
    dll.ResolveExport("mplayer_getAudioStreamCount", (void**)&pgetAudioStreamCount);
    dll.ResolveExport("mplayer_getAudioStreamInfo", (void**)&pgetAudioStreamInfo);
    dll.ResolveExport("mplayer_setAudioLanguage", (void**)&psetAudioLanguage);
    dll.ResolveExport("mplayer_setTime", (void**)&psetTime);
    dll.ResolveExport("mplayer_setTimeMs", (void**)&psetTimeMs);
    dll.ResolveExport("mplayer_getTime", (void**)&pgetTime);
    dll.ResolveExport("mplayer_ToFFRW", (void**)&pToFFRW);
    dll.ResolveExport("mplayer_getCurrentTime", (void**)&pgetCurrentTime);
    dll.ResolveExport("mplayer_get_current_module", (void**)&pGetCurrentModule);
    dll.ResolveExport("exit_player", (void**)&pExitPlayer);
    dll.ResolveExport("mplayer_getCurrentSubtitle", (void**) &pGetCurrentSubtitle);
    dll.ResolveExport("mplayer_isTextSubLoaded", (void**) &pIsTextSubLoaded);
    dll.ResolveExport("mplayer_SlaveCommand", (void**) &pSlaveCommand);
    dll.ResolveExport("mplayer_GetCacheLevel", (void**) &pGetCacheLevel);    
    dll.ResolveExport("MemAllocatorCreate", (void**) &pMemAllocatorCreate);

    pSetVideoFunctions(&video_functions);
    pSetAudioFunctions(&audio_functions);
  }
};
