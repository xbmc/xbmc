#ifndef CGUILIB_GUIFONT_H
#define CGUILIB_GUIFONT_H
#pragma once

#include "gui3d.h"

#include "graphiccontext.h"
#include <string>
#include "common/xbfont.h"
using namespace std;

class CGUIFont: public CXBFont  
{
public:
  CGUIFont(void);
  virtual ~CGUIFont(void);
  bool  Load(const string& strFontName,const string& strFilename);
  const string& GetFontName() const;
  void DrawShadowText( FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                        const WCHAR* strText, DWORD dwFlags=0,
                        FLOAT fMaxPixelWidth =0.0,
                        int iShadowWidth=5, 
                        int iShadowHeight=5,
                        DWORD dwShadowColor=0xff000000);
  void DrawTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                              const WCHAR* strText,float fMaxWidth);

protected:
  string m_strFontName;
};

#endif