/*!
	\file GUIFont.h
	\brief 
	*/

#ifndef CGUILIB_GUIFONTTTF_H
#define CGUILIB_GUIFONTTTF_H
#pragma once

#include "common/xbfont.h"
#include "GUIFont.h"
#include <xfont.h>
#include <hash_map>

/*!
	\ingroup textures
	\brief 
	*/
class CGUIFontTTF: public CGUIFont
{

public:

  CGUIFontTTF(const CStdString& strFontName);
  virtual ~CGUIFontTTF(void);

  // Change font style: XFONT_NORMAL, XFONT_BOLD, XFONT_ITALICS, XFONT_BOLDITALICS
  bool Load(const CStdString& strFilename, int height = 20, int style = XFONT_NORMAL);

  virtual void GetTextExtent(const WCHAR* strText, FLOAT* pWidth, 
							 FLOAT* pHeight, BOOL bFirstLineOnly = FALSE);

protected:
  virtual void DrawTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
							const WCHAR* strText, DWORD cchText, DWORD dwFlags = 0,
							FLOAT fMaxPixelWidth = 0.0f);

  virtual void DrawColourTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette,
								  const WCHAR* strText, BYTE* pbColours, DWORD cchText, DWORD dwFlags,
								  FLOAT fMaxPixelWidth);

  void DrawTrueType(LONG nPosX, LONG nPosY, WCHAR* text, BOOL bShadow);

  void DrawTrueType(LPDIRECT3DSURFACE8 pSurface, LONG nPosX, LONG nPosY, WCHAR* text, BOOL bShadow);

  XFONT*		m_pTrueTypeFont;
  int			m_iHeight;
  int			m_iStyle;
  CStdString	m_strFilename;
};

#endif
