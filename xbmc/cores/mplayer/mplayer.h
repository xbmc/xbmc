#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "../DllLoader/dll.h"

void		mplayer_load_dll(DllLoader& dll);
void		mplayer_put_key(int code);
int			mplayer_process();
int			mplayer_open_file(const char* szFileName);
int			mplayer_close_file();
int			mplayer_init(int argc,char* argv[]);
BOOL		mplayer_HasVideo();
BOOL		mplayer_HasAudio();
void		yv12toyuy2(const unsigned char *ysrc, const unsigned char *usrc, const unsigned char *vsrc, unsigned char *dst,unsigned int width, unsigned int height,int lumStride, int chromStride, int dstStride);
void		mplayer_setcache_size(int iCacheSize);
__int64 mplayer_get_pts();

void		mplayer_GetAudioInfo(char* strFourCC,char* strAudioCodec, long* bitrate, long* samplerate, int* channels, int* bVBR);
void		mplayer_GetVideoInfo(char* strFourCC,char* strVideoCodec, float* fps, unsigned int* iWidth,unsigned int* iHeight, long* tooearly, long* toolate);
void		mplayer_GetGeneralInfo(long* lFramesDropped, int* iQuality, int* iCacheFilled, float* fTotalCorrection, float* fAVDelay);
void    mplayer_setAVDelay(float fDelay);
float   mplayer_getAVDelay();
void    mplayer_setSubtitleDelay(float fDelay);
float   mplayer_getSubtitleDelay();
void    mplayer_setPercentage(int iPercent);
int     mplayer_getPercentage();
void    mplayer_showosd(bool bonoff);
int     mplayer_getSubtitle();
int     mplayer_getSubtitleCount();
void    mplayer_setSubtitle(int iSubtitle);

void    mplayer_showSubtitle(int bOnOff);
int     mplayer_SubtitleVisible();
int     mplayer_getAudioLanguageCount();
int     mplayer_getAudioLanguage();
int     mplayer_getAudioStream(int iStream);
int     mplayer_getAudioStreamCount();
void    mplayer_setAudioLanguage(int iAudioLang);
void    mplayer_setTime(int iTime);
int     mplayer_getTime();
__int64 mplayer_getCurrentTime();
void    mplayer_ToFFRW(int iSpeed);
#ifdef __cplusplus
}
#endif
