#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "../DllLoader/dll.h"

typedef	struct {
 int id; //	0	-	31 mpeg; 128 - 159 ac3;	160	-	191	pcm
 int language; 
 int type;
 int channels;
}	stream_language_t;

void		mplayer_load_dll(DllLoader&	dll);
void		mplayer_put_key(int	code);
int			mplayer_process();
int			mplayer_open_file(const	char*	szFileName);
int			mplayer_close_file();
int			mplayer_init(int argc,char*	argv[]);
BOOL		mplayer_HasVideo();
BOOL		mplayer_HasAudio();
void		mplayer_setcache_size(int	iCacheSize);
__int64	mplayer_get_pts();

void		mplayer_GetAudioInfo(char* strFourCC,char* strAudioCodec,	long*	bitrate, long* samplerate, int*	channels,	int* bVBR);
void		mplayer_GetVideoInfo(char* strFourCC,char* strVideoCodec,	float* fps,	unsigned int*	iWidth,unsigned	int* iHeight,	long*	tooearly,	long*	toolate);
void		mplayer_GetGeneralInfo(long* lFramesDropped, int*	iQuality,	int* iCacheFilled, float*	fTotalCorrection,	float* fAVDelay);
void		mplayer_setAVDelay(float fDelay);
float		mplayer_getAVDelay();
void		mplayer_setSubtitleDelay(float fDelay);
float		mplayer_getSubtitleDelay();
void		mplayer_setPercentage(int	iPercent);
int			mplayer_getPercentage();
void		mplayer_showosd(int	bonoff);
int			mplayer_getSubtitle();
int			mplayer_getSubtitleCount();
void		mplayer_setSubtitle(int	iSubtitle);

void		mplayer_showSubtitle(int bOnOff);
int			mplayer_SubtitleVisible();
int			mplayer_getAudioLanguageCount();
int			mplayer_getAudioLanguage();
int			mplayer_getAudioStream();	//Currently	playing	stream,	0	based	not	aid
int			mplayer_getAudioStreamCount(); //Number	of available streams
int			mplayer_getAudioStreamInfo(int iStream,	stream_language_t* stream_info); //More	info on	a	specified	stream,	0	based	not	aid
int			mplayer_getSubtitleStreamInfo(int	iStream, stream_language_t*	stream_info);
void		mplayer_setAudioLanguage(int iAudioLang);
void		mplayer_setTime(int	iTime);
int			mplayer_getTime();
__int64	mplayer_getCurrentTime();
void		mplayer_ToFFRW(int iSpeed);
void		mplayer_setVolume(int	iPercentage);
int			mplayer_getVolume();
void		mplayer_get_current_module(char* s, int n);

#ifdef __cplusplus
}
#endif
