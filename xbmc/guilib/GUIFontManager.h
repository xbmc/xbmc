/*!
\file GUIFontManager.h
\brief
*/

#ifndef GUILIB_FONTMANAGER_H
#define GUILIB_FONTMANAGER_H

#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <utility>
#include <vector>

#include "GraphicContext.h"
#include "IMsgTargetCallback.h"
#include "utils/GlobalsHandling.h"

// Forward
class CGUIFont;
class CGUIFontTTFBase;
class CXBMCTinyXML;
class TiXmlNode;
class CSetting;

struct OrigFontInfo
{
   int size;
   float aspect;
   std::string fontFilePath;
   std::string fileName;
   RESOLUTION_INFO sourceRes;
   bool preserveAspect;
   bool border;
};

/*!
 \ingroup textures
 \brief
 */
class GUIFontManager : public IMsgTargetCallback
{
public:
  GUIFontManager(void);
  virtual ~GUIFontManager(void);

  virtual bool OnMessage(CGUIMessage &message);

  void Unload(const std::string& strFontName);
  void LoadFonts(const std::string &fontSet);
  CGUIFont* LoadTTF(const std::string& strFontName, const std::string& strFilename, color_t textColor, color_t shadowColor, const int iSize, const int iStyle, bool border = false, float lineSpacing = 1.0f, float aspect = 1.0f, const RESOLUTION_INFO *res = NULL, bool preserveAspect = false);
  CGUIFont* GetFont(const std::string& strFontName, bool fallback = true);

  /*! \brief return a default font
   \param border whether the font should be a font with an outline
   \return the font.  NULL if no default font can be found.
   */
  CGUIFont* GetDefaultFont(bool border = false);

  void Clear();
  void FreeFontFile(CGUIFontTTFBase *pFont);

  static void SettingOptionsFontsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);

protected:
  void ReloadTTFFonts();
  static void RescaleFontSizeAndAspect(float *size, float *aspect, const RESOLUTION_INFO &sourceRes, bool preserveAspect);
  void LoadFonts(const TiXmlNode* fontNode);
  CGUIFontTTFBase* GetFontFile(const std::string& strFontFile);
  static void GetStyle(const TiXmlNode *fontNode, int &iStyle);

  std::vector<CGUIFont*> m_vecFonts;
  std::vector<CGUIFontTTFBase*> m_vecFontFiles;
  std::vector<OrigFontInfo> m_vecFontInfo;
  RESOLUTION_INFO m_skinResolution;
  bool m_canReload;
};

/*!
 \ingroup textures
 \brief
 */
XBMC_GLOBAL_REF(GUIFontManager, g_fontManager);
#define g_fontManager XBMC_GLOBAL_USE(GUIFontManager)
#endif
