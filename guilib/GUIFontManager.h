/*!
	\file GUIFontManager.h
	\brief 
	*/

#ifndef GUILIB_FONTMANAGER_H
#define GUILIB_FONTMANAGER_H

#pragma once
#include "gui3d.h"

#include "graphiccontext.h"
#include "guifont.h"

#include <vector>
#include "stdstring.h"
using namespace std;

/*!
	\ingroup textures
	\brief 
	*/
class GUIFontManager
{
public:
  GUIFontManager(void);
  virtual ~GUIFontManager(void);
	CGUIFont*				Load(const CStdString& strFontName,const CStdString& strFilename);
	void						Unload(const CStdString& strFontName);
  void						LoadFonts(const CStdString& strFilename);
  CGUIFont*				GetFont(const CStdString& strFontName);
	void						Clear();
protected:
  vector<CGUIFont*> m_vecFonts;
};

/*!
	\ingroup textures
	\brief 
	*/
extern GUIFontManager g_fontManager;
#endif
