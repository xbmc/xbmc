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
#ifndef ADDONMANAGER_H
#define ADDONMANAGER_H

#include "Settings.h"
#include "Addon.h"
#include "../Scraper.h"
#include "../addons/include/libaddon.h"
#include "tinyXML/tinyxml.h"
#include "Thread.h"
#include "StdString.h"
#include "DateTime.h"
#include <vector>
#include <map>

class CCriticalSection;

namespace ADDON
{
  typedef std::vector<AddonPtr> VECADDONS;
  typedef std::vector<AddonPtr>::iterator IVECADDONS;
  typedef std::map<TYPE, VECADDONS> MAPADDONS;

  const int        ADDON_DIRSCAN_FREQ         = 60;
  const CStdString ADDON_METAFILE             = "description.xml";
  const CStdString ADDON_MULTITYPE_EXT        = "*.add";
  const CStdString ADDON_VIZ_EXT              = "*.vis";
  const CStdString ADDON_SKIN_EXT             = "*.skin";
  const CStdString ADDON_PVRDLL_EXT           = "*.pvr";
  const CStdString ADDON_SCRIPT_EXT           = "*.py";
  const CStdString ADDON_SCRAPER_EXT          = "*.xml|*.idl";
  const CStdString ADDON_SCREENSAVER_EXT      = "*.xbs";
  const CStdString ADDON_PLUGIN_PVR_EXT       = "*.py|*.plpvr";
  const CStdString ADDON_PLUGIN_MUSIC_EXT     = "*.py|*.plmus";
  const CStdString ADDON_PLUGIN_VIDEO_EXT     = "*.py|*.plvid";
  const CStdString ADDON_PLUGIN_PROGRAM_EXT   = "*.py|*.plpro";
  const CStdString ADDON_PLUGIN_PICTURES_EXT  = "*.py|*.plpic";
  const CStdString ADDON_PLUGIN_WEATHER_EXT   = "*.py|*.plwea";
  const CStdString ADDON_DSP_AUDIO_EXT        = "*.adsp";
  const CStdString ADDON_VERSION_RE = "(?<Major>\\d*)\\.?(?<Minor>\\d*)?\\.?(?<Build>\\d*)?\\.?(?<Revision>\\d*)?";

  /**
  * Class - IAddonCallback
  * Used to access Add-on internal functions
  * The callback is handled from the parent class which
  * handle this types of Add-on's, as example for PVR Clients
  * it is CPVRManager
  */
  class IAddonCallback
  {
    public:
      virtual bool RequestRestart(const IAddon* addon, bool datachanged)=0;
      virtual bool RequestRemoval(const IAddon* addon)=0;
      virtual ADDON_STATUS SetSetting(const IAddon* addon, const char *settingName, const void *settingValue)=0;
      virtual addon_settings_t GetSettings(const IAddon* addon)=0;
  };

  /**
  * Class - CAddonStatusHandler
  * Used to informate the user about occurred errors and
  * changes inside Add-on's, and ask him what to do.
  * It can executed in the same thread as the calling
  * function or in a seperate thread.
  */
  class CAddonStatusHandler : private CThread
  {
    public:
      CAddonStatusHandler(IAddon* const addon, ADDON_STATUS status, CStdString message, bool sameThread = true);
      ~CAddonStatusHandler();

      /* Thread handling */
      virtual void Process();
      virtual void OnStartup();
      virtual void OnExit();

    private:
      static CCriticalSection   m_critSection;
      IAddon*                   m_addon;
      ADDON_STATUS              m_status;
      CStdString                m_message;
  };

  /**
  * Class - CAddonMgr
  * Holds references to all addons, enabled or
  * otherwise. Services the generic callbacks available
  * to all addon variants.
  */
  class CAddonMgr : public IAddonCallback
  {
  public:
    static CAddonMgr* Get();
    virtual ~CAddonMgr();

    IAddonCallback* GetCallbackForType(TYPE type);
    bool RegisterAddonCallback(TYPE type, IAddonCallback* cb);
    void UnregisterAddonCallback(TYPE type);

    /* Dll/so callbacks */
    addon_settings_t GetSettings(const IAddon* addon) { return NULL; }
    ADDON_STATUS SetSetting(const IAddon* addon, const char *settingName, const void *settingValue);
    bool RequestRestart(const IAddon* addon, bool datachanged);
    bool RequestRemoval(const IAddon* addon);

    /* Addon access */
    bool GetDefaultScraper(CScraperPtr &scaper, const CONTENT_TYPE &content);
    bool GetDefaultScraper(AddonPtr &scraper, const CONTENT_TYPE &content);
    bool GetAddon(const TYPE &type, const CStdString &str, AddonPtr &addon);
    bool GetAddonFromPath(const CStdString &path, AddonPtr &addon);
    bool HasAddons(const TYPE &type);
    bool GetAddons(const TYPE &type, VECADDONS &addons, const CONTENT_TYPE &content = CONTENT_NONE, bool enabled = true, bool refresh = false);
    bool GetAddons(const TYPE &type, VECADDONPROPS &addons, const CONTENT_TYPE &content = CONTENT_NONE, bool enabled = true, bool refresh = false);
    CStdString GetString(const CStdString &uuid, const int number);

    /* Addon operations */
    bool EnableAddon(AddonPtr &addon);
    bool DisableAddon(AddonPtr &addon);
    bool Clone(const AddonPtr& parent, AddonPtr& child);

    bool SaveAddonsXML(const TYPE &type);
    bool LoadAddonsXML(const TYPE &type);

  protected:
    void FindAddons(const TYPE &type, const bool refresh = false);
    bool AddonFromInfoXML(const TYPE &reqType, const CStdString &path, AddonPtr &addon);
    CStdString GetAddonsFile() const;
    CStdString GetAddonsFolder() const;

    bool SetAddons(TiXmlNode *root, const TYPE &type, const VECADDONS &addons);
    void GetAddons(const TiXmlElement* pRootElement, const TYPE &type);
    bool GetAddon(const TYPE &type, const TiXmlNode *node, AddonPtr &addon);

  private:
    CAddonMgr();
    static CAddonMgr* m_pInstance;

    static std::map<TYPE, IAddonCallback*> m_managers;
    MAPADDONS m_addons;
    std::map<TYPE, CDateTime> m_lastScan;
    std::map<CStdString, AddonPtr> m_uuidMap;
  };

}; /* namespace ADDON */

#endif /* ADDONMANAGER_H */
