#pragma once

typedef struct SubtitleStyle_s
{
	wchar_t* fontName; 
	int fontColor;
	int fontIsBold;
	int fontSize;
	int fontCharset;
	int shadow;
	int borderWidth;
	int isBorderOutline; //outline or opaque box
} SubtitleStyle_t;



//set default subtitle's style (call before LoadSubtitles to take effect)
void SetDefaultStyle(const SubtitleStyle_t* style, BOOL overrideUserStyles);
void SetAdvancedOptions(int subPicsBufferAhead, SIZE textureSize, BOOL pow2tex, BOOL disableAnim);

//load subtitles for video file fn, with given (rendered) graph 
BOOL LoadSubtitles(IDirect3DDevice9* d3DDev, SIZE size, const wchar_t* fn, IGraphBuilder* pGB, const wchar_t* paths);

//set sample time (set from EVR presenter, not used in case of vmr9)
void SetTime(REFERENCE_TIME nsSampleTime);

//render subtitles
void Render(int x, int y, int width, int height);

//save subtitles
BOOL IsModified(); //timings were modified
void SaveToDisk();

////
//subs management functions
///
int GetCount(); //total subtitles
BSTR GetLanguage(int i); //i  range from 0 to GetCount()-1
int GetCurrent(); 
void SetCurrent(int i);
BOOL GetEnable();
void SetEnable(BOOL enable);
int GetDelay(); //in milliseconds
void SetDelay(int delay_ms); //in milliseconds


void FreeSubtitles();

