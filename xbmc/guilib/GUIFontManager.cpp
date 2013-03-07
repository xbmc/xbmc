/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIFontManager.h"
#include "GraphicContext.h"
#include "GUIWindowManager.h"
#include "addons/Skin.h"
#include "GUIFontTTF.h"
#include "GUIFont.h"
#include "utils/XMLUtils.h"
#include "GUIControlFactory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "windowing/WindowingFactory.h"
#include "URL.h"

/* PLEX */
#include "plex/PlexMacUtils.h"
/* END PLEX */

using namespace std;

GUIFontManager::GUIFontManager(void)
{
  m_fontsetUnicode=false;
  m_canReload = true;
}

GUIFontManager::~GUIFontManager(void)
{
  Clear();
}

void GUIFontManager::RescaleFontSizeAndAspect(float *size, float *aspect, const RESOLUTION_INFO &sourceRes, bool preserveAspect) const
{
  // set scaling resolution so that we can scale our font sizes correctly
  // as fonts aren't scaled at render time (due to aliasing) we must scale
  // the size of the fonts before they are drawn to bitmaps
  g_graphicsContext.SetScalingResolution(sourceRes, true);

  if (preserveAspect)
  {
    // font always displayed in the aspect specified by the aspect parameter
    *aspect /= g_graphicsContext.GetPixelRatio(g_graphicsContext.GetVideoResolution());
  }
  else
  {
    // font streched like the rest of the UI, aspect parameter being the original aspect

    // adjust aspect ratio
    *aspect *= sourceRes.fPixelRatio;

    *aspect *= g_graphicsContext.GetGUIScaleY() / g_graphicsContext.GetGUIScaleX();
  }

  *size /= g_graphicsContext.GetGUIScaleY();
}

static bool CheckFont(CStdString& strPath, const CStdString& newPath,
                      const CStdString& filename)
{
  if (!XFILE::CFile::Exists(strPath))
  {
    strPath = URIUtils::AddFileToFolder(newPath,filename);
#ifdef _LINUX
    strPath = CSpecialProtocol::TranslatePathConvertCase(strPath);
#endif
    return false;
  }

  return true;
}

#ifndef __PLEX__
CGUIFont* GUIFontManager::LoadTTF(const CStdString& strFontName, const CStdString& strFilename, color_t textColor, color_t shadowColor, const int iSize, const int iStyle, bool border, float lineSpacing, float aspect, const RESOLUTION_INFO *sourceRes, bool preserveAspect)
#else
CGUIFont* GUIFontManager::LoadTTF(const CStdString& strFontName, const CStdString& strFilename, color_t textColor, color_t shadowColor, const int iSize, int iStyle, bool border, float lineSpacing, float aspect, RESOLUTION_INFO *sourceRes, bool preserveAspect, const CStdString& variant)
#endif
{
  float originalAspect = aspect;

  //check if font already exists
  CGUIFont* pFont = GetFont(strFontName, false);
  if (pFont)
    return pFont;

  if (!sourceRes) // no source res specified, so assume the skin res
    sourceRes = &m_skinResolution;

  float newSize = (float)iSize;
  RescaleFontSizeAndAspect(&newSize, &aspect, *sourceRes, preserveAspect);

  // First try to load the font from the skin
  CStdString strPath;
  if (!CURL::IsFullPath(strFilename))
  {
    strPath = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), "fonts");
    strPath = URIUtils::AddFileToFolder(strPath, strFilename);
  }
  else
    strPath = strFilename;

#ifdef _LINUX
  strPath = CSpecialProtocol::TranslatePathConvertCase(strPath);
#endif

  // Check if the file exists, otherwise try loading it from the global media dir
  CStdString file = URIUtils::GetFileName(strFilename);
  if (!CheckFont(strPath,"special://home/media/Fonts",file))
    CheckFont(strPath,"special://xbmc/media/Fonts",file);

  /* PLEX */
  if (!FindSystemFontPath(URIUtils::GetFileName(strFilename), &strPath))
  {
    #ifdef TARGET_DARWIN_OSX
    CStdString fontPath = PlexMacUtils::GetSystemFontPathFromDisplayName(strFilename);
    if (XFILE::CFile::Exists(fontPath))
      strPath = fontPath;
    #endif
  }
  /* END PLEX */

  // check if we already have this font file loaded (font object could differ only by color or style)
  CStdString TTFfontName;
  TTFfontName.Format("%s_%f_%f%s", strFilename, newSize, aspect, border ? "_border" : "");

  CGUIFontTTFBase* pFontFile = GetFontFile(TTFfontName);
  if (!pFontFile)
  {
    pFontFile = new CGUIFontTTF(TTFfontName);
#ifndef __PLEX__
    bool bFontLoaded = pFontFile->Load(strPath, newSize, aspect, 1.0f, border);
#else
    bool bFontLoaded = pFontFile->Load(strPath, newSize, aspect, 1.0f, border, variant);
#endif

    if (!bFontLoaded)
    {
      delete pFontFile;

      /* PLEX */
      CStdString fontAlias;
      CLog::Log(LOGINFO, "Looking for alias...");
      if (GetFontAlias(strFilename, variant, fontAlias, iStyle))
        return LoadTTF(strFontName, fontAlias, textColor, shadowColor, iSize, iStyle, border, lineSpacing, originalAspect);
      /* END PLEX */

      // font could not be loaded - try Arial.ttf, which we distribute
      if (strFilename != "arial.ttf")
      {
        CLog::Log(LOGERROR, "Couldn't load font name: %s(%s), trying to substitute arial.ttf", strFontName.c_str(), strFilename.c_str());
        return LoadTTF(strFontName, "arial.ttf", textColor, shadowColor, iSize, iStyle, border, lineSpacing, originalAspect);
      }
      CLog::Log(LOGERROR, "Couldn't load font name:%s file:%s", strFontName.c_str(), strPath.c_str());

      return NULL;
    }

    m_vecFontFiles.push_back(pFontFile);
  }

  // font file is loaded, create our CGUIFont
  CGUIFont *pNewFont = new CGUIFont(strFontName, iStyle, textColor, shadowColor, lineSpacing, (float)iSize, pFontFile);
  m_vecFonts.push_back(pNewFont);

  // Store the original TTF font info in case we need to reload it in a different resolution
  OrigFontInfo fontInfo;
  fontInfo.size = iSize;
  fontInfo.aspect = originalAspect;
  fontInfo.fontFilePath = strPath;
  fontInfo.fileName = strFilename;
  fontInfo.sourceRes = *sourceRes;
  fontInfo.preserveAspect = preserveAspect;
  fontInfo.border = border;
  /* PLEX */
  fontInfo.variant = variant;
  /* END PLEX */
  m_vecFontInfo.push_back(fontInfo);

  return pNewFont;
}

bool GUIFontManager::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() != GUI_MSG_NOTIFY_ALL)
    return false;

  if (message.GetParam1() == GUI_MSG_RENDERER_LOST)
  {
    m_canReload = false;
    return true;
  }

  if (message.GetParam1() == GUI_MSG_RENDERER_RESET)
  { // our device has been reset - we have to reload our ttf fonts, and send
    // a message to controls that we have done so
    ReloadTTFFonts();
    g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESIZE);
    m_canReload = true;
    return true;
  }

  if (message.GetParam1() == GUI_MSG_WINDOW_RESIZE)
  { // we need to reload our fonts
    if (m_canReload)
    {
      ReloadTTFFonts();
      // no need to send a resize message, as this message will do the rounds
      return true;
    }
  }
  return false;
}

void GUIFontManager::ReloadTTFFonts(void)
{
  if (!m_vecFonts.size())
    return;   // we haven't even loaded fonts in yet

  for (unsigned int i = 0; i < m_vecFonts.size(); i++)
  {
    CGUIFont* font = m_vecFonts[i];
    OrigFontInfo fontInfo = m_vecFontInfo[i];

    float aspect = fontInfo.aspect;
    float newSize = (float)fontInfo.size;
    CStdString& strPath = fontInfo.fontFilePath;
    CStdString& strFilename = fontInfo.fileName;
    /* PLEX */
    CStdString& variant = fontInfo.variant;
    /* END PLEX */

    RescaleFontSizeAndAspect(&newSize, &aspect, fontInfo.sourceRes, fontInfo.preserveAspect);

    CStdString TTFfontName;
    TTFfontName.Format("%s_%f_%f%s", strFilename, newSize, aspect, fontInfo.border ? "_border" : "");
    CGUIFontTTFBase* pFontFile = GetFontFile(TTFfontName);
    if (!pFontFile)
    {
      pFontFile = new CGUIFontTTF(TTFfontName);
#ifndef __PLEX__
      if (!pFontFile || !pFontFile->Load(strPath, newSize, aspect, 1.0f, fontInfo.border))
#else
      if (!pFontFile || !pFontFile->Load(strPath, newSize, aspect, 1.0f, fontInfo.border, variant))
#endif
      {
        delete pFontFile;
        // font could not be loaded
        CLog::Log(LOGERROR, "Couldn't re-load font file:%s", strPath.c_str());
        return;
      }

      m_vecFontFiles.push_back(pFontFile);
    }

    font->SetFont(pFontFile);
  }
}

void GUIFontManager::UnloadTTFFonts()
{
  for (vector<CGUIFontTTFBase*>::iterator i = m_vecFontFiles.begin(); i != m_vecFontFiles.end(); i++)
    delete (*i);

  m_vecFontFiles.clear();

  for (vector<CGUIFont*>::iterator i = m_vecFonts.begin(); i != m_vecFonts.end(); i++)
    (*i)->SetFont(NULL);
}

void GUIFontManager::Unload(const CStdString& strFontName)
{
  for (vector<CGUIFont*>::iterator iFont = m_vecFonts.begin(); iFont != m_vecFonts.end(); ++iFont)
  {
    if ((*iFont)->GetFontName().Equals(strFontName))
    {
      delete (*iFont);
      m_vecFonts.erase(iFont);
      return;
    }
  }
}

void GUIFontManager::FreeFontFile(CGUIFontTTFBase *pFont)
{
  for (vector<CGUIFontTTFBase*>::iterator it = m_vecFontFiles.begin(); it != m_vecFontFiles.end(); ++it)
  {
    if (pFont == *it)
    {
      m_vecFontFiles.erase(it);
      delete pFont;
      return;
    }
  }
}

CGUIFontTTFBase* GUIFontManager::GetFontFile(const CStdString& strFileName)
{
  for (int i = 0; i < (int)m_vecFontFiles.size(); ++i)
  {
    CGUIFontTTFBase* pFont = (CGUIFontTTFBase *)m_vecFontFiles[i];
    if (pFont->GetFileName().Equals(strFileName))
      return pFont;
  }
  return NULL;
}

CGUIFont* GUIFontManager::GetFont(const CStdString& strFontName, bool fallback /*= true*/)
{
  for (int i = 0; i < (int)m_vecFonts.size(); ++i)
  {
    CGUIFont* pFont = m_vecFonts[i];
    if (pFont->GetFontName().Equals(strFontName))
      return pFont;
  }
  // fall back to "font13" if we have none
  if (fallback && !strFontName.IsEmpty() && !strFontName.Equals("-") && !strFontName.Equals("font13"))
    return GetFont("font13");
  return NULL;
}

CGUIFont* GUIFontManager::GetDefaultFont(bool border)
{
  // first find "font13" or "__defaultborder__"
  unsigned int font13index = m_vecFonts.size();
  CGUIFont *font13border = NULL;
  for (unsigned int i = 0; i < m_vecFonts.size(); i++)
  {
    CGUIFont* font = m_vecFonts[i];
    if (font->GetFontName() == "font13")
      font13index = i;
    else if (font->GetFontName() == "__defaultborder__")
      font13border = font;
  }
  // no "font13" means no default font is found - use the first font found.
  if (font13index == m_vecFonts.size())
  {
    if (m_vecFonts.empty())
      return NULL;
    font13index = 0;
  }

  if (border)
  {
    if (!font13border)
    { // create it
      CGUIFont *font13 = m_vecFonts[font13index];
      OrigFontInfo fontInfo = m_vecFontInfo[font13index];
      font13border = LoadTTF("__defaultborder__", fontInfo.fileName, 0xFF000000, 0, fontInfo.size, font13->GetStyle(), true, 1.0f, fontInfo.aspect, &fontInfo.sourceRes, fontInfo.preserveAspect);
    }
    return font13border;
  }
  return m_vecFonts[font13index];
}

void GUIFontManager::Clear()
{
  for (int i = 0; i < (int)m_vecFonts.size(); ++i)
  {
    CGUIFont* pFont = m_vecFonts[i];
#ifndef __PLEX__ // I just can't track down this crash :(
    delete pFont;
#endif
  }

  m_vecFonts.clear();
  m_vecFontFiles.clear();
  m_vecFontInfo.clear();
  m_fontsetUnicode=false;

  /* PLEX */
  m_fontAliasMap.clear();
  /* END PLEX */
}

void GUIFontManager::LoadFonts(const CStdString& strFontSet)
{
  CXBMCTinyXML xmlDoc;
  if (!OpenFontFile(xmlDoc))
    return;

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  const TiXmlNode *pChild = pRootElement->FirstChild();

  // If there are no fontset's defined in the XML (old skin format) run in backward compatibility
  // and ignore the fontset request
  CStdString strValue = pChild->Value();
#ifndef __PLEX__
  if (strValue == "fontset")
#else
  if (strValue == "fontset" || strValue == "aliases")
#endif
  {
    CStdString foundTTF;
    while (pChild)
    {
      strValue = pChild->Value();
      if (strValue == "fontset")
      {
        const char* idAttr = ((TiXmlElement*) pChild)->Attribute("id");

        const char* unicodeAttr = ((TiXmlElement*) pChild)->Attribute("unicode");

        if (foundTTF.IsEmpty() && idAttr != NULL && unicodeAttr != NULL && stricmp(unicodeAttr, "true") == 0)
          foundTTF = idAttr;

        // Check if this is the fontset that we want
        if (idAttr != NULL && stricmp(strFontSet.c_str(), idAttr) == 0)
        {
          m_fontsetUnicode=false;
          // Check if this is the a ttf fontset
          if (unicodeAttr != NULL && stricmp(unicodeAttr, "true") == 0)
            m_fontsetUnicode=true;

#ifndef __PLEX__
          if (m_fontsetUnicode)
#endif
          {
            LoadFonts(pChild->FirstChild());
            break;
          }
        }
      }

      /* PLEX */
      else if (strValue == "aliases")
      {
        const TiXmlNode* pAliasChild = pChild->FirstChild();
        while (pAliasChild)
        {
          const TiXmlNode *pCheckAlias = pAliasChild;
          pAliasChild = pAliasChild->NextSibling();

          if (CStdString(pCheckAlias->Value()) == "alias")
          {
            const char* font = ((TiXmlElement*) pCheckAlias)->Attribute("font");
            const char* variant = ((TiXmlElement*) pCheckAlias)->Attribute("variant");
            const char* alias = ((TiXmlElement*) pCheckAlias)->Attribute("alias");
            const char* aliasStyle = ((TiXmlElement *)pCheckAlias)->Attribute("aliasStyle");

            if (font && variant && alias)
            {
              string key = string(font) + "/" + string(variant);

              int iAliasStyle = FONT_STYLE_NORMAL;
              if (aliasStyle)
              {
                CStdString style(aliasStyle);
                if (style == "bold")
                  iAliasStyle = FONT_STYLE_BOLD;
                else if (style == "italics")
                  iAliasStyle = FONT_STYLE_ITALICS;
                else if (style == "bolditalics")
                  iAliasStyle = FONT_STYLE_BOLD | FONT_STYLE_ITALICS;
              }

              CLog::Log(LOGINFO, "Adding alias %s -> %s:%d", key.c_str(), alias, iAliasStyle);
              m_fontAliasMap[key] = pair<string, int>(alias, iAliasStyle);
            }
          }
        }
      }
      /* END PLEX */
      pChild = pChild->NextSibling();
    }

    // If no fontset was loaded
    if (pChild == NULL)
    {
      CLog::Log(LOGWARNING, "file doesnt have <fontset> with name '%s', defaulting to first fontset", strFontSet.c_str());
      if (!foundTTF.IsEmpty())
        LoadFonts(foundTTF);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "file doesnt have <fontset> in <fonts>, but rather %s", strValue.c_str());
    return ;
  }
}

void GUIFontManager::LoadFonts(const TiXmlNode* fontNode)
{
  while (fontNode)
  {
    CStdString strValue = fontNode->Value();
    if (strValue == "font")
    {
      const TiXmlNode *pNode = fontNode->FirstChild("name");
      if (pNode)
      {
        CStdString strFontName = pNode->FirstChild()->Value();
        color_t shadowColor = 0;
        color_t textColor = 0;
        CGUIControlFactory::GetColor(fontNode, "shadow", shadowColor);
        CGUIControlFactory::GetColor(fontNode, "color", textColor);
        const TiXmlNode *pNode = fontNode->FirstChild("filename");
        if (pNode)
        {
          CStdString strFontFileName = pNode->FirstChild()->Value();

          /* PLEX */
          CStdString extension = URIUtils::GetExtension(strFontFileName);
#ifdef __APPLE__
          if (extension.Equals(".ttf") || extension.Equals(".dfont") || extension.Equals(".otf") || extension.Equals(".ttc") || extension.length() == 0)
#else
          if (extension.Equals(".ttf") || extension.Equals(".dfont") || extension.Equals(".otf") || extension.length() == 0)
#endif
          /* END PLEX */
          {
            int iSize = 20;
            int iStyle = FONT_STYLE_NORMAL;
            float aspect = 1.0f;
            float lineSpacing = 1.0f;

            XMLUtils::GetInt(fontNode, "size", iSize);
            if (iSize <= 0) iSize = 20;

            pNode = fontNode->FirstChild("style");
            if (pNode && pNode->FirstChild())
            {
              vector<string> styles = StringUtils::Split(pNode->FirstChild()->ValueStr(), " ");
              for (vector<string>::iterator i = styles.begin(); i != styles.end(); ++i)
              {
                if (*i == "bold")
                  iStyle |= FONT_STYLE_BOLD;
                else if (*i == "italics")
                  iStyle |= FONT_STYLE_ITALICS;
                else if (*i == "bolditalics") // backward compatibility
                  iStyle |= (FONT_STYLE_BOLD | FONT_STYLE_ITALICS);
                else if (*i == "uppercase")
                  iStyle |= FONT_STYLE_UPPERCASE;
                else if (*i == "lowercase")
                  iStyle |= FONT_STYLE_LOWERCASE;
              }
            }

            /* PLEX */
            CStdString variant;
            pNode = fontNode->FirstChild("variant");
            if (pNode)
              variant = pNode->FirstChild()->Value();
            /* END PLEX */

            XMLUtils::GetFloat(fontNode, "linespacing", lineSpacing);
            XMLUtils::GetFloat(fontNode, "aspect", aspect);

#ifndef __PLEX__
            LoadTTF(strFontName, strFontFileName, textColor, shadowColor, iSize, iStyle, false, lineSpacing, aspect);
#else
            LoadTTF(strFontName, strFontFileName, textColor, shadowColor, iSize, iStyle, false, lineSpacing, aspect, NULL, false, variant);
#endif
          }
        }
      }
    }

    fontNode = fontNode->NextSibling();
  }
}

bool GUIFontManager::OpenFontFile(CXBMCTinyXML& xmlDoc)
{
  // Get the file to load fonts from:
  CStdString strPath = g_SkinInfo->GetSkinPath("Font.xml", &m_skinResolution);
  CLog::Log(LOGINFO, "Loading fonts from %s", strPath.c_str());

  // first try our preferred file
  if ( !xmlDoc.LoadFile(strPath) )
  {
    CLog::Log(LOGERROR, "Couldn't load %s", strPath.c_str());
    return false;
  }
  TiXmlElement* pRootElement = xmlDoc.RootElement();

  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("fonts"))
  {
    CLog::Log(LOGERROR, "file %s doesnt start with <fonts>", strPath.c_str());
    return false;
  }

  return true;
}

bool GUIFontManager::GetFirstFontSetUnicode(CStdString& strFontSet)
{
  strFontSet.Empty();

  // Load our font file
  CXBMCTinyXML xmlDoc;
  if (!OpenFontFile(xmlDoc))
    return false;

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  const TiXmlNode *pChild = pRootElement->FirstChild();

  CStdString strValue = pChild->Value();
  if (strValue == "fontset")
  {
    while (pChild)
    {
      strValue = pChild->Value();
      if (strValue == "fontset")
      {
        const char* idAttr = ((TiXmlElement*) pChild)->Attribute("id");

        const char* unicodeAttr = ((TiXmlElement*) pChild)->Attribute("unicode");

        // Check if this is a fontset with a ttf attribute set to true
        if (unicodeAttr != NULL && stricmp(unicodeAttr, "true") == 0)
        {
          //  This is the first ttf fontset
          strFontSet=idAttr;
          break;
        }

      }

      pChild = pChild->NextSibling();
    }

    // If no fontset was loaded
    if (pChild == NULL)
      CLog::Log(LOGWARNING, "file doesnt have <fontset> with attribute unicode=\"true\"");
  }
  else
  {
    CLog::Log(LOGERROR, "file doesnt have <fontset> in <fonts>, but rather %s", strValue.c_str());
  }

  return !strFontSet.IsEmpty();
}

bool GUIFontManager::IsFontSetUnicode(const CStdString& strFontSet)
{
  CXBMCTinyXML xmlDoc;
  if (!OpenFontFile(xmlDoc))
    return false;

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  const TiXmlNode *pChild = pRootElement->FirstChild();

  CStdString strValue = pChild->Value();
  if (strValue == "fontset")
  {
    while (pChild)
    {
      strValue = pChild->Value();
      if (strValue == "fontset")
      {
        const char* idAttr = ((TiXmlElement*) pChild)->Attribute("id");

        const char* unicodeAttr = ((TiXmlElement*) pChild)->Attribute("unicode");

        // Check if this is the fontset that we want
        if (idAttr != NULL && stricmp(strFontSet.c_str(), idAttr) == 0)
          return (unicodeAttr != NULL && stricmp(unicodeAttr, "true") == 0);

      }

      pChild = pChild->NextSibling();
    }
  }

  return false;
}

/* PLEX */
bool GUIFontManager::FindSystemFontPath(const CStdString& strFilename, CStdString *fontPath)
{
  vector<string> systemPaths;
  vector<string> fontExtensions;

#ifdef __APPLE__
  // TODO: Add all the sub folders in each of these system paths
  string home = getenv("HOME");
  if (*home.rbegin() == '/')
    home.erase(home.end()-1, home.end());

  systemPaths.push_back(home + "/Library/Fonts/");
  systemPaths.push_back("/Library/Fonts/");
  systemPaths.push_back("/System/Library/Fonts/");

  fontExtensions.push_back("");
#elif _WIN32
  systemPaths.push_back("C:\\Windows\\Fonts\\");
#endif

  fontExtensions.push_back(".ttf");
  fontExtensions.push_back(".dfont");
  fontExtensions.push_back(".ttc");
  fontExtensions.push_back(".otf");

  string foundPath;
  string foundFullPath;

  bool iterateExtensions = (URIUtils::GetExtension(strFilename).length() == 0);
  for (unsigned i = 0; i < systemPaths.size(); i++)
  {
    foundPath = systemPaths[i] + strFilename.c_str();
    for (unsigned j = 0; j < fontExtensions.size(); j++)
    {
      foundFullPath = foundPath;
      if (iterateExtensions && fontExtensions[j].size() != 0)
        foundFullPath += fontExtensions[j];

#ifdef _LINUX
      foundFullPath = CSpecialProtocol::TranslatePathConvertCase(foundFullPath);
#endif

      if (XFILE::CFile::Exists(foundFullPath))
      {
        *fontPath = foundFullPath.c_str();
        return TRUE;
      }

      if (!iterateExtensions)
        break;
    }
  }
  return FALSE;
}

std::vector<std::string> GUIFontManager::GetSystemFontNames()
{
#ifndef TARGET_DARWIN_OSX
#ifdef _MSC_VER
#pragma message(__WARNING__"TODO: IMPLEMENT ME")
#else
#pragma message("TODO: IMPLEMENT ME")
#endif
  return std::vector<std::string>();
#else
  // should we add all the fonts found in the skin's path?
  return PlexMacUtils::GetSystemFonts();
#endif
}

bool GUIFontManager::GetFontAlias(const CStdString& strFontName, const CStdString& strVariant, CStdString& strAlias, int& aliasStyle)
{
  string key = strFontName + "/" + strVariant;
  if (m_fontAliasMap.find(key) != m_fontAliasMap.end())
  {
    pair<string, int> aliasPair = m_fontAliasMap[key];
    strAlias = aliasPair.first;
    aliasStyle = aliasPair.second;

    return true;
  }

  return false;
}

/* END PLEX */
