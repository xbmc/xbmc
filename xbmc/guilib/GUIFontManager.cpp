/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFontManager.h"

#include "FileItemList.h"
#include "GUIComponent.h"
#include "GUIFontTTF.h"
#include "GUIWindowManager.h"
#include "addons/AddonManager.h"
#include "addons/FontResource.h"
#include "addons/Skin.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/SpecialProtocol.h"
#include "windowing/GraphicContext.h"

#include <mutex>
#if defined(HAS_GL)
#include "GUIFontTTFGL.h"
#endif
#if defined(HAS_GLES)
#include "GUIFontTTFGLES.h"
#endif
#include "FileItem.h"
#include "GUIControlFactory.h"
#include "GUIFont.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "utils/FileUtils.h"
#include "utils/FontUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <set>

using namespace XFILE;
using namespace ADDON;

namespace
{
constexpr const char* XML_FONTCACHE_FILENAME = "fontcache.xml";

bool LoadXMLData(const std::string& filepath, CXBMCTinyXML& xmlDoc)
{
  if (!CFileUtils::Exists(filepath))
  {
    CLog::LogF(LOGDEBUG, "Couldn't load '{}' the file don't exists", filepath);
    return false;
  }
  if (!xmlDoc.LoadFile(filepath))
  {
    CLog::LogF(LOGERROR, "Couldn't load '{}'", filepath);
    return false;
  }
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || pRootElement->ValueStr() != "fonts")
  {
    CLog::LogF(LOGERROR, "Couldn't load '{}' XML content doesn't start with <fonts>", filepath);
    return false;
  }
  return true;
}
} // unnamed namespace


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
  if (!CFileUtils::Exists(strPath))
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
    CServiceBroker::GetAddonMgr().GetAddons(addons, AddonType::RESOURCE_FONT);
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

#if defined(HAS_GL)
  CGUIFontTTFGL::DestroyStaticVertexBuffers();
#endif

#if defined(HAS_GLES)
  CGUIFontTTFGLES::DestroyStaticVertexBuffers();
#endif
}

bool GUIFontManager::LoadFontsFromFile(const std::string& fontsetFilePath,
                                       const std::string& fontSet,
                                       std::string& firstFontset)
{
  CXBMCTinyXML xmlDoc;
  if (LoadXMLData(fontsetFilePath, xmlDoc))
  {
    TiXmlElement* rootElement = xmlDoc.RootElement();
    g_SkinInfo->ResolveIncludes(rootElement);
    const TiXmlElement* fontsetElement = rootElement->FirstChildElement("fontset");
    while (fontsetElement)
    {
      const char* idAttr = fontsetElement->Attribute("id");
      if (idAttr)
      {
        // Take note of the first fontset available in case we can't load the fontset requested
        if (firstFontset.empty())
          firstFontset = idAttr;

        if (StringUtils::EqualsNoCase(fontSet, idAttr))
        {
          // Found the requested fontset, so load the fonts and return
          CLog::LogF(LOGINFO, "Loading <fontset> with name '{}' from '{}'", fontSet,
                     fontsetFilePath);
          LoadFonts(fontsetElement->FirstChild("font"));
          return true;
        }
      }
      fontsetElement = fontsetElement->NextSiblingElement("fontset");
    }
  }
  return false;
}

void GUIFontManager::LoadFonts(const std::string& fontSet)
{
  std::string firstFontset;
  // Try to load the fontset from Font.xml
  const std::string fontsetFilePath = g_SkinInfo->GetSkinPath("Font.xml", &m_skinResolution);
  if (LoadFontsFromFile(fontsetFilePath, fontSet, firstFontset))
    return;

  // If we got here, then the requested fontset was not found in the skin's Font.xml file
  // Look at additional fontsets that are defined in .xml files in the skin's fonts directory
  CFileItemList xmlFileItems;
  CDirectory::GetDirectory(CSpecialProtocol::TranslatePath("special://skin/fonts"), xmlFileItems,
                           ".xml", DIR_FLAG_BYPASS_CACHE);
  for (int i = 0; i < xmlFileItems.Size(); i++)
    if (LoadFontsFromFile(xmlFileItems[i]->GetPath(), fontSet, firstFontset))
      return;

  // Requested fontset was not found, try the first
  if (!firstFontset.empty())
  {
    CLog::LogF(LOGWARNING,
               "Fontset with name '{}' was not found, "
               "defaulting to first fontset '{}' ",
               fontSet, firstFontset);
    LoadFonts(firstFontset);
  }
  else
    CLog::LogF(LOGERROR, "No valid <fontset> found in '{}' or in xml files in fonts directory",
               fontsetFilePath);
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
      LoadTTF(fontName, fileName, textColor, shadowColor, iSize, iStyle, false, lineSpacing,
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
  XFILE::CDirectory::GetDirectory(UTILS::FONT::FONTPATH::SYSTEM, itemsRoot,
                                  UTILS::FONT::SUPPORTED_EXTENSIONS_MASK,
                                  XFILE::DIR_FLAG_NO_FILE_DIRS | XFILE::DIR_FLAG_NO_FILE_INFO);
  XFILE::CDirectory::GetDirectory(UTILS::FONT::FONTPATH::USER, items,
                                  UTILS::FONT::SUPPORTED_EXTENSIONS_MASK,
                                  XFILE::DIR_FLAG_NO_FILE_DIRS | XFILE::DIR_FLAG_NO_FILE_INFO);

  for (auto itItem = itemsRoot.rbegin(); itItem != itemsRoot.rend(); ++itItem)
    items.AddFront(*itItem, 0);

  for (const auto& item : items)
  {
    if (item->m_bIsFolder)
      continue;

    list.emplace_back(item->GetLabel(), item->GetLabel());
  }
}

void GUIFontManager::Initialize()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  LoadUserFonts();
}

void GUIFontManager::LoadUserFonts()
{
  if (!XFILE::CDirectory::Exists(UTILS::FONT::FONTPATH::USER))
    return;

  CLog::LogF(LOGDEBUG, "Updating user fonts cache...");
  CXBMCTinyXML xmlDoc;
  std::string userFontCacheFilepath =
      URIUtils::AddFileToFolder(UTILS::FONT::FONTPATH::USER, XML_FONTCACHE_FILENAME);
  if (LoadXMLData(userFontCacheFilepath, xmlDoc))
  {
    // Load in cache the fonts metadata previously stored in the XML
    TiXmlElement* pRootElement = xmlDoc.RootElement();
    if (pRootElement)
    {
      const TiXmlNode* fontNode = pRootElement->FirstChild("font");
      while (fontNode)
      {
        std::string filename;
        XMLUtils::GetString(fontNode, "filename", filename);

        std::set<std::string> familyNames;
        for (const TiXmlElement* fnChildNode = fontNode->FirstChildElement("familyname");
             fnChildNode; fnChildNode = fnChildNode->NextSiblingElement("familyname"))
        {
          familyNames.emplace(fnChildNode->GetText());
        }

        m_userFontsCache.emplace_back(filename, familyNames);
        fontNode = fontNode->NextSibling("font");
      }
    }
  }

  bool isCacheChanged{false};
  size_t previousCacheSize = m_userFontsCache.size();
  CFileItemList dirItems;
  // Get the current files list from user fonts folder
  XFILE::CDirectory::GetDirectory(UTILS::FONT::FONTPATH::USER, dirItems,
                                  UTILS::FONT::SUPPORTED_EXTENSIONS_MASK,
                                  XFILE::DIR_FLAG_NO_FILE_DIRS | XFILE::DIR_FLAG_NO_FILE_INFO);
  dirItems.SetFastLookup(true);

  // Remove files that no longer exist from cache
  auto it = m_userFontsCache.begin();
  while (it != m_userFontsCache.end())
  {
    const std::string filePath = UTILS::FONT::FONTPATH::USER + (*it).m_filename;
    if (!dirItems.Contains(filePath))
    {
      it = m_userFontsCache.erase(it);
    }
    else
    {
      auto item = dirItems.Get(filePath);
      dirItems.Remove(item.get());
      ++it;
    }
  }
  isCacheChanged = previousCacheSize != m_userFontsCache.size();
  previousCacheSize = m_userFontsCache.size();

  // Add new files in cache and generate the metadata
  //!@todo FIXME: this "for" loop should be replaced with the more performant
  //! parallel execution std::for_each(std::execution::par, ...
  //! to halving loading times of fonts list, maybe with C++17 with appropriate
  //! fix to include parallel execution or future C++20
  for (auto& item : dirItems)
  {
    std::string filepath = item->GetPath();
    if (item->m_bIsFolder)
      continue;

    std::set<std::string> familyNames;
    if (UTILS::FONT::GetFontFamilyNames(filepath, familyNames))
    {
      m_userFontsCache.emplace_back(item->GetLabel(), familyNames);
    }
  }
  isCacheChanged = isCacheChanged || previousCacheSize != m_userFontsCache.size();

  // If the cache is changed save an updated XML cache file
  if (isCacheChanged)
  {
    CXBMCTinyXML xmlDoc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    xmlDoc.InsertEndChild(decl);
    TiXmlElement xmlMainElement("fonts");

    TiXmlNode* fontsNode = xmlDoc.InsertEndChild(xmlMainElement);
    if (fontsNode)
    {
      for (const FontMetadata& fontMetadata : m_userFontsCache)
      {
        TiXmlElement fontElement("font");
        TiXmlNode* fontNode = fontsNode->InsertEndChild(fontElement);
        XMLUtils::SetString(fontNode, "filename", fontMetadata.m_filename);
        for (const std::string& familyName : fontMetadata.m_familyNames)
        {
          XMLUtils::SetString(fontNode, "familyname", familyName);
        }
      }
      if (!xmlDoc.SaveFile(userFontCacheFilepath))
        CLog::LogF(LOGERROR, "Failed to save fonts cache file \"{}\"", userFontCacheFilepath);
    }
    else
    {
      CLog::LogF(LOGERROR, "Failed to create XML \"fonts\" node");
    }
  }
  CLog::LogF(LOGDEBUG, "Updating user fonts cache... DONE");
}

std::vector<std::string> GUIFontManager::GetUserFontsFamilyNames()
{
  // We ensure to have unique font family names and sorted alphabetically
  // Duplicated family names can happens for example when a font have each style
  // on different files
  std::set<std::string, sortstringbyname> familyNames;
  for (const FontMetadata& fontMetadata : m_userFontsCache)
  {
    for (const std::string& familyName : fontMetadata.m_familyNames)
    {
      familyNames.insert(familyName);
    }
  }
  return std::vector<std::string>(familyNames.begin(), familyNames.end());
}
