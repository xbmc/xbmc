/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIComponent.h"
#include "GUIFontManager.h"
#include "windowing/GraphicContext.h"
#include "GUIWindowManager.h"
#include "addons/Skin.h"
#include "addons/AddonManager.h"
#include "addons/FontResource.h"
#include "GUIFontTTF.h"
#if defined(HAS_GLES) || defined (HAS_GL)
#include "GUIFontTTFGL.h"
#endif
#include "GUIFont.h"
#include "utils/XMLUtils.h"
#include "GUIControlFactory.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "FileItem.h"
#include "URL.h"
#include "ServiceBroker.h"

#ifdef TARGET_POSIX
#include "filesystem/SpecialProtocol.h"
#endif

using namespace ADDON;

GUIFontManager::GUIFontManager(void)
{
  m_canReload = true;
}

GUIFontManager::~GUIFontManager(void)
{
  Clear();
}

void GUIFontManager::RescaleFontSizeAndAspect(float *size, float *aspect, const RESOLUTION_INFO &sourceRes, bool preserveAspect)
{
  // get the UI scaling constants so that we can scale our font sizes correctly
  // as fonts aren't scaled at render time (due to aliasing) we must scale
  // the size of the fonts before they are drawn to bitmaps
  float scaleX, scaleY;
  CServiceBroker::GetWinSystem()->GetGfxContext().GetGUIScaling(sourceRes, scaleX, scaleY);

  if (preserveAspect)
  {
    // font always displayed in the aspect specified by the aspect parameter
    *aspect /= CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo().fPixelRatio;
  }
  else
  {
    // font stretched like the rest of the UI, aspect parameter being the original aspect

    // adjust aspect ratio
    *aspect *= sourceRes.fPixelRatio;

    *aspect *= scaleY / scaleX;
  }

  *size /= scaleY;
}

static bool CheckFont(std::string& strPath, const std::string& newPath,
                      const std::string& filename)
{
  if (!XFILE::CFile::Exists(strPath))
  {
    strPath = URIUtils::AddFileToFolder(newPath,filename);
#ifdef TARGET_POSIX
    strPath = CSpecialProtocol::TranslatePathConvertCase(strPath);
#endif
    return false;
  }

  return true;
}

CGUIFont* GUIFontManager::LoadTTF(const std::string& strFontName, const std::string& strFilename, UTILS::Color textColor, UTILS::Color shadowColor, const int iSize, const int iStyle, bool border, float lineSpacing, float aspect, const RESOLUTION_INFO *sourceRes, bool preserveAspect)
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
  std::string strPath;
  if (!CURL::IsFullPath(strFilename))
  {
    strPath = URIUtils::AddFileToFolder(CServiceBroker::GetWinSystem()->GetGfxContext().GetMediaDir(), "fonts", strFilename);
  }
  else
    strPath = strFilename;

#ifdef TARGET_POSIX
  strPath = CSpecialProtocol::TranslatePathConvertCase(strPath);
#endif

  // Check if the file exists, otherwise try loading it from the global media dir
  std::string file = URIUtils::GetFileName(strFilename);
  if (!CheckFont(strPath, "special://home/media/Fonts", file) &&
      !CheckFont(strPath, "special://xbmc/media/Fonts", file))
  {
    VECADDONS addons;
    CServiceBroker::GetAddonMgr().GetAddons(addons, ADDON_RESOURCE_FONT);
    for (auto& it : addons)
    {
      std::shared_ptr<CFontResource> font(std::static_pointer_cast<CFontResource>(it));
      if (font->GetFont(file, strPath))
        break;
    }
  }

  // check if we already have this font file loaded (font object could differ only by color or style)
  std::string TTFfontName = StringUtils::Format("%s_%f_%f%s", strFilename.c_str(), newSize, aspect, border ? "_border" : "");

  CGUIFontTTF* pFontFile = GetFontFile(TTFfontName);
  if (!pFontFile)
  {
    pFontFile = CGUIFontTTF::CreateGUIFontTTF(TTFfontName);
    bool bFontLoaded = pFontFile->Load(strPath, newSize, aspect, 1.0f, border);

    if (!bFontLoaded)
    {
      delete pFontFile;

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
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESIZE);
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
    std::string& strPath = fontInfo.fontFilePath;
    std::string& strFilename = fontInfo.fileName;

    RescaleFontSizeAndAspect(&newSize, &aspect, fontInfo.sourceRes, fontInfo.preserveAspect);

    std::string TTFfontName = StringUtils::Format("%s_%f_%f%s", strFilename.c_str(), newSize, aspect, fontInfo.border ? "_border" : "");
    CGUIFontTTF* pFontFile = GetFontFile(TTFfontName);
    if (!pFontFile)
    {
      pFontFile = CGUIFontTTF::CreateGUIFontTTF(TTFfontName);
      if (!pFontFile || !pFontFile->Load(strPath, newSize, aspect, 1.0f, fontInfo.border))
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

void GUIFontManager::Unload(const std::string& strFontName)
{
  for (std::vector<CGUIFont*>::iterator iFont = m_vecFonts.begin(); iFont != m_vecFonts.end(); ++iFont)
  {
    if (StringUtils::EqualsNoCase((*iFont)->GetFontName(), strFontName))
    {
      delete (*iFont);
      m_vecFonts.erase(iFont);
      return;
    }
  }
}

void GUIFontManager::FreeFontFile(CGUIFontTTF* pFont)
{
  for (std::vector<CGUIFontTTF*>::iterator it = m_vecFontFiles.begin(); it != m_vecFontFiles.end();
       ++it)
  {
    if (pFont == *it)
    {
      m_vecFontFiles.erase(it);
      delete pFont;
      return;
    }
  }
}

CGUIFontTTF* GUIFontManager::GetFontFile(const std::string& strFileName)
{
  for (int i = 0; i < (int)m_vecFontFiles.size(); ++i)
  {
    CGUIFontTTF* pFont = m_vecFontFiles[i];
    if (StringUtils::EqualsNoCase(pFont->GetFileName(), strFileName))
      return pFont;
  }
  return NULL;
}

CGUIFont* GUIFontManager::GetFont(const std::string& strFontName, bool fallback /*= true*/)
{
  for (int i = 0; i < (int)m_vecFonts.size(); ++i)
  {
    CGUIFont* pFont = m_vecFonts[i];
    if (StringUtils::EqualsNoCase(pFont->GetFontName(), strFontName))
      return pFont;
  }
  // fall back to "font13" if we have none
  if (fallback && !strFontName.empty() && !StringUtils::EqualsNoCase(strFontName, "font13"))
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
    delete pFont;
  }

  m_vecFonts.clear();
  m_vecFontFiles.clear();
  m_vecFontInfo.clear();

#if defined(HAS_GLES) || defined (HAS_GL)
  CGUIFontTTFGL::DestroyStaticVertexBuffers();
#endif
}

void GUIFontManager::LoadFonts(const std::string& fontSet)
{
  // Get the file to load fonts from:
  const std::string strPath = g_SkinInfo->GetSkinPath("Font.xml", &m_skinResolution);
  CLog::Log(LOGINFO, "Loading fonts from %s", strPath.c_str());

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "Couldn't load %s", strPath.c_str());
    return;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || pRootElement->ValueStr() != "fonts")
  {
    CLog::Log(LOGERROR, "file %s doesn't start with <fonts>", strPath.c_str());
    return;
  }
  // Resolve includes in Font.xml
  g_SkinInfo->ResolveIncludes(pRootElement);
  // take note of the first font available in case we can't load the one specified
  std::string firstFont;

  const TiXmlElement *pChild = pRootElement->FirstChildElement("fontset");
  while (pChild)
  {
    const char* idAttr = pChild->Attribute("id");
    if (idAttr)
    {
      if (firstFont.empty())
        firstFont = idAttr;

      if (StringUtils::EqualsNoCase(fontSet, idAttr))
      {
        LoadFonts(pChild->FirstChild("font"));
        return;
      }
    }
    pChild = pChild->NextSiblingElement("fontset");
  }

  // no fontset was loaded, try the first
  if (!firstFont.empty())
  {
    CLog::Log(LOGWARNING, "file doesn't have <fontset> with name '%s', defaulting to first fontset", fontSet.c_str());
    LoadFonts(firstFont);
  }
  else
    CLog::Log(LOGERROR, "file '%s' doesn't have a valid <fontset>", strPath.c_str());
}

void GUIFontManager::LoadFonts(const TiXmlNode* fontNode)
{
  while (fontNode)
  {
    std::string fontName;
    std::string fileName;
    int iSize = 20;
    float aspect = 1.0f;
    float lineSpacing = 1.0f;
    UTILS::Color shadowColor = 0;
    UTILS::Color textColor = 0;
    int iStyle = FONT_STYLE_NORMAL;

    XMLUtils::GetString(fontNode, "name", fontName);
    XMLUtils::GetInt(fontNode, "size", iSize);
    XMLUtils::GetFloat(fontNode, "linespacing", lineSpacing);
    XMLUtils::GetFloat(fontNode, "aspect", aspect);
    CGUIControlFactory::GetColor(fontNode, "shadow", shadowColor);
    CGUIControlFactory::GetColor(fontNode, "color", textColor);
    XMLUtils::GetString(fontNode, "filename", fileName);
    GetStyle(fontNode, iStyle);

    if (!fontName.empty() && URIUtils::HasExtension(fileName, ".ttf"))
    {
      //! @todo Why do we tolower() this shit?
      std::string strFontFileName = fileName;
      StringUtils::ToLower(strFontFileName);
      LoadTTF(fontName, strFontFileName, textColor, shadowColor, iSize, iStyle, false, lineSpacing, aspect);
    }
    fontNode = fontNode->NextSibling("font");
  }
}

void GUIFontManager::GetStyle(const TiXmlNode *fontNode, int &iStyle)
{
  std::string style;
  iStyle = FONT_STYLE_NORMAL;
  if (XMLUtils::GetString(fontNode, "style", style))
  {
    std::vector<std::string> styles = StringUtils::Tokenize(style, " ");
    for (const std::string& i : styles)
    {
      if (i == "bold")
        iStyle |= FONT_STYLE_BOLD;
      else if (i == "italics")
        iStyle |= FONT_STYLE_ITALICS;
      else if (i == "bolditalics") // backward compatibility
        iStyle |= (FONT_STYLE_BOLD | FONT_STYLE_ITALICS);
      else if (i == "uppercase")
        iStyle |= FONT_STYLE_UPPERCASE;
      else if (i == "lowercase")
        iStyle |= FONT_STYLE_LOWERCASE;
      else if (i == "capitalize")
        iStyle |= FONT_STYLE_CAPITALIZE;
      else if (i == "lighten")
        iStyle |= FONT_STYLE_LIGHT;
    }
  }
}

void GUIFontManager::SettingOptionsFontsFiller(const SettingConstPtr& setting,
                                               std::vector<StringSettingOption>& list,
                                               std::string& current,
                                               void* data)
{
  CFileItemList items;
  CFileItemList items2;

  // find TTF fonts
  XFILE::CDirectory::GetDirectory("special://home/media/Fonts/", items2, "", XFILE::DIR_FLAG_DEFAULTS);

  if (XFILE::CDirectory::GetDirectory("special://xbmc/media/Fonts/", items, "", XFILE::DIR_FLAG_DEFAULTS))
  {
    items.Append(items2);
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr pItem = items[i];

      if (!pItem->m_bIsFolder
          && URIUtils::HasExtension(pItem->GetLabel(), ".ttf"))
      {
        list.emplace_back(pItem->GetLabel(), pItem->GetLabel());
      }
    }
  }
}
