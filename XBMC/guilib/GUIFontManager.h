#ifndef GUILIB_FONTMANAGER_H
#define GUILIB_FONTMANAGER_H

#pragma once
#include "gui3d.h"

#include "graphiccontext.h"
#include "guifont.h"

#include <vector>
#include "stdstring.h"
using namespace std;

class GUIFontManager
{
public:
  GUIFontManager(void);
  virtual ~GUIFontManager(void);
	CGUIFont*  Load(const CStdString& strFontName,const CStdString& strFilename);
  void       LoadFonts(const CStdString& strFilename);
  CGUIFont*	GetFont(const CStdString& strFontName);
protected:
  vector<CGUIFont*> m_vecFonts;
};

extern GUIFontManager g_fontManager;
#endif