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

  typedef std::map<std::string, std::pair<const AddonVersion, bool> > ADDONDEPS;
  typedef std::map<std::string, std::string> InfoMap;
  typedef std::map<std::string, std::string> ArtMap;

  class CAddonInfo
  {
  public:
    CAddonInfo();
    CAddonInfo(std::string id, TYPE type);

    /*!
     * @brief Utilities to translate add-on parts to his requested part.
     */
    //@{
    static std::string TranslateType(TYPE type, bool pretty=false);
    static std::string TranslateIconType(TYPE type);
    static TYPE TranslateType(const std::string &string);
    static TYPE TranslateSubContent(const std::string &content);
    //@}

    std::string id;
    TYPE type;
    AddonVersion version{"0.0.0"};
    AddonVersion minversion{"0.0.0"};
    std::string name;
    std::string license;
    std::string summary;
    std::string description;
    std::string libname;
    std::string author;
    std::string source;
    std::string path;
    std::string changelog;
    std::string icon;
    std::map<std::string, std::string> art;
    std::vector<std::string> screenshots;
    std::string disclaimer;
    ADDONDEPS dependencies;
    std::string broken;
    InfoMap extrainfo;
    CDateTime installDate;
    CDateTime lastUpdated;
    CDateTime lastUsed;
    std::string origin;
    uint64_t packageSize;
  };

} /* namespace ADDON */
