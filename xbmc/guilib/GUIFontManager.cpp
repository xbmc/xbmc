/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFontManager.h"

#include "GUIComponent.h"
#include "GUIFontTTF.h"
#include "GUIWindowManager.h"
#include "addons/AddonManager.h"
#include "addons/FontResource.h"
#include "addons/Skin.h"
#include "windowing/GraphicContext.h"
#if defined(HAS_GLES) || defined(HAS_GL)
#include "GUIFontTTFGL.h"
#endif
#include "FileItem.h"
#include "GUIControlFactory.h"
#include "GUIFont.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#ifdef TARGET_POSIX
#include "filesystem/SpecialProtocol.h"
#endif

using namespace ADDON;

GUIFontManager::GUIFontManager() = default;

GUIFontManager::~GUIFontManager()
{
  Clear();
}

void GUIFontManager::RescaleFontSizeAndAspect(CGraphicContext& context,
                                              float* size,
                                              float* aspect,
                                              const RESOLUTION_INFO& sourceRes,
                                              bool preserveAspect)
{
  // get the UI scaling constants so that we can scale our font sizes correctly
  // as fonts aren't scaled at render time (due to aliasing) we must scale
  // the size of the fonts before they are drawn to bitmaps
  float scaleX, scaleY;
  context.GetGUIScaling(sourceRes, scaleX, scaleY);

  if (preserveAspect)
  {
    // font always displayed in the aspect specified by the aspect parameter
    *aspect /= context.GetResInfo().fPixelRatio;
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

static bool CheckFont(std::string& strPath, const std::string& newPath, const std::string& filename)
{
  if (!XFILE::CFile::Exists(strPath))
  {
    strPath = URIUtils::AddFileToFolder(newPath, filename);
#ifdef TARGET_POSIX
    strPath = CSpecialProtocol::TranslatePathConvertCase(strPath);
#endif
    return false;
  }

  return true;
}

CGUIFont* GUIFontManager::LoadTTF(const std::string& strFontName,
                                  const std::string& strFilename,
                                  UTILS::COLOR::Color textColor,
                                  UTILS::COLOR::Color shadowColor,
                                  const int iSize,
                                  const int iStyle,
                                  bool border,
                                  float lineSpacing,
                                  float aspect,
                                  const RESOLUTION_INFO* sourceRes,
                                  bool preserveAspect)
{
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
  {
    CLog::Log(LOGFATAL,
              "GUIFontManager::{}: Something tries to call function without an available GUI "
              "window system",
              __func__);
    return nullptr;
  }

  CGraphicContext& context = winSystem->GetGfxContext();

  float originalAspect = aspect;

  //check if font already exists
  CGUIFont* pFont = GetFont(strFontName, false);
  if (pFont)
    return pFont;

  if (!sourceRes) // no source res specified, so assume the skin res
    sourceRes = &m_skinResolution;

  float newSize = static_cast<float>(iSize);
  RescaleFontSizeAndAspect(context, &newSize, &aspect, *sourceRes, preserveAspect);

  // First try to load the font from the skin
  std::string strPath;
  if (!CURL::IsFullPath(strFilename))
  {
    strPath = URIUtils::AddFileToFolder(context.GetMediaDir(), "fonts", strFilename);
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
  const std::string fontIdent =
      StringUtils::Format("{}_{:f}_{:f}{}", strFilename, newSize, aspect, border ? "_border" : "");

  CGUIFontTTF* pFontFile = GetFontFile(fontIdent);
  if (!pFontFile)
  {
    pFontFile = CGUIFontTTF::CreateGUIFontTTF(fontIdent);
    bool bFontLoaded = pFontFile->Load(strPath, newSize, aspect, 1.0f, border);

    if (!bFontLoaded)
    {
      delete pFontFile;

      // font could not be loaded - try Arial.ttf, which we distribute
      if (strFilename != "arial.ttf")
      {
        CLog::Log(LOGERROR, "GUIFontManager::{}: Couldn't load font name: {}({}), trying arial.ttf",
                  __func__, strFontName, strFilename);
        return LoadTTF(strFontName, "arial.ttf", textColor, shadowColor, iSize, iStyle, border,
                       lineSpacing, originalAspect);
      }
      CLog::Log(LOGERROR, "GUIFontManager::{}: Couldn't load font name:{} file:{}", __func__,
                strFontName, strPath);

      return nullptr;
    }

    m_vecFontFiles.emplace_back(pFontFile);
  }

  // font file is loaded, create our CGUIFont
  CGUIFont* pNewFont = new CGUIFont(strFontName, iStyle, textColor, shadowColor, lineSpacing,
                                    static_cast<float>(iSize), pFontFile);
  m_vecFonts.emplace_back(pNewFont);

  // Store the original TTF font info in case we need to reload it in a different resolution
  OrigFontInfo fontInfo;
  fontInfo.size = iSize;
  fontInfo.aspect = originalAspect;
  fontInfo.fontFilePath = strPath;
  fontInfo.fileName = strFilename;
  fontInfo.sourceRes = *sourceRes;
  fontInfo.preserveAspect = preserveAspect;
  fontInfo.border = border;
  m_vecFontInfo.emplace_back(fontInfo);

  return pNewFont;
}

bool GUIFontManager::OnMessage(CGUIMessage& message)
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
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0,
                                                             GUI_MSG_WINDOW_RESIZE);
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
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (m_vecFonts.empty() || !winSystem)
    return; // we haven't even loaded fonts in yet

  for (size_t i = 0; i < m_vecFonts.size(); ++i)
  {
    const auto& font = m_vecFonts[i];
    OrigFontInfo fontInfo = m_vecFontInfo[i];

    float aspect = fontInfo.aspect;
    float newSize = static_cast<float>(fontInfo.size);
    std::string& strPath = fontInfo.fontFilePath;
    std::string& strFilename = fontInfo.fileName;

    RescaleFontSizeAndAspect(winSystem->GetGfxContext(), &newSize, &aspect, fontInfo.sourceRes,
                             fontInfo.preserveAspect);

    const std::string fontIdent = StringUtils::Format("{}_{:f}_{:f}{}", strFilename, newSize,
                                                      aspect, fontInfo.border ? "_border" : "");
    CGUIFontTTF* pFontFile = GetFontFile(fontIdent);
    if (!pFontFile)
    {
      pFontFile = CGUIFontTTF::CreateGUIFontTTF(fontIdent);
      if (!pFontFile || !pFontFile->Load(strPath, newSize, aspect, 1.0f, fontInfo.border))
      {
        delete pFontFile;
        // font could not be loaded
        CLog::Log(LOGERROR, "GUIFontManager::{}: Couldn't re-load font file: '{}'", __func__,
                  strPath);
        return;
      }

      m_vecFontFiles.emplace_back(pFontFile);
    }

    font->SetFont(pFontFile);
  }
}

void GUIFontManager::Unload(const std::string& strFontName)
{
  for (auto iFont = m_vecFonts.begin(); iFont != m_vecFonts.end(); ++iFont)
  {
    if (StringUtils::EqualsNoCase((*iFont)->GetFontName(), strFontName))
    {
      m_vecFonts.erase(iFont);
      return;
    }
  }
}

void GUIFontManager::FreeFontFile(CGUIFontTTF* pFont)
{
  for (auto it = m_vecFontFiles.begin(); it != m_vecFontFiles.end(); ++it)
  {
    if (pFont == it->get())
    {
      m_vecFontFiles.erase(it);
      return;
    }
  }
}

CGUIFontTTF* GUIFontManager::GetFontFile(const std::string& fontIdent)
{
  for (const auto& it : m_vecFontFiles)
  {
    if (StringUtils::EqualsNoCase(it->GetFontIdent(), fontIdent))
      return it.get();
  }

  return nullptr;
}

CGUIFont* GUIFontManager::GetFont(const std::string& strFontName, bool fallback /*= true*/)
{
  for (const auto& it : m_vecFonts)
  {
    CGUIFont* pFont = it.get();
    if (StringUtils::EqualsNoCase(pFont->GetFontName(), strFontName))
      return pFont;
  }

  // fall back to "font13" if we have none
  if (fallback && !strFontName.empty() && !StringUtils::EqualsNoCase(strFontName, "font13"))
    return GetFont("font13");

  return nullptr;
}

CGUIFont* GUIFontManager::GetDefaultFont(bool border)
{
  // first find "font13" or "__defaultborder__"
  size_t font13index = m_vecFonts.size();
  CGUIFont* font13border = nullptr;
  for (size_t i = 0; i < m_vecFonts.size(); i++)
  {
    CGUIFont* font = m_vecFonts[i].get();
    if (font->GetFontName() == "font13")
      font13index = i;
    else if (font->GetFontName() == "__defaultborder__")
      font13border = font;
  }
  // no "font13" means no default font is found - use the first font found.
  if (font13index == m_vecFonts.size())
  {
    if (m_vecFonts.empty())
      return nullptr;

    font13index = 0;
  }

  if (border)
  {
    if (!font13border)
    { // create it
      const auto& font13 = m_vecFonts[font13index];
      OrigFontInfo fontInfo = m_vecFontInfo[font13index];
      font13border = LoadTTF("__defaultborder__", fontInfo.fileName, UTILS::COLOR::BLACK, 0,
                             fontInfo.size, font13->GetStyle(), true, 1.0f, fontInfo.aspect,
                             &fontInfo.sourceRes, fontInfo.preserveAspect);
    }
    return font13border;
  }

  return m_vecFonts[font13index].get();
}

void GUIFontManager::Clear()
{
  m_vecFonts.clear();
  m_vecFontFiles.clear();
  m_vecFontInfo.clear();

#if defined(HAS_GLES) || defined(HAS_GL)
  CGUIFontTTFGL::DestroyStaticVertexBuffers();
#endif
}

void GUIFontManager::LoadFonts(const std::string& fontSet)
{
  // Get the file to load fonts from:
  const std::string strPath = g_SkinInfo->GetSkinPath("Font.xml", &m_skinResolution);
  CLog::Log(LOGINFO, "GUIFontManager::{}: Loading fonts from '{}'", __func__, strPath);

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "GUIFontManager::{}: Couldn't load '{}'", __func__, strPath);
    return;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || pRootElement->ValueStr() != "fonts")
  {
    CLog::Log(LOGERROR, "GUIFontManager::{}: File {} doesn't start with <fonts>", __func__,
              strPath);
    return;
  }

  // Resolve includes in Font.xml
  g_SkinInfo->ResolveIncludes(pRootElement);
  // take note of the first font available in case we can't load the one specified
  std::string firstFont;

  const TiXmlElement* pChild = pRootElement->FirstChildElement("fontset");
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
    CLog::Log(LOGWARNING,
              "GUIFontManager::{}: File doesn't have <fontset> with name '{}', defaulting to first "
              "fontset",
              __func__, fontSet);
    LoadFonts(firstFont);
  }
  else
    CLog::Log(LOGERROR, "GUIFontManager::{}: File '{}' doesn't have a valid <fontset>", __func__,
              strPath);
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
    UTILS::COLOR::Color shadowColor = 0;
    UTILS::COLOR::Color textColor = 0;
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
      LoadTTF(fontName, strFontFileName, textColor, shadowColor, iSize, iStyle, false, lineSpacing,
              aspect);
    }
    fontNode = fontNode->NextSibling("font");
  }
}

void GUIFontManager::GetStyle(const TiXmlNode* fontNode, int& iStyle)
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
  CFileItemList itemsRoot;
  CFileItemList items;

  // Find font files
  XFILE::CDirectory::GetDirectory("special://xbmc/media/Fonts/", itemsRoot, "",
                                  XFILE::DIR_FLAG_DEFAULTS);
  XFILE::CDirectory::GetDirectory("special://home/media/Fonts/", items, "",
                                  XFILE::DIR_FLAG_DEFAULTS);

  for (auto itItem = itemsRoot.rbegin(); itItem != itemsRoot.rend(); ++itItem)
    items.AddFront(*itItem, 0);

  for (const auto& item : items)
  {
    if (!item->m_bIsFolder && CUtil::IsSupportedFontExtension(item->GetLabel()))
    {
      list.emplace_back(item->GetLabel(), item->GetLabel());
    }
  }
}
