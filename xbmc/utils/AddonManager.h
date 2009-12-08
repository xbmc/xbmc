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

#include "IAddon.h"
#include "StringUtils.h"
#include "tinyXML/tinyxml.h"
#include "utils/Thread.h"
#include "utils/CriticalSection.h"
#include "../addons/include/xbmc_addon_types.h"
#include <vector>
#include <map>

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
      CAddonStatusHandler(IAddon* addon, ADDON_STATUS status, CStdString message, bool sameThread = true);
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

  class CAddon;

//  typedef std::vector<CAddon> VECADDONS;
//  typedef std::vector<CAddon>::iterator IVECADDONS;

  /**
   * Class - CAddonMgr
   */
  class CAddonMgr
  {
  public:
    static CAddonMgr* Get();
    virtual ~CAddonMgr();

    IAddonCallback* GetCallbackForType(TYPE type);
    bool RegisterAddonCallback(TYPE type, IAddonCallback* cb);
    void UnregisterAddonCallback(TYPE type);





    void LoadAddons();
    bool SaveAddons();
    void UpdateAddons();
    VECADDONS *GetAllAddons();
    VECADDONS *GetAddonsFromType(const TYPE &type);
    bool GetAddonFromGUID(const CStdString &guid, AddonPtr &addon);
    bool GetAddonFromNameAndType(const CStdString &name, const TYPE &type, AddonPtr &addon);
    bool DisableAddon(const CStdString &addon, const ADDON::TYPE &type);

  protected:
    bool AddonFromInfoXML(const CStdString &path, AddonPtr &addon);
    bool SetAddons(TiXmlNode *root, const TYPE &type, const VECADDONS &addons);
    void GetAddons(const TiXmlElement* pRootElement, const ADDON::TYPE &type);
    bool GetAddon(const ADDON::TYPE &type, const TiXmlNode *node, AddonPtr &addon);

    CStdString GetAddonsFile() const;
    CStdString GetAddonsFolder() const;

  private:
    CAddonMgr();
    static CAddonMgr* m_pInstance;

    static std::map<TYPE, IAddonCallback*> m_managers;

    VECADDONS  m_allAddons;
    VECADDONS  m_virtualAddons;

    VECADDONS  m_multitypeAddons;
    VECADDONS  m_visualisationAddons;
    VECADDONS  m_skinAddons;
    VECADDONS  m_pvrAddons;
    VECADDONS  m_scriptAddons;
    VECADDONS  m_scraperAddons;
    VECADDONS  m_screensaverAddons;
    VECADDONS  m_pluginAddons;
    VECADDONS  m_DSPAudioAddons;
  };

}; /* namespace ADDON */
