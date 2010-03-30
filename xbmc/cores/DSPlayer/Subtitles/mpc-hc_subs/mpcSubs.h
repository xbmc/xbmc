#pragma once

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MPCSUBS_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DELME_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef MPCSUBS_EXPORTS
#define MPCSUBS_API __declspec(dllexport)
#else
#define MPCSUBS_API extern
#endif

struct SubtitleStyle
{
	wchar_t* fontName; 
	int fontColor;
	int fontIsBold;
	int fontSize;
	int fontCharset;
	int shadow;
	int borderWidth;
	int isBorderOutline; //outline or opaque box
};

extern "C" {

//set default subtitle's style (call before LoadSubtitles to take effect)
MPCSUBS_API void SetDefaultStyle(const SubtitleStyle* style, BOOL overrideUserStyles);
MPCSUBS_API void SetAdvancedOptions(int subPicsBufferAhead, SIZE textureSize, BOOL pow2tex, BOOL disableAnim);

//load subtitles for video file fn, with given (rendered) graph 
MPCSUBS_API BOOL LoadSubtitles(IDirect3DDevice9* d3DDev, SIZE size, const wchar_t* fn, IGraphBuilder* pGB, const wchar_t* paths);

//set sample time (set from EVR presenter, not used in case of vmr9)
MPCSUBS_API void SetTime(REFERENCE_TIME nsSampleTime);

//render subtitles
MPCSUBS_API void Render(int x, int y, int width, int height);

//save subtitles
MPCSUBS_API BOOL IsModified(); //timings were modified
MPCSUBS_API void SaveToDisk();

////
//subs management functions
///
MPCSUBS_API int GetCount(); //total subtitles
MPCSUBS_API BSTR GetLanguage(int i); //i  range from 0 to GetCount()-1
MPCSUBS_API int GetCurrent(); 
MPCSUBS_API void SetCurrent(int i);
MPCSUBS_API BOOL GetEnable();
MPCSUBS_API void SetEnable(BOOL enable);
MPCSUBS_API int GetDelay(); //in milliseconds
MPCSUBS_API void SetDelay(int delay_ms); //in milliseconds


MPCSUBS_API void FreeSubtitles();

}
