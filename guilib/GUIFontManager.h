/*!
\file GUIFontManager.h
\brief 
*/

#ifndef GUILIB_FONTMANAGER_H
#define GUILIB_FONTMANAGER_H

#pragma once

// Forward
class CGUIFont;
class CGUIFontBase;

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
  CGUIFont* LoadXPR(const CStdString& strFontName, const CStdString& strFilename, DWORD textColor, DWORD shadowColor);
  CGUIFont* LoadTTF(const CStdString& strFontName, const CStdString& strFilename, DWORD textColor, DWORD shadowColor, const int iSize, const int iStyle);
  CGUIFont* GetFont(const CStdString& strFontName);
  void Clear();

protected:
  void LoadFonts(const TiXmlNode* fontNode);
  CGUIFontBase* GetFontFile(const CStdString& strFontFile);

  vector<CGUIFont*> m_vecFonts;
  vector<CGUIFontBase*> m_vecFontFiles;
};

/*!
 \ingroup textures
 \brief 
 */
extern GUIFontManager g_fontManager;
#endif
