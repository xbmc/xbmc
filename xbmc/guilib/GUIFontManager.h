/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIFontManager.h
\brief
*/

#include "IMsgTargetCallback.h"
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "utils/ColorUtils.h"
#include "utils/GlobalsHandling.h"
#include "windowing/GraphicContext.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

// Forward
class CGUIFont;
class CGUIFontTTF;
class CWinSystemBase;
class CXBMCTinyXML;
class TiXmlNode;
class CSetting;
struct StringSettingOption;

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

struct FontEntry
{
  std::unique_ptr<CGUIFont> font;
  OrigFontInfo origInfo;
};

struct FontScope
{
  //! true once discovery has run, even if no fonts were found. Never test
  //! `fonts.empty()` for this: an addon with no Font.xml has a legitimately
  //! empty loaded scope, and an emptiness test would re-parse on every open.
  bool loaded{false};
  std::vector<FontEntry> fonts;
  //! Per-scope, so a nested dialog's push does not wipe the outer window's
  //! dedup state and cause it to re-warn.
  std::set<std::string> warned;
};

struct FontMetadata
{
  FontMetadata(const std::string& filename, const std::set<std::string>& familyNames)
    : m_filename{filename},
      m_familyNames{familyNames}
  {
  }

  std::string m_filename;
  std::set<std::string> m_familyNames;
};

/*!
 \ingroup textures
 \brief
 */
class GUIFontManager : public IMsgTargetCallback
{
public:
  GUIFontManager();
  ~GUIFontManager() override;

  /*!
   *  \brief Initialize the font manager.
   *  Checks that fonts cache are up to date, otherwise update it.
   */
  void Initialize();

  bool IsUpdating() const { return m_critSection.IsLocked(); }

  bool OnMessage(CGUIMessage& message) override;

  void Unload(const std::string& strFontName);
  void LoadFonts(const std::string& fontSet);
  CGUIFont* LoadTTF(const std::string& strFontName,
                    const std::string& strFilename,
                    KODI::UTILS::COLOR::Color textColor,
                    KODI::UTILS::COLOR::Color shadowColor,
                    const int iSize,
                    const int iStyle,
                    bool border = false,
                    float lineSpacing = 1.0f,
                    float aspect = 1.0f,
                    const RESOLUTION_INFO* res = nullptr,
                    bool preserveAspect = false,
                    std::vector<FontEntry>* destination = nullptr,
                    const std::string& extraSearchDir = "");
  CGUIFont* GetFont(const std::string& strFontName, bool fallback = true);

  /*!
   \brief RAII bracket around a font scope. Pops even if the guarded call
   throws, which Python-invoked control creation can.
   */
  class CScopeGuard
  {
  public:
    CScopeGuard(GUIFontManager& manager, const std::string& scopeKey) : m_manager(manager)
    {
      m_manager.PushFontScope(scopeKey);
    }
    ~CScopeGuard() { m_manager.PopFontScope(); }

    CScopeGuard(const CScopeGuard&) = delete;
    CScopeGuard& operator=(const CScopeGuard&) = delete;

  private:
    GUIFontManager& m_manager;
  };

  void PushFontScope(const std::string& scopeKey);
  void PopFontScope();

  /*!
   \brief File name of the typeface the active skin uses, so an addon font that
          declares no <filename> can inherit it.
   \return the skin's "font13" file, else the first font it loaded, else empty
   */
  std::string GetSkinDefaultFontFileName() const;

  bool LoadFontsIntoScope(const std::string& scopeKey,
                          const std::string& fontXmlPath,
                          const std::string& extraSearchDir,
                          const RESOLUTION_INFO& sourceRes);
  void UnloadFontScope(const std::string& scopeKey);
  bool IsFontScopeLoaded(const std::string& scopeKey) const;

  /*! \brief return a default font
   \param border whether the font should be a font with an outline
   \return the font. `nullptr` if no default font can be found.
   */
  CGUIFont* GetDefaultFont(bool border = false);

  void Clear();
  void FreeFontFile(CGUIFontTTF* pFont);

  static void SettingOptionsFontsFiller(const std::shared_ptr<const CSetting>& setting,
                                        std::vector<StringSettingOption>& list,
                                        std::string& current);

  /*!
   * \brief Get the list of user fonts as family names from cache
   * \return The list of available fonts family names
   */
  std::vector<std::string> GetUserFontsFamilyNames();

  /*!
   \brief Build the key under which a rasterised CGUIFontTTF is pooled.
   \param fontFilePath the RESOLVED font file path, not the raw XML filename.
          Two addons may ship a different font under the same basename.
   */
  static std::string MakeFontIdent(const std::string& fontFilePath,
                                   float size,
                                   float aspect,
                                   bool border);

protected:
  void ReloadTTFFonts();
  bool ReloadFontEntry(CWinSystemBase& winSystem, FontEntry& entry);
  static void RescaleFontSizeAndAspect(CGraphicContext& context,
                                       float* size,
                                       float* aspect,
                                       const RESOLUTION_INFO& sourceRes,
                                       bool preserveAspect);
  void LoadFonts(const TiXmlNode* fontNode);
  CGUIFontTTF* GetFontFile(const std::string& fontIdent);

  //! \brief Depth of the calling thread's scope stack.
  size_t FontScopeDepth() const;
  //! \brief Key of the calling thread's innermost scope, empty if none.
  std::string InnermostFontScopeKey() const;
  //! \brief Mark a scope loaded without parsing its Font.xml.
  void MarkFontScopeLoaded(const std::string& scopeKey);

  std::vector<FontEntry> m_fonts;
  std::vector<std::unique_ptr<CGUIFontTTF>> m_vecFontFiles;
  RESOLUTION_INFO m_skinResolution;
  bool m_canReload{true};

private:
  void LoadUserFonts();
  bool LoadFontsFromFile(const std::string& fontsetFilePath,
                         const std::string& fontSet,
                         std::string& firstFontset);
  void WarnFontFallback(const std::string& strFontName);

  mutable CCriticalSection m_critSection;
  std::vector<FontMetadata> m_userFontsCache;

  //! Guarded by m_critSection.
  std::map<std::string, FontScope> m_scopedFonts;

  //! Per-thread resolution stack. Control::Create runs on the Python thread.
  //! Only the key is stored, never a FontScope*: the key is re-resolved against
  //! m_scopedFonts under m_critSection on every lookup, precisely so a
  //! concurrent Clear()/UnloadFontScope() on another thread cannot leave a
  //! dangling pointer into an erased map node.
  struct ScopeStackEntry
  {
    std::string key;
  };
  static thread_local std::vector<ScopeStackEntry> ms_scopeStack;
};

/*!
 \ingroup textures
 \brief
 */
XBMC_GLOBAL_REF(GUIFontManager, g_fontManager);
#define g_fontManager XBMC_GLOBAL_USE(GUIFontManager)
