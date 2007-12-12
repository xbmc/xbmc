/*!
\file GUIFontManager.h
\brief 
*/

#ifndef GUILIB_FONTMANAGER_H
#define GUILIB_FONTMANAGER_H

#include "GraphicContext.h"

#pragma once

// Forward
class CGUIFont;
class CGUIFontTTF;

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
  CGUIFont* LoadTTF(const CStdString& strFontName, const CStdString& strFilename, DWORD textColor, DWORD shadowColor, const int iSize, const int iStyle, float aspect = 1.0f);
  CGUIFont* GetFont(const CStdString& strFontName, bool fallback = true);
  void Clear();
  void FreeFontFile(CGUIFontTTF *pFont);

  bool IsFontSetUnicode() { return m_fontsetUnicode; }
  bool IsFontSetUnicode(const CStdString& strFontSet);
  bool GetFirstFontSetUnicode(CStdString& strFontSet);

protected:
  void LoadFonts(const TiXmlNode* fontNode);
  CGUIFontTTF* GetFontFile(const CStdString& strFontFile);
  bool OpenFontFile(TiXmlDocument& xmlDoc);

  vector<CGUIFont*> m_vecFonts;
  vector<CGUIFontTTF*> m_vecFontFiles;
  bool m_fontsetUnicode;
  RESOLUTION m_skinResolution;
};

/*!
 \ingroup textures
 \brief 
 */
extern GUIFontManager g_fontManager;
#endif
