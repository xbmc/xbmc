#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "XBDateTime.h"
#include "addons/AddonVersion.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ADDON
{

  typedef enum
  {
    ADDON_UNKNOWN,
    ADDON_VIZ,
    ADDON_SKIN,
    ADDON_PVRDLL,
    ADDON_ADSPDLL,
    ADDON_INPUTSTREAM,
    ADDON_GAMEDLL,
    ADDON_PERIPHERALDLL,
    ADDON_SCRIPT,
    ADDON_SCRIPT_WEATHER,
    ADDON_SUBTITLE_MODULE,
    ADDON_SCRIPT_LYRICS,
    ADDON_SCRAPER_ALBUMS,
    ADDON_SCRAPER_ARTISTS,
    ADDON_SCRAPER_MOVIES,
    ADDON_SCRAPER_MUSICVIDEOS,
    ADDON_SCRAPER_TVSHOWS,
    ADDON_SCREENSAVER,
    ADDON_PLUGIN,
    ADDON_REPOSITORY,
    ADDON_WEB_INTERFACE,
    ADDON_SERVICE,
    ADDON_AUDIOENCODER,
    ADDON_CONTEXT_ITEM,
    ADDON_AUDIODECODER,
    ADDON_RESOURCE_IMAGES,
    ADDON_RESOURCE_LANGUAGE,
    ADDON_RESOURCE_UISOUNDS,
    ADDON_RESOURCE_GAMES,
    ADDON_VFS,
    ADDON_IMAGEDECODER,
    ADDON_SCRAPER_LIBRARY,
    ADDON_SCRIPT_LIBRARY,
    ADDON_SCRIPT_MODULE,
    ADDON_GAME_CONTROLLER,

    /**
     * @brief virtual addon types
     */
    //@{
    ADDON_VIDEO,
    ADDON_AUDIO,
    ADDON_IMAGE,
    ADDON_EXECUTABLE,
    ADDON_GAME,
    //@}

    ADDON_MAX
  } TYPE;

  class CAddonInfo;
  typedef std::shared_ptr<CAddonInfo> AddonInfoPtr;

  typedef std::map<std::string, std::pair<const AddonVersion, bool> > ADDONDEPS;
  typedef std::map<std::string, std::string> InfoMap;
  typedef std::map<std::string, std::string> ArtMap;

  class CAddonBuilder;

  class CAddonInfo
  {
  public:
    CAddonInfo();
    CAddonInfo(std::string id, TYPE type);

    bool IsUsable() const { return m_usable; }

    /*!
     * @brief Parts used from ADDON::CAddonDatabase
     */
    //@{
    void SetInstallDate(CDateTime installDate) { m_installDate = installDate; }
    void SetLastUpdated(CDateTime lastUpdated) { m_lastUpdated = lastUpdated; }
    void SetLastUsed(CDateTime lastUsed) { m_lastUsed = lastUsed; }
    void SetOrigin(std::string origin) { m_origin = std::move(origin); }
    //@}

    const std::string& ID() const { return m_id; }
    TYPE MainType() const { return m_mainType; }
    const AddonVersion& Version() const { return m_version; }
    const AddonVersion& MinVersion() const { return m_minversion; }
    const std::string& Name() const { return m_name; }
    const std::string& License() const { return m_license; }
    const std::string& Summary() const { return m_summary; }
    const std::string& Description() const { return m_description; }
    const std::string& LibName() const { return m_libname; }
    const std::string& Author() const { return m_author; }
    const std::string& Source() const { return m_source; }
    const std::string& Path() const { return m_path; }
    void SetPath(const std::string& path) { m_path = path; }
    const std::string& ChangeLog() const { return m_changelog; }
    const std::string& Icon() const { return m_icon; }
    const ArtMap& Art() const { return m_art; }
    const std::vector<std::string>& Screenshots() const { return m_screenshots; }
    const std::string& Disclaimer() const { return m_disclaimer; }
    const ADDONDEPS& GetDeps() const { return m_dependencies; }
    const std::string& Broken() const { return m_broken; }
    const CDateTime& InstallDate() const { return m_installDate; }
    const CDateTime& LastUpdated() const { return m_lastUpdated; }
    const CDateTime& LastUsed() const { return m_lastUsed; }
    const std::string& Origin() const { return m_origin; }
    uint64_t PackageSize() const { return m_packageSize; }
    const std::string& Language() const { return m_language; }
    bool MeetsVersion(const AddonVersion &version) const;

    /*!
     * @brief Utilities to translate add-on parts to his requested part.
     */
    //@{
    static std::string TranslateType(TYPE type, bool pretty=false);
    static std::string TranslateIconType(TYPE type);
    static TYPE TranslateType(const std::string &string);
    static TYPE TranslateSubContent(const std::string &content);
    //@}

    InfoMap extrainfo; // defined here, but removed in future with cpluff

  private:
    friend class ADDON::CAddonBuilder;

    bool m_usable;

    std::string m_id;
    TYPE m_mainType;

    AddonVersion m_version{"0.0.0"};
    AddonVersion m_minversion{"0.0.0"};
    std::string m_name;
    std::string m_license;
    std::string m_summary;
    std::string m_description;
    std::string m_author;
    std::string m_source;
    std::string m_path;
    std::string m_changelog;
    std::string m_icon;
    ArtMap m_art;
    std::vector<std::string> m_screenshots;
    std::string m_disclaimer;
    ADDONDEPS m_dependencies;
    std::string m_broken;
    CDateTime m_installDate;
    CDateTime m_lastUpdated;
    CDateTime m_lastUsed;
    std::string m_origin;
    uint64_t m_packageSize;
    std::string m_language;

    std::string m_libname;
  };

} /* namespace ADDON */
