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

#include "StdString.h"
#include "utils/Thread.h"
#include "FileSystem/Directory.h"
#include "../addons/IndependentHeaders/xbmc_addon_types.h"
#include <vector>

class CURL;

namespace ADDON
{

class CAddon;

enum AddonType
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
};

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

/*!
\ingroup windows
\brief Represents an Addon.
\sa VECAddon, IVECADDONS
*/
class CAddon
{
public:
  CAddon(void);
  ~CAddon() {};
  void Reset();
  bool operator==(const CAddon &rhs) const;

  virtual void Remove() {};
  virtual ADDON_STATUS SetSetting(const char *settingName, const void *settingValue) { return STATUS_UNKNOWN; };

  /* Callback pointer return function */
  static IAddonCallback* GetCallbackForType(AddonType type);
  static bool RegisterAddonCallback(AddonType type, IAddonCallback* cb);
  static void UnregisterAddonCallback(AddonType type);

  /* Add-on language functions */
  static void LoadAddonStrings(const CURL &url);
  static void ClearAddonStrings();

  /* Copy existing add-on and reuse it again with another GUID */
  static bool CreateChildAddon(const CAddon &parent, CAddon &child);

  /* Beginning of Add-on data fields (readed from info.xml) */
  CStdString m_guid;       ///< Unique identifier for this addon, chosen by developer
  CStdString m_guid_parent;///< Unique identifier of the parent for this child addon, chosen by developer
  CStdString m_strName;    ///< Name of the addon, can be chosen freely.
  CStdString m_strVersion; ///< Version of the addon, must be in form
  CStdString m_summary;    ///< Short summary of addon
  CStdString m_strDesc;    ///< Description of addon
  CStdString m_strPath;    ///< Path to the addon
  CStdString m_strLibName; ///< Name of the library
  CStdString m_strCreator; ///< Author(s) of the addon
  CStdString m_icon;       ///< Path to icon for the addon, or blank by default
  int        m_stars;      ///< Rating
  CStdString m_disclaimer; ///< if exists, user needs to confirm before installation
  bool       m_disabled;   ///< Is this addon disabled?
  AddonType  m_addonType;  ///< Type identifier of this Add-on
  int        m_childs;     ///< How many child add-on's are present

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
};


/*!
\ingroup windows
\brief A vector to hold CAddon objects.
\sa CAddon, IVECADDONS
*/
typedef std::vector<CAddon> VECADDONS;

/*!
\ingroup windows
\brief Iterator of VECADDONS.
\sa CAddon, VECADDONS
*/
typedef std::vector<CAddon>::iterator IVECADDONS;

}; /* namespace ADDON */

