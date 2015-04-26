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
#include <memory>

#include <map>
#include <set>
#include <string>
#include <stdint.h>

class TiXmlElement;

namespace ADDON
{
  typedef enum
  {
    ADDON_UNKNOWN,
    ADDON_VIZ,
    ADDON_SKIN,
    ADDON_PVRDLL,
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
    ADDON_RESOURCE_LANGUAGE,
    ADDON_RESOURCE_UISOUNDS,
    ADDON_VIDEO, // virtual addon types
    ADDON_AUDIO,
    ADDON_IMAGE,
    ADDON_EXECUTABLE,
    ADDON_VIZ_LIBRARY,
    ADDON_SCRAPER_LIBRARY,
    ADDON_SCRIPT_LIBRARY,
    ADDON_SCRIPT_MODULE,
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
    virtual AddonPtr Clone() const =0;
    virtual TYPE Type() const =0;
    virtual bool IsType(TYPE type) const =0;
    virtual AddonProps Props() const =0;
    virtual AddonProps& Props() =0;
    virtual const std::string ID() const =0;
    virtual const std::string Name() const =0;
    virtual bool Enabled() const =0;
    virtual bool IsInUse() const =0;
    virtual const AddonVersion Version() const =0;
    virtual const AddonVersion MinVersion() const =0;
    virtual const std::string Summary() const =0;
    virtual const std::string Description() const =0;
    virtual const std::string Path() const =0;
    virtual const std::string Profile() const =0;
    virtual const std::string LibPath() const =0;
    virtual const std::string ChangeLog() const =0;
    virtual const std::string FanArt() const =0;
    virtual const std::string Author() const =0;
    virtual const std::string Icon() const =0;
    virtual int  Stars() const =0;
    virtual const std::string Disclaimer() const =0;
    virtual const InfoMap &ExtraInfo() const =0;
    virtual bool HasSettings() =0;
    virtual void SaveSettings() =0;
    virtual void UpdateSetting(const std::string& key, const std::string& value) =0;
    virtual std::string GetSetting(const std::string& key) =0;
    virtual TiXmlElement* GetSettingsXML() =0;
    virtual std::string GetString(uint32_t id) =0;
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
    virtual bool CanInstall(const std::string& referer) =0;

  protected:
    virtual bool LoadSettings(bool bForce = false) =0;

  private:
    friend class CAddonMgr;
    virtual bool LoadStrings() =0;
    virtual void ClearStrings() =0;
  };
};

