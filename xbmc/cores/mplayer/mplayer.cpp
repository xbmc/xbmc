#include <xtl.h>
#include "audio.h"
#include "video.h"
#include "../DllLoader/dll.h"

//deal wma, and wmv dlls in openfile, and closefile functions
extern DllLoader * wmaDMOdll;
extern DllLoader * wmvDMOdll;
extern DllLoader * wmsDMOdll;
extern "C" BOOL WINAPI dllFreeLibrary(HINSTANCE hLibModule);

int							(__cdecl* pInitPlayer)(int argc, char* argvp[]);
int							(__cdecl* pOpenFile)(const char*);
int							(__cdecl* pProcess)();
int							(__cdecl* pCloseFile)();
void						(__cdecl* pSetAudioFunctions)(ao_functions_t* pFunctions);
ao_functions_t* (__cdecl* pGetAudioFunctions)();
int							(__cdecl* pAudioOutFormatBits)(int);
ao_data_t*			(__cdecl* pGetAOData)(void);
void						(__cdecl* pSetVideoFunctions)(vo_functions_t*);
void						(__cdecl* pMplayerPutKey)(int);
void						(__cdecl* pVODrawText)(int dxs,int dys,void (*draw_alpha)(int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride));
void						(__cdecl* pAspectSaveScreenres)(int scrw, int scrh);
void						(__cdecl* pAspectSavePrescale)(int scrw, int scrh);
void						(__cdecl* pAspectSaveOrig)(int scrw, int scrh);
void						(__cdecl* pAspect)(unsigned int*, unsigned int*, int);
void						(__cdecl* pVODrawAlphayv12)(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride);
void						(__cdecl* pVODrawAlphayuy2)(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride);
void						(__cdecl* pVODrawAlphargb24)(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride);
void						(__cdecl* pVODrawAlphargb32)(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride);
void						(__cdecl* pVODrawAlphargb15)(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride);
void						(__cdecl* pVODrawAlphargb16)(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride);
__int64					(__cdecl* pGetPTS)();
BOOL						(__cdecl* pHasVideo)();
BOOL						(__cdecl* pHasAudio)();
void						(__cdecl* pyv12toyuy2)(const unsigned char *ysrc, const unsigned char *usrc, const unsigned char *vsrc, unsigned char *dst,unsigned int width, unsigned int height,int lumStride, int chromStride, int dstStride);
int							(__cdecl* pImageOutput)(IMAGE * image, unsigned int width,int height,unsigned int edged_width, unsigned char * dst[4], unsigned int dst_stride[4],int csp,int interlaced);
void						(__cdecl* pInitColorConversions)();
void						(__cdecl* pSetCacheSize)(int);
void						(__cdecl* pGetAudioInfo)(char* strFourCC,char* strAudioCodec, long* bitrate, long* samplerate, int* channels, int* bVBR);
void						(__cdecl* pGetVideoInfo)(char* strFourCC,char* strVideoCodec, float* fps, unsigned int* iWidth,unsigned int* iHeight, long* tooearly, long* toolate);
void						(__cdecl* pGetGeneralInfo)(long* lFramesDropped, int* iQuality, int* iCacheFilled, float* fTotalCorrection, float* fAVDelay);

void						(__cdecl* psetAVDelay)(float);
void						(__cdecl* psetSubtitleDelay)(float);
void						(__cdecl* psetPercentage)(int);
void						(__cdecl* psetSubtitle)(int);
void						(__cdecl* pshowSubtitle)(int);
void						(__cdecl* psetAudioLanguage)(int);

float					  (__cdecl* pgetAVDelay)();
float					  (__cdecl* pgetSubtitleDelay)();
int 						(__cdecl* pgetPercentage)();
int             (__cdecl* pgetSubtitle)();
int             (__cdecl* pgetSubtitleCount)();
int             (__cdecl* pgetSubtitleVisible)();
int             (__cdecl* pgetAudioLanguageCount)();
int             (__cdecl* pgetAudioLanguage)();
int             (__cdecl* pgetAudioStream)(int);
int             (__cdecl* pgetAudioStreamCount)();
extern "C" 
{
  int mplayer_getAudioLanguageCount()
  {
    return pgetAudioLanguageCount();
  }
  
  int mplayer_getAudioLanguage()
  {
    return pgetAudioLanguage();
  }
  
  int mplayer_getAudioStream(int iStream)
  {
    return pgetAudioStream(iStream);
  }

  int mplayer_getAudioStreamCount()
  {
    return pgetAudioStreamCount();
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

	void	mplayer_setcache_size(int iCacheSize)
	{
		pSetCacheSize(iCacheSize);
	}
	void init_color_conversions()
	{
		pInitColorConversions();
	}
	int image_output(IMAGE * image, unsigned int width,int height,unsigned int edged_width, unsigned char * dst[4], unsigned int dst_stride[4],int csp,int interlaced)
	{
		return pImageOutput(image, width,height,edged_width, dst, dst_stride,csp,interlaced);
	}


	BOOL		mplayer_HasVideo()
	{
		return pHasVideo();
	}

	BOOL		mplayer_HasAudio()
	{
		return pHasAudio();
	}

	void yv12toyuy2(const unsigned char *ysrc, const unsigned char *usrc, const unsigned char *vsrc, unsigned char *dst,unsigned int width, unsigned int height,int lumStride, int chromStride, int dstStride)
	{
		pyv12toyuy2(ysrc, usrc, vsrc, dst,width, height,lumStride, chromStride, dstStride);
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
		wmaDMOdll = NULL;					//hacks for free library for DMOs
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

	void mplayer_load_dll(DllLoader& dll)
	{

		void* pProc;
		dll.ResolveExport("audio_out_format_bits", &pProc);
		pAudioOutFormatBits=(int (__cdecl*)(int))pProc;

		dll.ResolveExport("SetVideoFunctions", &pProc);
		pSetVideoFunctions=(void (__cdecl*)(vo_functions_t*))pProc;

		dll.ResolveExport("GetAOData", &pProc);
		pGetAOData=(ao_data_t* (__cdecl*)())pProc;

		dll.ResolveExport("SetAudioFunctions", &pProc);
		pSetAudioFunctions=(void (__cdecl*)(ao_functions_t* ))pProc;

		dll.ResolveExport("mplayer_init", &pProc);
		pInitPlayer=(int (__cdecl*)(int argc, char* argvp[]))pProc;
		
		dll.ResolveExport("mplayer_open_file", &pProc);
		pOpenFile=(int (__cdecl*)(const char* ))pProc;
		
		dll.ResolveExport("mplayer_process", &pProc);
		pProcess=(int (__cdecl*)())pProc;

		
		dll.ResolveExport("mplayer_close_file", &pProc);
		pCloseFile=(int (__cdecl*)())pProc;

		dll.ResolveExport("mplayer_put_key", &pProc);
		pMplayerPutKey=(void (__cdecl*)(int))pProc;

		dll.ResolveExport("vo_draw_text", &pProc);
		pVODrawText=(void (__cdecl*)(int dxs,int dys,void (*draw_alpha)(int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride)))pProc;


		dll.ResolveExport("aspect_save_screenres", &pProc);
		pAspectSaveScreenres=(void (__cdecl*)(int scrw, int scrh))pProc;

		dll.ResolveExport("aspect_save_prescale", &pProc);
		pAspectSavePrescale=(void (__cdecl*)(int scrw, int scrh))pProc;

		dll.ResolveExport("aspect_save_orig", &pProc);
		pAspectSaveOrig=(void (__cdecl*)(int scrw, int scrh))pProc;

		dll.ResolveExport("aspect", &pProc);
		pAspect=(void (__cdecl*)(unsigned int*,unsigned int*, int ))pProc;

		dll.ResolveExport("vo_draw_alpha_yv12", &pProc);
		pVODrawAlphayv12=(void (__cdecl*)(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride))pProc;

		dll.ResolveExport("vo_draw_alpha_yuy2", &pProc);
		pVODrawAlphayuy2=(void (__cdecl*)(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride))pProc;

		dll.ResolveExport("vo_draw_alpha_rgb24", &pProc);
		pVODrawAlphargb24=(void (__cdecl*)(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride))pProc;

		dll.ResolveExport("vo_draw_alpha_rgb32", &pProc);
		pVODrawAlphargb32=(void (__cdecl*)(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride))pProc;

		dll.ResolveExport("vo_draw_alpha_rgb15", &pProc);
		pVODrawAlphargb15=(void (__cdecl*)(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride))pProc;

		dll.ResolveExport("vo_draw_alpha_rgb16", &pProc);
		pVODrawAlphargb16=(void (__cdecl*)(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride))pProc;

		dll.ResolveExport("mplayer_get_pts", &pProc);
		pGetPTS=(__int64 (__cdecl*)())pProc;

		dll.ResolveExport("mplayer_HasVideo", &pProc);
		pHasVideo=(BOOL (__cdecl*)())pProc;

		dll.ResolveExport("mplayer_HasAudio", &pProc);
		pHasAudio=(BOOL (__cdecl*)())pProc;

		dll.ResolveExport("yv12toyuy2_C", &pProc);
		pyv12toyuy2= (void (__cdecl*)(const unsigned char *ysrc, const unsigned char *usrc, const unsigned char *vsrc, unsigned char *dst,unsigned int width, unsigned int height,int lumStride, int chromStride, int dstStride))pProc;

		dll.ResolveExport("image_output", &pProc);
		pImageOutput=(int (__cdecl*)(IMAGE * image, unsigned int width,int height,unsigned int edged_width, unsigned char * dst[4], unsigned int dst_stride[4],int csp,int interlaced))pProc;

		dll.ResolveExport("init_color_conversions", &pProc);
		pInitColorConversions=(void(__cdecl*)())pProc;

		dll.ResolveExport("mplayer_setcache_size", &pProc);
		pSetCacheSize=(void(__cdecl*)(int))pProc;

		dll.ResolveExport("mplayer_GetAudioInfo", &pProc);
		pGetAudioInfo=(void(__cdecl*)(char* strFourCC,char* strAudioCodec, long* bitrate, long* samplerate, int* channels, BOOL* bVBR))pProc;

		dll.ResolveExport("mplayer_GetVideoInfo", &pProc);
		pGetVideoInfo=(void(__cdecl*)(char* strFourCC,char* strVideoCodec, float* fps, unsigned int* iWidth,unsigned int* iHeight, long* tooearly, long* toolate))pProc;

		dll.ResolveExport("mplayer_GetGeneralInfo", &pProc);
		pGetGeneralInfo=(void(__cdecl*)(long* lFramesDropped, int* iQuality, int* iCacheFilled, float* fTotalCorrection, float* fAVDelay))pProc;

		dll.ResolveExport("mplayer_setAVDelay", &pProc);
		psetAVDelay=(void(__cdecl*)(float))pProc;

		dll.ResolveExport("mplayer_setSubtitleDelay", &pProc);
		psetSubtitleDelay=(void(__cdecl*)(float))pProc;

		dll.ResolveExport("mplayer_setPercentage", &pProc);
		psetPercentage=(void(__cdecl*)(int))pProc;

		dll.ResolveExport("mplayer_getAVDelay", &pProc);
		pgetAVDelay=(float(__cdecl*)())pProc;

		dll.ResolveExport("mplayer_getSubtitleDelay", &pProc);
		pgetSubtitleDelay=(float(__cdecl*)())pProc;

		dll.ResolveExport("mplayer_getPercentage", &pProc);
		pgetPercentage=(int(__cdecl*)())pProc;

    dll.ResolveExport("mplayer_getSubtitle", &pProc);
		pgetSubtitle=(int(__cdecl*)())pProc;

		dll.ResolveExport("mplayer_getSubtitleCount", &pProc);
		pgetSubtitleCount=(int(__cdecl*)())pProc;

		dll.ResolveExport("mplayer_SubtitleVisible", &pProc);
		pgetSubtitleVisible=(int(__cdecl*)())pProc;


		dll.ResolveExport("mplayer_setPercentage", &pProc);
		psetPercentage=(void(__cdecl*)(int))pProc;

		dll.ResolveExport("mplayer_setSubtitle", &pProc);
		psetSubtitle=(void(__cdecl*)(int))pProc;

		dll.ResolveExport("mplayer_showSubtitle", &pProc);
		pshowSubtitle=(void(__cdecl*)(int))pProc;
  
		dll.ResolveExport("mplayer_getAudioLanguageCount", &pProc);
		pgetAudioLanguageCount=(int(__cdecl*)())pProc;
    
		dll.ResolveExport("mplayer_getAudioLanguage", &pProc);
		pgetAudioLanguage=(int(__cdecl*)())pProc;
    
		dll.ResolveExport("mplayer_getAudioStream", &pProc);
		pgetAudioStream=(int(__cdecl*)(int))pProc;
    
		dll.ResolveExport("mplayer_getAudioStreamCount", &pProc);
		pgetAudioStreamCount=(int(__cdecl*)())pProc;


		dll.ResolveExport("mplayer_setAudioLanguage", &pProc);
		psetAudioLanguage=(void(__cdecl*)(int))pProc;

		pSetVideoFunctions(&video_functions);
		pSetAudioFunctions(&audio_functions);
		init_color_conversions();
	}
};