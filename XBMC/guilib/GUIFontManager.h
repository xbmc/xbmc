#ifndef GUILIB_FONTMANAGER_H
#define GUILIB_FONTMANAGER_H

#pragma once
#include "gui3d.h"

#include "graphiccontext.h"
#include "guifont.h"

#include <vector>
#include <string>
using namespace std;

class GUIFontManager
{
public:
  GUIFontManager(void);
  virtual ~GUIFontManager(void);
	CGUIFont*  Load(const string& strFontName,const string& strFilename);
  void       LoadFonts(const string& strFilename);
  CGUIFont*	GetFont(const string& strFontName);
protected:
  vector<CGUIFont*> m_vecFonts;
};

extern GUIFontManager g_fontManager;
#endif