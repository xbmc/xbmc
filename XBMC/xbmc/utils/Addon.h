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
#include "../addons/DllAddonTypes.h"
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
  ADDON_SCRAPER           = 5,
  ADDON_SCREENSAVER       = 6,
  ADDON_PLUGIN_PVR        = 7,
  ADDON_PLUGIN_MUSIC      = 8,
  ADDON_PLUGIN_VIDEO      = 9,
  ADDON_PLUGIN_PROGRAM    = 10,
  ADDON_PLUGIN_PICTURES   = 11 
};

/**
* IAddonCallback
*/
class IAddonCallback
{
public:
  virtual bool RequestRestart(const CAddon* addon, bool datachanged)=0;
  virtual bool RequestRemoval(const CAddon* addon)=0;
  virtual bool SetSetting(const CAddon* addon, const char *settingName, const void *settingValue)=0;
};

const CStdString ADDON_PVRDLL_EXT = "*.pvr";
const CStdString ADDON_GUID_RE = "^(\\{){0,1}[0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12}(\\}){0,1}$";
const CStdString ADDON_VERSION_RE = "(?<Major>\\d*)\\.?(?<Minor>\\d*)?\\.?(?<Build>\\d*)?\\.?(?<Revision>\\d*)?";

/**
 * CAddonStatusHandler
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
  bool operator==(const CAddon &rhs) const;

  virtual void Remove() {};
  virtual bool SetSetting(const char *settingName, const void *settingValue) { return false; };

  static void LoadAddonStrings(const CURL &url);
  static void ClearAddonStrings();
  static void OpenAddonSettings(const CURL &url, bool bReload = true);
  static void OpenAddonSettings(const CAddon* addon, bool bReload);
  static bool GetAddonSetting(const CAddon* addon, const char* settingName, void *settingValue);

  static bool OpenDialogOK(const char* heading, const char* line1, const char* line2, const char* line3);
  static bool OpenDialogYesNo(const char* heading, const char* line1, const char* line2, const char* line3, const char* nolabel, const char* yeslabel);
  static const char* OpenDialogBrowse(int type, const char* heading, const char* shares, const char* mask, bool useThumbs, bool treatAsFolder, const char* default_folder);
  static const char* OpenDialogNumeric(int type, const char* heading, const char* default_value);
  static const char* OpenDialogKeyboard(const char* heading, const char* default_value, bool hidden);
  static int OpenDialogSelect(const char* heading, AddOnStringList* list);
  static bool ProgressDialogCreate(const char* heading, const char* line1, const char* line2, const char* line3);
  static void ProgressDialogUpdate(int percent, const char* line1, const char* line2, const char* line3);
  static bool ProgressDialogIsCanceled();
  static void ProgressDialogClose();

  static void GUILock();
  static void GUIUnlock();
  static int GUIGetCurrentWindowId();
  static int GUIGetCurrentWindowDialogId();

  static void Shutdown();
  static void Restart();
  static void Dashboard();
  static void ExecuteScript(const char *script);
  static void ExecuteBuiltIn(const char *function);
  static const char* ExecuteHttpApi(char *httpcommand);
  static const char* GetLocalizedString(const CAddon* addon, long dwCode);
  static const char* GetSkinDir();
  static const char* UnknownToUTF8(const char *sourceDest);
  static const char* GetLanguage();
  static const char* GetIPAddress();
  static int GetDVDState();
  static int GetFreeMem();
  static const char* GetInfoLabel(const char *infotag);
  static const char* GetInfoImage(const char *infotag);
  static bool GetCondVisibility(const char *condition);
  static void EnableNavSounds(bool yesNo);
  static void PlaySFX(const char *filename);
  static int GetGlobalIdleTime();
  static const char* GetCacheThumbName(const char *path);
  static const char* MakeLegalFilename(const char *filename);
  static const char* TranslatePath(const char *path);
  static const char* GetRegion(const char *id);
  static const char* GetSupportedMedia(const char *media);
  static bool SkinHasImage(const char *filename);

  static void TransferAddonSettings(const CURL &url);
  static void TransferAddonSettings(const CAddon* addon);
  static IAddonCallback* GetCallbackForType(AddonType type);

  CStdString m_guid;       ///< Unique identifier for this addon, chosen by developer
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

  AddonType m_addonType;
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

