/*!
	\file GUIFont.h
	\brief 
	*/

#ifndef CGUILIB_GUIFONT_H
#define CGUILIB_GUIFONT_H
#pragma once

#include "gui3d.h"
#include <string>
#include "graphiccontext.h"
#include "stdstring.h"
#include "common/xbfont.h"
using namespace std;

/*!
	\ingroup textures
	\brief 
	*/
class CGUIFont: public CXBFont  
{
public:
  CGUIFont(void);
  virtual ~CGUIFont(void);
  bool  Load(const CStdString& strFontName,const CStdString& strFilename);
  const CStdString& GetFontName() const;
  void DrawShadowText( FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                        const WCHAR* strText, DWORD dwFlags=0,
                        FLOAT fMaxPixelWidth =0.0,
                        int iShadowWidth=5, 
                        int iShadowHeight=5,
                        DWORD dwShadowColor=0xff000000);
  void DrawTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                              const WCHAR* strText,float fMaxWidth);
  void DrawColourTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette,
                              const WCHAR* strText, BYTE* pbColours, float fMaxWidth);

  virtual HRESULT DrawText( FLOAT sx, FLOAT sy, DWORD dwColor, 
                    const WCHAR* strText, DWORD dwFlags=0L,
                    FLOAT fMaxPixelWidth = 0.0f );
  virtual HRESULT DrawTextEx( FLOAT sx, FLOAT sy, DWORD dwColor, 
                    const WCHAR* strText, DWORD cchText, DWORD dwFlags=0L,
                    FLOAT fMaxPixelWidth = 0.0f );
protected:
  CStdString m_strFontName;
  static WCHAR* m_pwzBuffer;
  static INT m_nBufferSize;
	float m_iMaxCharWidth;
};

#endif
