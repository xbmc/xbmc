#pragma once
/*
*      Copyright (C) 2005-2013 Team XBMC
*      http://xbmc.org
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

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "XBDateTime.h"

class TiXmlElement;

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
    ADDON_VIDEO, // virtual addon types
    ADDON_AUDIO,
    ADDON_IMAGE,
    ADDON_EXECUTABLE,
    ADDON_SCRAPER_LIBRARY,
    ADDON_SCRIPT_LIBRARY,
    ADDON_SCRIPT_MODULE,
    ADDON_GAME_CONTROLLER,
    ADDON_MAX
  } TYPE;

  class IAddon;
  typedef std::shared_ptr<IAddon> AddonPtr;
  class CVisualisation;
  typedef std::shared_ptr<CVisualisation> VizPtr;
  class CSkinInfo;
  typedef std::shared_ptr<CSkinInfo> SkinPtr;
  class CPluginSource;
  typedef std::shared_ptr<CPluginSource> PluginPtr;

  class CAddonMgr;
  class AddonVersion;
  typedef std::map<std::string, std::pair<const AddonVersion, bool> > ADDONDEPS;
  typedef std::map<std::string, std::string> InfoMap;
  class AddonProps;

  class IAddon : public std::enable_shared_from_this<IAddon>
  {
  public:
    virtual ~IAddon() {};
    virtual TYPE Type() const =0;
    virtual TYPE FullType() const =0;
    virtual bool IsType(TYPE type) const =0;
    virtual std::string ID() const =0;
    virtual std::string Name() const =0;
    virtual bool IsInUse() const =0;
    virtual AddonVersion Version() const =0;
    virtual AddonVersion MinVersion() const =0;
    virtual std::string Summary() const =0;
    virtual std::string Description() const =0;
    virtual std::string Path() const =0;
    virtual std::string Profile() const =0;
    virtual std::string LibPath() const =0;
    virtual std::string ChangeLog() const =0;
    virtual std::string FanArt() const =0;
    virtual std::vector<std::string> Screenshots() const =0;
    virtual std::string Author() const =0;
    virtual std::string Icon() const =0;
    virtual std::string Disclaimer() const =0;
    virtual std::string Broken() const =0;
    virtual CDateTime InstallDate() const =0;
    virtual CDateTime LastUpdated() const =0;
    virtual CDateTime LastUsed() const =0;
    virtual std::string Origin() const =0;
    virtual uint64_t PackageSize() const =0;
    virtual const InfoMap &ExtraInfo() const =0;
    virtual bool HasSettings() =0;
    virtual void SaveSettings() =0;
    virtual void UpdateSetting(const std::string& key, const std::string& value) =0;
    virtual std::string GetSetting(const std::string& key) =0;
    virtual TiXmlElement* GetSettingsXML() =0;
    virtual const ADDONDEPS &GetDeps() const =0;
    virtual AddonVersion GetDependencyVersion(const std::string &dependencyID) const =0;
    virtual bool MeetsVersion(const AddonVersion &version) const =0;
    virtual bool ReloadSettings() =0;
    virtual void OnDisabled() =0;
    virtual void OnEnabled() =0;
    virtual AddonPtr GetRunningInstance() const=0;
    virtual void OnPreInstall() =0;
    virtual void OnPostInstall(bool update, bool modal) =0;
    virtual void OnPreUnInstall() =0;
    virtual void OnPostUnInstall() =0;
  };
};

