#include "stdafx.h"
#include "audio.h"
#include "video.h"
#include "../DllLoader/dll.h"
#include "mplayer.h"

#include <vector>
#include "../../utils/log.h"
extern void* fs_seg;
extern vector<DllLoader *> m_vecDlls;

//deal wma, and wmv dlls in openfile, and closefile functions
extern DllLoader * wmaDMOdll;
extern DllLoader * wmvDMOdll;
extern DllLoader * wmsDMOdll;
extern "C" BOOL WINAPI dllFreeLibrary(HINSTANCE hLibModule);

int					(__cdecl*	pInitPlayer)(int argc, char* argvp[]);
int					(__cdecl*	pOpenFile)(const char*);
int					(__cdecl*	pProcess)();
int					(__cdecl*	pCloseFile)();
void				(__cdecl*	pSetAudioFunctions)(ao_functions_t*	pFunctions);
ao_functions_t*	(__cdecl*	pGetAudioFunctions)();
int						(__cdecl*	pAudioOutFormatBits)(int);
ao_data_t*		(__cdecl*	pGetAOData)(void);
void					(__cdecl*	pSetVideoFunctions)(vo_functions_t*);
void					(__cdecl*	pMplayerPutKey)(int);
void					(__cdecl*	pVODrawText)(int dxs,int dys,void	(*draw_alpha)(int	x0,int y0, int w,int h,	unsigned char* src,	unsigned char	*srca, int stride));
void					(__cdecl*	pAspectSaveScreenres)(int	scrw,	int	scrh);
void					(__cdecl*	pAspectSavePrescale)(int scrw, int scrh);
void					(__cdecl*	pAspectSaveOrig)(int scrw, int scrh);
void					(__cdecl*	pAspect)(unsigned	int*,	unsigned int*, int);
void					(__cdecl*	pVODrawAlphayv12)(int	w,int	h, unsigned	char*	src, unsigned	char *srca,	int	srcstride, unsigned	char*	dstbase,int	dststride);
void					(__cdecl*	pVODrawAlphayuy2)(int	w,int	h, unsigned	char*	src, unsigned	char *srca,	int	srcstride, unsigned	char*	dstbase,int	dststride);
void					(__cdecl*	pVODrawAlphargb24)(int w,int h,	unsigned char* src,	unsigned char	*srca, int srcstride,	unsigned char* dstbase,int dststride);
void					(__cdecl*	pVODrawAlphargb32)(int w,int h,	unsigned char* src,	unsigned char	*srca, int srcstride,	unsigned char* dstbase,int dststride);
void					(__cdecl*	pVODrawAlphargb15)(int w,int h,	unsigned char* src,	unsigned char	*srca, int srcstride,	unsigned char* dstbase,int dststride);
void					(__cdecl*	pVODrawAlphargb16)(int w,int h,	unsigned char* src,	unsigned char	*srca, int srcstride,	unsigned char* dstbase,int dststride);
__int64				(__cdecl*	pGetPTS)();
BOOL					(__cdecl*	pHasVideo)();
BOOL					(__cdecl*	pHasAudio)();
int						(__cdecl*	pImageOutput)(IMAGE	*	image, unsigned	int	width,int	height,unsigned	int	edged_width, unsigned	char * dst[4], unsigned	int	dst_stride[4],int	csp,int	interlaced);
void					(__cdecl*	pInitColorConversions)();
void					(__cdecl*	pSetCacheSize)(int);
void					(__cdecl*	pSetCacheBackBuffer)(int);
void					(__cdecl*	pGetAudioInfo)(char* strFourCC,char* strAudioCodec,	long*	bitrate, long* samplerate, int*	channels,	int* bVBR);
void					(__cdecl*	pGetVideoInfo)(char* strFourCC,char* strVideoCodec,	float* fps,	unsigned int*	iWidth,unsigned	int* iHeight,	long*	tooearly,	long*	toolate);
void					(__cdecl*	pGetGeneralInfo)(long* lFramesDropped, int*	iQuality,	int* iCacheFilled, float*	fTotalCorrection,	float* fAVDelay);

void					(__cdecl*	psetAVDelay)(float);
void					(__cdecl*	psetSubtitleDelay)(float);
void					(__cdecl*	psetPercentage)(int);
void					(__cdecl*	psetSubtitle)(int);
void					(__cdecl*	pshowSubtitle)(int);
void					(__cdecl*	psetAudioLanguage)(int);
void					(__cdecl*	psetTime)(int);
void					(__cdecl*	pToFFRW)(int);

float					(__cdecl*	pgetAVDelay)();
float					(__cdecl*	pgetSubtitleDelay)();
int						(__cdecl*	pgetPercentage)();
int						(__cdecl*	pgetSubtitle)();
int						(__cdecl*	pgetSubtitleCount)();
int						(__cdecl*	pgetSubtitleVisible)();
int						(__cdecl*	pgetAudioLanguageCount)();
int						(__cdecl*	pgetAudioLanguage)();
int						(__cdecl*	pgetAudioStream)();
int						(__cdecl*	pgetAudioStreamCount)();
int						(__cdecl*	pgetAudioStreamInfo)(int iStream,	stream_language_t* stream_info);
int						(__cdecl*	pgetSubtitleStreamInfo)(int	iStream, stream_language_t*	stream_info);
int						(__cdecl*	pgetTime)();
__int64				(__cdecl*	pgetCurrentTime)();
void					(__cdecl*	pShowOSD)(int);
void					(__cdecl* pGetCurrentModule)(char* s, int n);
void                     (__cdecl* pExitPlayer)(char* how);

extern "C" 
{
	void mplayer_showosd(int bonoff)
	{
		return pShowOSD(bonoff);
	}
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

	int mplayer_getSubtitleStreamInfo(int iStream, stream_language_t* stream_info)
	{
		return pgetSubtitleStreamInfo(iStream, stream_info);
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

	void mplayer_GetAudioInfo(char* strFourCC,char* strAudioCodec, long* bitrate, long* samplerate, int* channels, int* bVBR)
	{
		pGetAudioInfo(strFourCC,strAudioCodec, bitrate, samplerate, channels, bVBR);
	}

	void mplayer_GetVideoInfo(char* strFourCC,char* strVideoCodec, float* fps, unsigned int* iWidth,unsigned int* iHeight, long* tooearly, long* toolate)
	{
		pGetVideoInfo(strFourCC, strVideoCodec, fps, iWidth,iHeight, tooearly, toolate);
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
		if(pSetCacheBackBuffer)
			pSetCacheBackBuffer(iCacheBackBuffer);
	}

	void init_color_conversions()
	{
		pInitColorConversions();
	}
	int image_output(IMAGE * image, unsigned int width,int height,unsigned int edged_width, unsigned char * dst[4], unsigned int dst_stride[4],int csp,int interlaced)
	{
		return pImageOutput(image, width,height,edged_width, dst, dst_stride,csp,interlaced);
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
		return pProcess();
	}

	int mplayer_init(int argc,char* argv[])
	{
		return pInitPlayer(argc,argv);
	}

	int mplayer_open_file(const char* szFile)
	{
		wmaDMOdll = NULL;                                       //hacks for free library for DMOs
		wmvDMOdll = NULL;
		wmsDMOdll = NULL;
		return pOpenFile(szFile);
	}

	int mplayer_close_file()
	{
		int hr = pCloseFile();
		if (wmaDMOdll) 
			dllFreeLibrary( (HINSTANCE)wmaDMOdll );
		wmaDMOdll = NULL;
		if (wmvDMOdll) 
			dllFreeLibrary( (HINSTANCE)wmvDMOdll );
		wmvDMOdll = NULL;
		if (wmsDMOdll) 
			dllFreeLibrary( (HINSTANCE)wmsDMOdll );
		wmsDMOdll = NULL;

		// Unload unfreed dll's
		for (int i = m_vecDlls.size()-1; i >= 0; i--)
		{
			DllLoader * dll = m_vecDlls[i];			
			CLog::Log(LOGDEBUG,"Freeing Unfreed DLL: %s handle: 0x%x", dll->GetDLLName(), dll);
			dllFreeLibrary((HINSTANCE)dll);
		}

		// Free allocated memory for FS segment
		if (fs_seg != NULL) {
			CLog::Log(LOGDEBUG,"Freeing FS segment @ 0x%x", fs_seg);
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

	void vo_draw_alpha_yv12(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride)
	{
		pVODrawAlphayv12(w,h, src, srca, srcstride, dstbase,dststride);
	}
	void vo_draw_alpha_yuy2(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride)
	{
		pVODrawAlphayuy2(w,h, src, srca, srcstride, dstbase,dststride);
	}
	void vo_draw_alpha_rgb24(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride)
	{
		pVODrawAlphargb24(w,h, src, srca, srcstride, dstbase,dststride);
	}
	void vo_draw_alpha_rgb32(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride)
	{
		pVODrawAlphargb32(w,h, src, srca, srcstride, dstbase,dststride);
	}
	void vo_draw_alpha_rgb15(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride)
	{
		pVODrawAlphargb15(w,h, src, srca, srcstride, dstbase,dststride);
	}
	void vo_draw_alpha_rgb16(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride)
	{
		pVODrawAlphargb16(w,h, src, srca, srcstride, dstbase,dststride);
	}



	void aspect_save_orig(int orgw, int orgh)
	{
		pAspectSaveOrig(orgw,orgh);
	}
	void aspect(unsigned int *srcw, unsigned int *srch, int zoom)
	{
		pAspect(srcw,srch,zoom);
	}
	void aspect_save_prescale(int prew, int preh)
	{
		pAspectSavePrescale(prew,preh);
	}
	void aspect_save_screenres(int scrw, int scrh)
	{
		pAspectSaveScreenres(scrw,scrh);
	}

	void vo_draw_text(int dxs,int dys,void (*mydrawalpha)(int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride))
	{
		pVODrawText(dxs,dys,mydrawalpha);
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
		if(pExitPlayer)
			pExitPlayer("");
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
		dll.ResolveExport("aspect_save_screenres", (void**)&pAspectSaveScreenres);
		dll.ResolveExport("aspect_save_prescale", (void**)&pAspectSavePrescale);
		dll.ResolveExport("aspect_save_orig", (void**)&pAspectSaveOrig);
		dll.ResolveExport("aspect", (void**)&pAspect);
		dll.ResolveExport("vo_draw_alpha_yv12", (void**)&pVODrawAlphayv12);
		dll.ResolveExport("vo_draw_alpha_yuy2", (void**)&pVODrawAlphayuy2);
		dll.ResolveExport("vo_draw_alpha_rgb24", (void**)&pVODrawAlphargb24);
		dll.ResolveExport("vo_draw_alpha_rgb32", (void**)&pVODrawAlphargb32);
		dll.ResolveExport("vo_draw_alpha_rgb15", (void**)&pVODrawAlphargb15);
		dll.ResolveExport("vo_draw_alpha_rgb16", (void**)&pVODrawAlphargb16);
		dll.ResolveExport("mplayer_get_pts", (void**)&pGetPTS);
		dll.ResolveExport("mplayer_HasVideo", (void**)&pHasVideo);
		dll.ResolveExport("mplayer_HasAudio", (void**)&pHasAudio);
		dll.ResolveExport("image_output", (void**)&pImageOutput);
		dll.ResolveExport("init_color_conversions", (void**)&pInitColorConversions);
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
		dll.ResolveExport("mplayer_getTime", (void**)&pgetTime);
		dll.ResolveExport("mplayer_ToFFRW", (void**)&pToFFRW);
		dll.ResolveExport("mplayer_getCurrentTime", (void**)&pgetCurrentTime);
		dll.ResolveExport("mplayer_showosd", (void**)&pShowOSD);
		dll.ResolveExport("mplayer_get_current_module", (void**)&pGetCurrentModule);
		dll.ResolveExport("exit_player", (void**)&pExitPlayer);

		pSetVideoFunctions(&video_functions);
		pSetAudioFunctions(&audio_functions);
		init_color_conversions();
	}
};
