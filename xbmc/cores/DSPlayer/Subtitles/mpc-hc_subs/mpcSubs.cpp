#include "stdafx.h"
#include <d3d9.h>
#include "mpcSubs.h"
#include "..\subtitles\RTS.h"
#include "SubManager.h"

static boost::shared_ptr<CSubManager> g_subManager;

void SetDefaultStyle(const SubtitleStyle* s, BOOL overrideUserStyles)
{
	g_style.fontName = s->fontName;
	g_style.colors[0] = s->fontColor;
	g_style.fontWeight = (s->fontIsBold ? 700 : 400);
	g_style.fontSize = s->fontSize; 
	g_style.charSet = s->fontCharset;
	g_style.shadowDepthX  = g_style.shadowDepthY = s->shadow;
	g_style.outlineWidthX = g_style.outlineWidthY = s->borderWidth;
	g_style.borderStyle = (s->isBorderOutline ? 0 : 1); // 0: outline, 1: opaque box
	g_overrideUserStyles = overrideUserStyles;
}

void SetAdvancedOptions(int subPicsBufferAhead, SIZE textureSize, BOOL pow2tex, BOOL disableAnim)
{
	g_subPicsBufferAhead = subPicsBufferAhead;
	g_textureSize = textureSize;
	g_pow2tex = pow2tex != 0;
	g_disableAnim = disableAnim;
}

BOOL LoadSubtitles(IDirect3DDevice9* d3DDev, SIZE size, const wchar_t* fn, IGraphBuilder* pGB, const wchar_t* paths, ISubManager** manager)
{
	*manager = NULL;
  g_subManager.reset();
	HRESULT hr = S_OK;
	CSubManager *subManager(new CSubManager(d3DDev, size, hr));
	if (FAILED(hr))
	{
    delete subManager;
		return FALSE;
	}
	subManager->LoadSubtitlesForFile(fn, pGB, paths);
	g_subManager.reset(subManager);
  *manager = g_subManager.get();
	return TRUE;
}

void SetTime(REFERENCE_TIME nsSampleTime)
{
	if (g_subManager)
	{
		g_subManager->SetTime(nsSampleTime);
	}
}

void Render(int x, int y, int width, int height)
{
	if (g_subManager)
	{
		g_subManager->Render(x, y, width, height);
	}
}

int GetCount()
{
	return (g_subManager ? g_subManager->GetCount() : 0);
}

BSTR GetLanguage(int i)
{
	return (g_subManager ? g_subManager->GetLanguage(i) : NULL);
}

int GetCurrent()
{
	return (g_subManager ? g_subManager->GetCurrent() : -1);
}

void SetCurrent(int current)
{
	if (g_subManager)
		g_subManager->SetCurrent(current);
}

BOOL GetEnable()
{
	return (g_subManager ? g_subManager->GetEnable() : FALSE);
}

void SetEnable(BOOL enable)
{
	if (g_subManager)
		g_subManager->SetEnable(enable);
}

int GetDelay()
{
	return (g_subManager ? g_subManager->GetDelay() : 0);
}

void SetDelay(int delay)
{
	if (g_subManager)
		g_subManager->SetDelay(delay);
}

void SaveToDisk()
{
	if (g_subManager)
		g_subManager->SaveToDisk();
}

BOOL IsModified()
{
	return (g_subManager ? (g_subManager->IsModified() ? TRUE : FALSE) : FALSE);
}

void FreeSubtitles()
{
	g_subManager.reset();
}
