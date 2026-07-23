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
#include "GUIFontParsing.h"
#include "GUIFontTTF.h"
#include "GUIWindowManager.h"
#include "addons/AddonManager.h"
#include "addons/FontResource.h"
#include "addons/Skin.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/SpecialProtocol.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <mutex>
#if defined(HAS_GL)
#include "GUIFontTTFGL.h"
#endif
#if defined(HAS_GLES)
#include "GUIFontTTFGLES.h"
#endif
#include "FileItem.h"
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

thread_local std::vector<GUIFontManager::ScopeStackEntry> GUIFontManager::ms_scopeStack;

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
                                  KODI::UTILS::COLOR::Color textColor,
                                  KODI::UTILS::COLOR::Color shadowColor,
                                  const int iSize,
                                  const int iStyle,
                                  bool border,
                                  float lineSpacing,
                                  float aspect,
                                  const RESOLUTION_INFO* sourceRes,
                                  bool preserveAspect,
                                  std::vector<FontEntry>* destination,
                                  const std::string& extraSearchDir)
{
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
  {
    CLog::LogF(LOGFATAL, "Something tries to call function without an available GUI window system");
    return nullptr;
  }

  CGraphicContext& context = winSystem->GetGfxContext();

  float originalAspect = aspect;

  std::vector<FontEntry>& fonts = destination ? *destination : m_fonts;

  //check if font already exists in the destination
  for (const auto& entry : fonts)
  {
    if (StringUtils::EqualsNoCase(entry.font->GetFontName(), strFontName))
      return entry.font.get();
  }

  if (!sourceRes) // no source res specified, so assume the skin res
    sourceRes = &m_skinResolution;

  float newSize = static_cast<float>(iSize);
  RescaleFontSizeAndAspect(context, &newSize, &aspect, *sourceRes, preserveAspect);

  // An addon-scoped load looks in the addon's own fonts directory first. Were
  // the skin searched first, an addon font whose file name happens to match one
  // the active skin ships would silently resolve to the skin's file, which is
  // exactly the skin dependence a scope exists to remove. A skin load, which
  // passes no extra search directory, keeps looking in the skin first.
  const std::string skinFontsDir = URIUtils::AddFileToFolder(context.GetMediaDir(), "fonts");

  std::string strPath;
  if (CURL::IsFullPath(strFilename))
    strPath = strFilename;
  else if (!extraSearchDir.empty())
    strPath = URIUtils::AddFileToFolder(extraSearchDir, strFilename);
  else
    strPath = URIUtils::AddFileToFolder(skinFontsDir, strFilename);

#ifdef TARGET_POSIX
  strPath = CSpecialProtocol::TranslatePathConvertCase(strPath);
#endif

  // Check if the file exists, otherwise fall back to the skin, then to the
  // global media dirs, then to any resource.font addon.
  std::string file = URIUtils::GetFileName(strFilename);
  if (!CheckFont(strPath, skinFontsDir, file) &&
      !CheckFont(strPath, "special://home/media/Fonts", file) &&
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
  const std::string fontIdent = MakeFontIdent(strPath, newSize, aspect, border);

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
        CLog::LogF(LOGERROR, "Couldn't load font name: {}({}), trying arial.ttf",
                   EscapeFontName(strFontName), EscapeFontName(strFilename));
        return LoadTTF(strFontName, "arial.ttf", textColor, shadowColor, iSize, iStyle, border,
                       lineSpacing, originalAspect, sourceRes, preserveAspect, destination,
                       extraSearchDir);
      }
      CLog::LogF(LOGERROR, "Couldn't load font name:{} file:{}", EscapeFontName(strFontName),
                 EscapeFontName(strPath));

      return nullptr;
    }

    m_vecFontFiles.emplace_back(pFontFile);
  }

  // font file is loaded, create our CGUIFont
  CGUIFont* pNewFont = new CGUIFont(strFontName, iStyle, textColor, shadowColor, lineSpacing,
                                    static_cast<float>(iSize), pFontFile);

  // Store the original TTF font info in case we need to reload it in a different resolution
  OrigFontInfo fontInfo;
  fontInfo.size = iSize;
  fontInfo.aspect = originalAspect;
  fontInfo.fontFilePath = strPath;
  fontInfo.fileName = strFilename;
  fontInfo.sourceRes = *sourceRes;
  fontInfo.preserveAspect = preserveAspect;
  fontInfo.border = border;

  fonts.emplace_back(FontEntry{std::unique_ptr<CGUIFont>(pNewFont), fontInfo});

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

std::string GUIFontManager::MakeFontIdent(const std::string& fontFilePath,
                                          float size,
                                          float aspect,
                                          bool border)
{
  return StringUtils::Format("{}_{:f}_{:f}{}", fontFilePath, size, aspect, border ? "_border" : "");
}

bool GUIFontManager::ReloadFontEntry(CWinSystemBase& winSystem, FontEntry& entry)
{
  OrigFontInfo fontInfo = entry.origInfo;

  float aspect = fontInfo.aspect;
  float newSize = static_cast<float>(fontInfo.size);
  std::string& strPath = fontInfo.fontFilePath;

  RescaleFontSizeAndAspect(winSystem.GetGfxContext(), &newSize, &aspect, fontInfo.sourceRes,
                           fontInfo.preserveAspect);

  const std::string fontIdent = MakeFontIdent(strPath, newSize, aspect, fontInfo.border);
  CGUIFontTTF* pFontFile = GetFontFile(fontIdent);
  if (!pFontFile)
  {
    pFontFile = CGUIFontTTF::CreateGUIFontTTF(fontIdent);
    if (!pFontFile || !pFontFile->Load(strPath, newSize, aspect, 1.0f, fontInfo.border))
    {
      delete pFontFile;
      CLog::LogF(LOGERROR, "Couldn't re-load font file: '{}'", EscapeFontName(strPath));
      return false;
    }

    m_vecFontFiles.emplace_back(pFontFile);
  }

  entry.font->SetFont(pFontFile);
  return true;
}

void GUIFontManager::ReloadTTFFonts(void)
{
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return;

  for (auto& entry : m_fonts)
  {
    if (!ReloadFontEntry(*winSystem, entry))
      return;
  }

  std::unique_lock lock(m_critSection);
  for (auto& [key, scope] : m_scopedFonts)
  {
    for (auto& entry : scope.fonts)
    {
      if (!ReloadFontEntry(*winSystem, entry))
        return;
    }
  }
}

void GUIFontManager::Unload(const std::string& strFontName)
{
  EraseFirstByName(m_fonts, strFontName,
                   [](const FontEntry& entry) { return entry.font->GetFontName(); });
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
  if (strFontName.empty())
    return nullptr;

  // Innermost scope first. Empty for skin windows, which is the common case:
  // one branch on an empty vector.
  for (auto it = ms_scopeStack.rbegin(); it != ms_scopeStack.rend(); ++it)
  {
    if (it->key.empty()) // null-scope marker for a window with no addon font scope
      continue;

    // Re-resolve the key under the lock rather than caching a FontScope*, so a
    // concurrent Clear()/UnloadFontScope() cannot leave us dereferencing an
    // erased map node.
    std::unique_lock lock(m_critSection);
    const auto scopeIt = m_scopedFonts.find(it->key);
    if (scopeIt == m_scopedFonts.end())
      continue;

    for (const auto& entry : scopeIt->second.fonts)
    {
      if (StringUtils::EqualsNoCase(entry.font->GetFontName(), strFontName))
        return entry.font.get();
    }
  }

  for (const auto& entry : m_fonts)
  {
    CGUIFont* pFont = entry.font.get();
    if (StringUtils::EqualsNoCase(pFont->GetFontName(), strFontName))
      return pFont;
  }

  if (!fallback)
    return nullptr;

  WarnFontFallback(strFontName);

  if (StringUtils::EqualsNoCase(strFontName, "font13"))
    return nullptr;

  // Resolve the default from the global set directly rather than recursing,
  // which would re-walk the whole scope stack.
  for (const auto& entry : m_fonts)
  {
    if (StringUtils::EqualsNoCase(entry.font->GetFontName(), "font13"))
      return entry.font.get();
  }

  return nullptr;
}

void GUIFontManager::WarnFontFallback(const std::string& strFontName)
{
  // Only addon windows have a scope, so skin windows never warn. Dedup is per
  // scope, so a 30-label dialog missing two fonts logs two lines, not thirty.
  if (ms_scopeStack.empty() || ms_scopeStack.back().key.empty())
    return;

  // Re-resolve under the lock rather than caching a FontScope*, so a concurrent
  // Clear()/UnloadFontScope() cannot leave us dereferencing an erased map node.
  std::unique_lock lock(m_critSection);
  const auto scopeIt = m_scopedFonts.find(ms_scopeStack.back().key);
  if (scopeIt == m_scopedFonts.end())
    return;

  FontScope& scope = scopeIt->second;
  if (!scope.warned.insert(strFontName).second)
    return;

  CLog::LogF(LOGWARNING,
             "Font '{}' is defined by neither the addon window '{}' nor the active skin, "
             "falling back to 'font13'",
             EscapeFontName(strFontName), ms_scopeStack.back().key);
}

void GUIFontManager::PushFontScope(const std::string& scopeKey)
{
  // An empty key means the window has no addon font scope (a plain
  // xbmcgui.Window, or any non-WindowXML window). Push a null-scope marker so
  // push/pop stays balanced; GetFont and WarnFontFallback skip null entries.
  if (scopeKey.empty())
  {
    ms_scopeStack.emplace_back(ScopeStackEntry{""});
    return;
  }

  {
    // Ensure the scope node (and its `warned` set) exists and survives; the key
    // is re-resolved under the lock on every lookup, so we never cache the
    // pointer.
    std::unique_lock lock(m_critSection);
    m_scopedFonts[scopeKey]; // default-constructs an unloaded scope
  }

  ms_scopeStack.emplace_back(ScopeStackEntry{scopeKey});
}

void GUIFontManager::PopFontScope()
{
  if (ms_scopeStack.empty())
  {
    CLog::LogF(LOGERROR, "Font scope stack underflow, unbalanced push/pop");
    return;
  }

  ms_scopeStack.pop_back();
}

size_t GUIFontManager::FontScopeDepth() const
{
  return ms_scopeStack.size();
}

std::string GUIFontManager::InnermostFontScopeKey() const
{
  if (ms_scopeStack.empty())
    return {};

  return ms_scopeStack.back().key;
}

void GUIFontManager::MarkFontScopeLoaded(const std::string& scopeKey)
{
  std::unique_lock lock(m_critSection);
  m_scopedFonts[scopeKey].loaded = true;
}

bool GUIFontManager::IsFontScopeLoaded(const std::string& scopeKey) const
{
  std::unique_lock lock(m_critSection);
  const auto it = m_scopedFonts.find(scopeKey);
  return it != m_scopedFonts.end() && it->second.loaded;
}

void GUIFontManager::UnloadFontScope(const std::string& scopeKey)
{
  std::unique_lock lock(m_critSection);
  m_scopedFonts.erase(scopeKey); // absent key: silent no-op, by contract
}

std::string GUIFontManager::GetSkinDefaultFontFileName() const
{
  // Mirrors GetDefaultFont's choice: "font13" if the skin defines it, otherwise
  // the first font it loaded. In practice that file is the skin's body face,
  // and it follows the fontset the user picked in settings.
  const FontEntry* first = nullptr;
  for (const auto& entry : m_fonts)
  {
    if (!first)
      first = &entry;

    if (StringUtils::EqualsNoCase(entry.font->GetFontName(), "font13"))
      return entry.origInfo.fileName;
  }

  return first ? first->origInfo.fileName : std::string{};
}

bool GUIFontManager::LoadFontsIntoScope(const std::string& scopeKey,
                                        const std::string& fontXmlPath,
                                        const std::string& extraSearchDir,
                                        const RESOLUTION_INFO& sourceRes)
{
  {
    std::unique_lock lock(m_critSection);
    if (m_scopedFonts[scopeKey].loaded)
      return true;
  }

  std::vector<FontDefinition> definitions;

  // LoadXMLData is the existing file-scope helper at GUIFontManager.cpp:55. It
  // checks existence, logs LOGDEBUG when absent and LOGERROR when malformed, so
  // the caller need not probe with CFile::Exists first.
  CXBMCTinyXML xmlDoc;
  if (!fontXmlPath.empty() && LoadXMLData(fontXmlPath, xmlDoc))
  {
    // Never hand an addon-authored document to the skin's ResolveIncludes.
    StripIncludes(xmlDoc.RootElement());

    const TiXmlElement* fontsetElement = xmlDoc.RootElement()->FirstChildElement("fontset");
    const TiXmlElement* chosen = nullptr;
    for (; fontsetElement; fontsetElement = fontsetElement->NextSiblingElement("fontset"))
    {
      const char* idAttr = fontsetElement->Attribute("id");
      if (!idAttr)
        continue;

      if (!chosen)
        chosen = fontsetElement; // first fontset, the fallback
      if (StringUtils::EqualsNoCase(idAttr, "Default"))
      {
        chosen = fontsetElement;
        break;
      }
    }

    if (chosen)
      definitions = ParseFontSet(chosen->FirstChild("font"));
    else
      CLog::LogF(LOGERROR, "No valid <fontset> in addon font file '{}'", fontXmlPath);
  }
  // LoadXMLData already logged the absent/malformed case. An addon with no
  // Font.xml lands here and gets a loaded-but-empty scope, which is correct.

  const std::string skinTypeface = GetSkinDefaultFontFileName();

  for (const FontDefinition& def : definitions)
  {
    // No <filename> means "use whatever typeface the skin uses". The addon
    // still owns the size and the style, and the file it inherits is one the
    // skin already loaded, so it needs no confinement check.
    std::string fileName = def.fileName;
    std::string searchDir = extraSearchDir;
    if (fileName.empty())
    {
      if (skinTypeface.empty())
      {
        CLog::LogF(LOGWARNING, "Font '{}' inherits the skin typeface, but the skin loaded none",
                   EscapeFontName(def.name));
        continue;
      }

      fileName = skinTypeface;
      searchDir.clear(); // resolve it where the skin did, not in the addon
      CLog::LogF(LOGDEBUG, "Font '{}' inherits the skin typeface '{}'", EscapeFontName(def.name),
                 EscapeFontName(fileName));
    }
    else if (!IsAddonSafeFontFilename(fileName))
    {
      CLog::LogF(LOGWARNING, "Rejecting font '{}': filename '{}' is not confined to the addon",
                 EscapeFontName(def.name), EscapeFontName(fileName));
      continue;
    }

    std::vector<FontEntry>* destination = nullptr;
    {
      std::unique_lock lock(m_critSection);
      destination = &m_scopedFonts[scopeKey].fonts;
    }

    LoadTTF(def.name, fileName, def.textColor, def.shadowColor, def.size, def.style, false,
            def.lineSpacing, def.aspect, &sourceRes, false, destination, searchDir);
  }

  std::unique_lock lock(m_critSection);
  m_scopedFonts[scopeKey].loaded = true; // loaded, even if empty
  return true;
}

CGUIFont* GUIFontManager::GetDefaultFont(bool border)
{
  // first find "font13" or "__defaultborder__"
  size_t font13index = m_fonts.size();
  CGUIFont* font13border = nullptr;
  for (size_t i = 0; i < m_fonts.size(); i++)
  {
    CGUIFont* font = m_fonts[i].font.get();
    if (font->GetFontName() == "font13")
      font13index = i;
    else if (font->GetFontName() == "__defaultborder__")
      font13border = font;
  }
  // no "font13" means no default font is found - use the first font found.
  if (font13index == m_fonts.size())
  {
    if (m_fonts.empty())
      return nullptr;

    font13index = 0;
  }

  if (border)
  {
    if (!font13border)
    { // create it
      const FontEntry& entry = m_fonts[font13index];
      OrigFontInfo fontInfo = entry.origInfo;
      font13border = LoadTTF("__defaultborder__", fontInfo.fileName, KODI::UTILS::COLOR::BLACK, 0,
                             fontInfo.size, entry.font->GetStyle(), true, 1.0f, fontInfo.aspect,
                             &fontInfo.sourceRes, fontInfo.preserveAspect);
    }
    return font13border;
  }

  return m_fonts[font13index].font.get();
}

void GUIFontManager::Clear()
{
  m_fonts.clear();
  m_vecFontFiles.clear();

  {
    std::unique_lock lock(m_critSection);
    m_scopedFonts.clear();
  }
  ms_scopeStack.clear();

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
    auto skin = CServiceBroker::GetGUI()->GetSkinInfo();
    if (skin)
      skin->ResolveIncludes(rootElement);
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
          CLog::LogF(LOGINFO, "Loading <fontset> with name '{}' from '{}'", EscapeFontName(fontSet),
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
  auto skin = CServiceBroker::GetGUI()->GetSkinInfo();
  if (!skin)
    return;

  std::string firstFontset;
  // Try to load the fontset from Font.xml
  const std::string fontsetFilePath = skin->GetSkinPath("Font.xml", &m_skinResolution);
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
               EscapeFontName(fontSet), EscapeFontName(firstFontset));
    LoadFonts(firstFontset);
  }
  else
    CLog::LogF(LOGERROR, "No valid <fontset> found in '{}' or in xml files in fonts directory",
               fontsetFilePath);
}

void GUIFontManager::LoadFonts(const TiXmlNode* fontNode)
{
  for (const FontDefinition& def : ParseFontSet(fontNode))
  {
    // Only an addon font scope may omit <filename>, to inherit the skin's
    // typeface. A skin has nothing to inherit from.
    if (def.fileName.empty())
    {
      CLog::LogF(LOGWARNING, "Skin font '{}' has no <filename>", EscapeFontName(def.name));
      continue;
    }

    LoadTTF(def.name, def.fileName, def.textColor, def.shadowColor, def.size, def.style, false,
            def.lineSpacing, def.aspect);
  }
}

void GUIFontManager::SettingOptionsFontsFiller(const SettingConstPtr& setting,
                                               std::vector<StringSettingOption>& list,
                                               std::string& current)
{
  CFileItemList itemsRoot;
  CFileItemList items;

  // Find font files
  XFILE::CDirectory::GetDirectory(KODI::UTILS::FONT::FONTPATH::SYSTEM, itemsRoot,
                                  KODI::UTILS::FONT::SUPPORTED_EXTENSIONS_MASK,
                                  XFILE::DIR_FLAG_NO_FILE_DIRS | XFILE::DIR_FLAG_NO_FILE_INFO);
  XFILE::CDirectory::GetDirectory(KODI::UTILS::FONT::FONTPATH::USER, items,
                                  KODI::UTILS::FONT::SUPPORTED_EXTENSIONS_MASK,
                                  XFILE::DIR_FLAG_NO_FILE_DIRS | XFILE::DIR_FLAG_NO_FILE_INFO);

  for (auto itItem = itemsRoot.rbegin(); itItem != itemsRoot.rend(); ++itItem)
    items.AddFront(*itItem, 0);

  for (const auto& item : items)
  {
    if (item->IsFolder())
      continue;

    list.emplace_back(item->GetLabel(), item->GetLabel());
  }
}

void GUIFontManager::Initialize()
{
  std::unique_lock lock(m_critSection);
  LoadUserFonts();
}

void GUIFontManager::LoadUserFonts()
{
  if (!XFILE::CDirectory::Exists(KODI::UTILS::FONT::FONTPATH::USER))
    return;

  CLog::LogF(LOGDEBUG, "Updating user fonts cache...");
  CXBMCTinyXML xmlDoc;
  std::string userFontCacheFilepath =
      URIUtils::AddFileToFolder(KODI::UTILS::FONT::FONTPATH::USER, XML_FONTCACHE_FILENAME);
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
  XFILE::CDirectory::GetDirectory(KODI::UTILS::FONT::FONTPATH::USER, dirItems,
                                  KODI::UTILS::FONT::SUPPORTED_EXTENSIONS_MASK,
                                  XFILE::DIR_FLAG_NO_FILE_DIRS | XFILE::DIR_FLAG_NO_FILE_INFO);
  dirItems.SetFastLookup(true);

  // Remove files that no longer exist from cache
  auto it = m_userFontsCache.begin();
  while (it != m_userFontsCache.end())
  {
    const std::string filePath = KODI::UTILS::FONT::FONTPATH::USER + (*it).m_filename;
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
    if (item->IsFolder())
      continue;

    std::set<std::string> familyNames;
    if (KODI::UTILS::FONT::GetFontFamilyNames(filepath, familyNames))
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
