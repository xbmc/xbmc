#pragma once

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

class DllLoader;

#ifdef __cplusplus
extern "C"
{
#endif


#define SUB_MAX_TEXT 10
#define SUB_ALIGNMENT_HLEFT 1
#define SUB_ALIGNMENT_HCENTER 0
#define SUB_ALIGNMENT_HRIGHT 2


#define MAX_XBMC_NAME 10
#define XBMC_SUBTYPE_STANDARD 1
#define XBMC_SUBTYPE_VOBSUB 2
#define XBMC_SUBTYPE_DVDSUB 3
#define XBMC_SUBTYPE_OGGSUB 4
#define XBMC_SUBTYPE_MKVSUB 5

  typedef struct
  {
    int id; //The internal id of a subtitle of a certain type.
    char name[MAX_XBMC_NAME]; //Name as given by the file playing
    char* desc; //Possibly some descriptive data, not used at the moment.
    int type; //Subtitle type
    int invalid; //Will be set if this subtitle has been invalidated for some reason. can't be used
  }
  xbmc_subtitle;

  typedef struct
  {
    int id; // 0 - 31 mpeg; 128 - 159 ac3; 160 - 191 pcm
    int language;
    int type;
    int channels;
  }
  stream_language_t;


  typedef struct
  {
    int lines;
    unsigned long start;
    unsigned long end;
    char *text[SUB_MAX_TEXT];
    unsigned char alignment;
  }
  subtitle;

  void mplayer_load_dll(DllLoader& dll);
  void mplayer_put_key(int code);
  int mplayer_process();
  int mplayer_open_file(const char* szFileName);
  int mplayer_close_file();
  int mplayer_init(int argc, char* argv[]);
  BOOL mplayer_HasVideo();
  BOOL mplayer_HasAudio();
  void mplayer_setcache_size(int iCacheSize);
  void mplayer_setcache_backbuffer(int buffer_size); // back buffer size in %
  __int64 mplayer_get_pts();

  void mplayer_GetAudioInfo(char* strFourCC, char* strAudioCodec, long* bitrate, long* samplerate, int* channels, int* bVBR);
  void mplayer_GetVideoInfo(char* strFourCC, char* strVideoCodec, float* fps, unsigned int* iWidth, unsigned int* iHeight, long* tooearly, long* toolate);
  void mplayer_GetGeneralInfo(long* lFramesDropped, int* iQuality, int* iCacheFilled, float* fTotalCorrection, float* fAVDelay);
  void mplayer_setAVDelay(float fDelay);
  float mplayer_getAVDelay();
  void mplayer_setSubtitleDelay(float fDelay);
  float mplayer_getSubtitleDelay();
  void mplayer_setPercentage(int iPercent);
  int mplayer_getPercentage();
  int mplayer_getSubtitle();
  int mplayer_getSubtitleCount();
  void mplayer_setSubtitle(int iSubtitle);

  void mplayer_showSubtitle(int bOnOff);
  int mplayer_SubtitleVisible();
  int mplayer_getAudioLanguageCount();
  int mplayer_getAudioLanguage();
  int mplayer_getAudioStream(); //Currently playing stream, 0 based not aid
  int mplayer_getAudioStreamCount(); //Number of available streams
  int mplayer_getAudioStreamInfo(int iStream, stream_language_t* stream_info); //More info on a specified stream, 0 based not aid
  int mplayer_getSubtitleStreamInfo(int iStream, stream_language_t* stream_info);
  char* mplayer_getSubtitleInfo(int iStream, xbmc_subtitle* sub);
  void mplayer_setAudioLanguage(int iAudioLang);
  void mplayer_setTime(int iTime);
  void mplayer_setTimeMs(__int64 iTime);
  int mplayer_getTime();
  __int64 mplayer_getCurrentTime();
  void mplayer_ToFFRW(int iSpeed);
  void mplayer_setVolume(long nVolume);
  void mplayer_setDRC(long drc);
  int mplayer_getVolume();
  void mplayer_get_current_module(char* s, int n);
  void mplayer_exit_player(void);
  subtitle* mplayer_GetCurrentSubtitle(void);
  bool mplayer_isTextSubLoaded(void);
  void mplayer_SlaveCommand(const char* s, ... );
  int mplayer_GetCacheLevel();

  IUnknown* mplayer_MemAllocatorCreate();

#ifdef __cplusplus
};
#endif
