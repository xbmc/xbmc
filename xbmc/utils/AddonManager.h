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

#include "StringUtils.h"
#include "tinyXML/tinyxml.h"
#include "utils/Thread.h"
#include "utils/CriticalSection.h"
#include "../addons/include/xbmc_addon_types.h"
#include <vector>

namespace ADDON
{
  typedef enum
  {
    ADDON_UNKNOWN           = -1,
    ADDON_MULTITYPE         = 0,
    ADDON_VIZ               = 1,
    ADDON_SKIN              = 2,
    ADDON_PVRDLL            = 3,
    ADDON_SCRIPT            = 4,
    ADDON_SCRAPER_PVR       = 5,
    ADDON_SCRAPER_VIDEO     = 6,
    ADDON_SCRAPER_MUSIC     = 7,
    ADDON_SCRAPER_PROGRAM   = 8,
    ADDON_SCREENSAVER       = 9,
    ADDON_PLUGIN_PVR        = 10,
    ADDON_PLUGIN_VIDEO      = 11,
    ADDON_PLUGIN_MUSIC      = 12,
    ADDON_PLUGIN_PROGRAM    = 13,
    ADDON_PLUGIN_PICTURES   = 14,
    ADDON_PLUGIN_WEATHER    = 16,
    ADDON_DSP_AUDIO         = 17
  } AddonType;

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
  const CStdString ADDON_GUID_RE = "^(\\{){0,1}[0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12}(\\}){0,1}$";
  const CStdString ADDON_VERSION_RE = "(?<Major>\\d*)\\.?(?<Minor>\\d*)?\\.?(?<Build>\\d*)?\\.?(?<Revision>\\d*)?";

  class CAddon;

  typedef std::vector<CAddon> VECADDONS;
  typedef std::vector<CAddon>::iterator IVECADDONS;

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
    CAddonStatusHandler(const CAddon* addon, ADDON_STATUS status, CStdString message, bool sameThread = true);
    ~CAddonStatusHandler();

    /* Thread handling */
    virtual void Process();
    virtual void OnStartup();
    virtual void OnExit();

  private:
    static CCriticalSection   m_critSection;
    const CAddon*             m_addon;
    ADDON_STATUS              m_status;
    CStdString                m_message;
  };

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
      virtual bool RequestRestart(const CAddon* addon, bool datachanged)=0;
      virtual bool RequestRemoval(const CAddon* addon)=0;
      virtual ADDON_STATUS SetSetting(const CAddon* addon, const char *settingName, const void *settingValue)=0;
  };

  /**
  * Class - CAddonDummyCallback
  * Used as fallback by unkown Add-on type
  */
  class CAddonDummyCallback : public IAddonCallback
  {
    public:
      CAddonDummyCallback() {}
      ~CAddonDummyCallback() {}
      bool RequestRestart(const CAddon* addon, bool datachanged) { return false; }
      bool RequestRemoval(const CAddon* addon) { return false; }
      ADDON_STATUS SetSetting(const CAddon* addon, const char *settingName, const void *settingValue) { return STATUS_UNKNOWN; }
      addon_settings_t GetSettings(const CAddon* addon) { return NULL; }
  };

  /**
   * Class - CAddonManager
   */
  class CAddonManager
  {
  public:
    CAddonManager();
    virtual ~CAddonManager();

    IAddonCallback* GetCallbackForType(AddonType type);
    bool RegisterAddonCallback(AddonType type, IAddonCallback* cb);
    void UnregisterAddonCallback(AddonType type);

    void LoadAddons();
    bool SaveAddons();
    void UpdateAddons();
    VECADDONS *GetAllAddons();
    VECADDONS *GetAddonsFromType(const AddonType &type);
    bool GetAddonFromGUID(const CStdString &guid, CAddon &addon);
    bool GetAddonFromNameAndType(const CStdString &name, const AddonType &type, CAddon &addon);
    bool DisableAddon(const CStdString &addon, const ADDON::AddonType &type);
    void SaveVirtualAddon(CAddon &addon);

  protected:
    bool AddonFromInfoXML(const CStdString &path, ADDON::CAddon &addon);
    bool SetAddons(TiXmlNode *root, const AddonType &type, const VECADDONS &addons);
    void GetAddons(const TiXmlElement* pRootElement, const ADDON::AddonType &type);
    bool GetAddon(const ADDON::AddonType &type, const TiXmlNode *node, ADDON::CAddon &addon);

    CStdString GetAddonsFile() const;
    CStdString GetAddonsFolder() const;

  private:
    static IAddonCallback *m_cbMultitye;
    static IAddonCallback *m_cbViz;
    static IAddonCallback *m_cbSkin;
    static IAddonCallback *m_cbPVR;
    static IAddonCallback *m_cbScript;
    static IAddonCallback *m_cbScraperPVR;
    static IAddonCallback *m_cbScraperVideo;
    static IAddonCallback *m_cbScraperMusic;
    static IAddonCallback *m_cbScraperProgram;
    static IAddonCallback *m_cbScreensaver;
    static IAddonCallback *m_cbPluginPVR;
    static IAddonCallback *m_cbPluginVideo;
    static IAddonCallback *m_cbPluginMusic;
    static IAddonCallback *m_cbPluginProgram;
    static IAddonCallback *m_cbPluginPictures;
    static IAddonCallback *m_cbPluginWeather;
    static IAddonCallback *m_cbDSPAudio;

    VECADDONS  m_allAddons;
    VECADDONS  m_virtualAddons;

    VECADDONS  m_multitypeAddons;
    VECADDONS  m_visualisationAddons;
    VECADDONS  m_skinAddons;
    VECADDONS  m_pvrAddons;
    VECADDONS  m_scriptAddons;
    VECADDONS  m_scraperPVRAddons;
    VECADDONS  m_scraperVideoAddons;
    VECADDONS  m_scraperMusicAddons;
    VECADDONS  m_scraperProgramAddons;
    VECADDONS  m_screensaverAddons;
    VECADDONS  m_pluginPvrAddons;
    VECADDONS  m_pluginMusicAddons;
    VECADDONS  m_pluginVideoAddons;
    VECADDONS  m_pluginProgramAddons;
    VECADDONS  m_pluginPictureAddons;
    VECADDONS  m_pluginWeatherAddons;
    VECADDONS  m_DSPAudioAddons;
  };

  extern class CAddonManager g_addonmanager;

}; /* namespace ADDON */
