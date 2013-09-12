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
#include "boost/shared_ptr.hpp"
#include "utils/StdString.h"

#include <boost/enable_shared_from_this.hpp>

#include <set>
#include <map>

class TiXmlElement;

namespace ADDON
{
  typedef enum
  {
    ADDON_UNKNOWN,
    ADDON_VIZ,
    ADDON_SKIN,
    ADDON_PVRDLL,
    ADDON_GAMEDLL,
    ADDON_SCRIPT,
    ADDON_SCRIPT_WEATHER,
    ADDON_SCRIPT_SUBTITLES,
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
    ADDON_VIDEO, // virtual addon types
    ADDON_AUDIO,
    ADDON_IMAGE,
    ADDON_GAME,
    ADDON_EXECUTABLE,
    ADDON_VIZ_LIBRARY, // add noninstallable after this and installable before
    ADDON_SCRAPER_LIBRARY,
    ADDON_SCRIPT_LIBRARY,
    ADDON_SCRIPT_MODULE
  } TYPE;

  class IAddon;
  typedef boost::shared_ptr<IAddon> AddonPtr;
  class CVisualisation;
  typedef boost::shared_ptr<CVisualisation> VizPtr;
  class CSkinInfo;
  typedef boost::shared_ptr<CSkinInfo> SkinPtr;
  class CPluginSource;
  typedef boost::shared_ptr<CPluginSource> PluginPtr;

  class CAddonMgr;
  class AddonVersion;
  typedef std::map<CStdString, std::pair<const AddonVersion, bool> > ADDONDEPS;
  typedef std::map<CStdString, CStdString> InfoMap;
  class AddonProps;

  class IAddon : public boost::enable_shared_from_this<IAddon>
  {
  public:
    virtual ~IAddon() {};
    virtual AddonPtr Clone() const =0;
    virtual TYPE Type() const =0;
    virtual bool IsType(TYPE type) const =0;
    virtual AddonProps Props() const =0;
    virtual AddonProps& Props() =0;
    virtual const CStdString ID() const =0;
    virtual const CStdString Name() const =0;
    virtual bool Enabled() const =0;
    virtual bool IsInUse() const =0;
    virtual const AddonVersion Version() const =0;
    virtual const AddonVersion MinVersion() const =0;
    virtual const CStdString Summary() const =0;
    virtual const CStdString Description() const =0;
    virtual const CStdString Path() const =0;
    virtual const CStdString Profile() const =0;
    virtual const CStdString LibPath() const =0;
    virtual const CStdString ChangeLog() const =0;
    virtual const CStdString FanArt() const =0;
    virtual const CStdString Author() const =0;
    virtual const CStdString Icon() const =0;
    virtual int  Stars() const =0;
    virtual const CStdString Disclaimer() const =0;
    virtual const InfoMap &ExtraInfo() const =0;
    virtual bool HasSettings() =0;
    virtual void SaveSettings() =0;
    virtual void UpdateSetting(const CStdString& key, const CStdString& value) =0;
    virtual CStdString GetSetting(const CStdString& key) =0;
    virtual TiXmlElement* GetSettingsXML() =0;
    virtual CStdString GetString(uint32_t id) =0;
    virtual const ADDONDEPS &GetDeps() const =0;
    virtual bool MeetsVersion(const AddonVersion &version) const =0;
    virtual bool ReloadSettings() =0;

  protected:
    virtual bool LoadSettings(bool bForce = false) =0;

  private:
    friend class CAddonMgr;
    virtual bool IsAddonLibrary() =0;
    virtual void Enable() =0;
    virtual void Disable() =0;
    virtual bool LoadStrings() =0;
    virtual void ClearStrings() =0;
  };

  // some utilitiy methods

  /**
   * This function will extract the Addon's currently assigned xbmc.python
   * API version. If addon is NULL, or there is no xbmc.python dependency defined,
   * then the version is assumed to be "1.0"
   */
  CStdString GetXbmcApiVersionDependency(ADDON::AddonPtr addon);

};

