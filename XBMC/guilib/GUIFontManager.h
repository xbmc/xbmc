/*!
\file GUIFontManager.h
\brief
*/

#ifndef GUILIB_FONTMANAGER_H
#define GUILIB_FONTMANAGER_H

#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GraphicContext.h"

// Forward
class CGUIFont;
class CGUIFontTTF;
class TiXmlDocument;
class TiXmlNode;

struct OrigFontInfo
{
   int size;
   float aspect;
   CStdString fontFilePath;
   CStdString fileName;
};

/*!
 \ingroup textures
 \brief
 */
class GUIFontManager
{
public:
  GUIFontManager(void);
  virtual ~GUIFontManager(void);
  void Unload(const CStdString& strFontName);
  void LoadFonts(const CStdString& strFontSet);
  CGUIFont* LoadTTF(const CStdString& strFontName, const CStdString& strFilename, DWORD textColor, DWORD shadowColor, const int iSize, const int iStyle, float lineSpacing = 1.0f, float aspect = 1.0f, RESOLUTION res = INVALID);
  CGUIFont* GetFont(const CStdString& strFontName, bool fallback = true);
  void Clear();
  void FreeFontFile(CGUIFontTTF *pFont);

  bool IsFontSetUnicode() { return m_fontsetUnicode; }
  bool IsFontSetUnicode(const CStdString& strFontSet);
  bool GetFirstFontSetUnicode(CStdString& strFontSet);

  void ReloadTTFFonts(void);

protected:
  void LoadFonts(const TiXmlNode* fontNode);
  CGUIFontTTF* GetFontFile(const CStdString& strFontFile);
  bool OpenFontFile(TiXmlDocument& xmlDoc);

  std::vector<CGUIFont*> m_vecFonts;
  std::vector<CGUIFontTTF*> m_vecFontFiles;
  std::vector<OrigFontInfo> m_vecFontInfo;
  bool m_fontsetUnicode;
  RESOLUTION m_skinResolution;
};

/*!
 \ingroup textures
 \brief
 */
extern GUIFontManager g_fontManager;
#endif
