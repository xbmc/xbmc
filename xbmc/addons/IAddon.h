#pragma once
/*
*      Copyright (C) 2005-2009 Team XBMC
*      http://www.xbmc.org
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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/
#include "boost/shared_ptr.hpp"
#include "StdString.h"
#include <set>
#include <map>

class TiXmlElement;

typedef enum
{
  CONTENT_MOVIES,
  CONTENT_TVSHOWS,
  CONTENT_MUSICVIDEOS,
  CONTENT_EPISODES,
  CONTENT_ALBUMS,
  CONTENT_ARTISTS,
  CONTENT_NONE,
  CONTENT_AUDIO,
  CONTENT_IMAGE,
  CONTENT_PROGRAM,
  CONTENT_VIDEO
} CONTENT_TYPE;

namespace ADDON
{
  typedef enum
  {
    ADDON_UNKNOWN,
    ADDON_VIZ,
    ADDON_SKIN,
    ADDON_PVRDLL,
    ADDON_SCRIPT,
    ADDON_SCRAPER,
    ADDON_SCREENSAVER,
    ADDON_PLUGIN,
    ADDON_REPOSITORY,
    ADDON_VIZ_LIBRARY, // add noninstallable after this and installable before
    ADDON_SCRAPER_LIBRARY
  } TYPE;

  class IAddon;
  typedef boost::shared_ptr<IAddon> AddonPtr;
  class CVisualisation;
  typedef boost::shared_ptr<CVisualisation> VizPtr;

  class CAddonMgr;
  struct AddonVersion;
  typedef std::map<CStdString, std::pair<const AddonVersion, const AddonVersion> > ADDONDEPS;
  struct AddonProps;

  class IAddon
  {
  public:
    virtual ~IAddon() {};
    virtual AddonPtr Clone(const AddonPtr& self) const =0;
    virtual const TYPE Type() const =0;
    virtual AddonProps Props() const =0;
    virtual AddonProps& Props() =0;
    virtual const CStdString ID() const =0;
    virtual const AddonPtr Parent() const =0;
    virtual const CStdString Name() const =0;
    virtual bool Enabled() const =0;
    virtual const AddonVersion Version() =0;
    virtual const CStdString Summary() const =0;
    virtual const CStdString Description() const =0;
    virtual const CStdString Path() const =0;
    virtual const CStdString Profile() const =0;
    virtual const CStdString LibName() const =0;
    virtual const CStdString Author() const =0;
    virtual const CStdString Icon() const =0;
    virtual const int  Stars() const =0;
    virtual const CStdString Disclaimer() const =0;
    virtual bool Supports(const CONTENT_TYPE &content) const =0;
    virtual bool HasSettings() =0;
    virtual bool LoadSettings() =0;
    virtual void SaveSettings() =0;
    virtual void SaveFromDefault() =0;
    virtual void UpdateSetting(const CStdString& key, const CStdString& value, const CStdString &type = "") =0;
    virtual CStdString GetSetting(const CStdString& key) const =0;
    virtual TiXmlElement* GetSettingsXML() =0;
    virtual CStdString GetString(uint32_t id) =0;
    virtual ADDONDEPS& GetDeps() =0;

  private:
    friend class CAddonMgr;
    virtual bool IsAddonLibrary() =0;
    virtual void Enable() =0;
    virtual void Disable() =0;
    virtual bool LoadStrings() =0;
    virtual void ClearStrings() =0;
  };
};

