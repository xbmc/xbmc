#ifndef CGUILIB_GUIFONT_H
#define CGUILIB_GUIFONT_H
#pragma once

#include "gui3d.h"
#include <string>
#include "graphiccontext.h"
#include "stdstring.h"
#include "common/xbfont.h"
using namespace std;

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

protected:
  CStdString m_strFontName;
};

#endif