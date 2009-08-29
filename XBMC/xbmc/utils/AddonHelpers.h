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

#include "Addon.h"
#include "../addons/lib/addon_local.h"

namespace ADDON
{

class CAddonUtils
{
public:
  CAddonUtils(void) {};
  ~CAddonUtils() {};

  static void CreateAddOnCallbacks(AddonCB *cbTable);

  /* General Functions */
  static void AddOnLog(void *addonData, const addon_log_t loglevel, const char *msg);
  static void AddonStatusHandler(void *addonData, const ADDON_STATUS status, const char* msg);
  static void OpenAddonSettings(void *addonData);
  static void TransferAddonSettings(const CAddon &addon);
  static bool GetAddonSetting(void *addonData, const char* settingName, void *settingValue);
  static char* GetAddonDirectory(void *addonData);
  static char* GetUserDirectory(void *addonData);

  /* Add-on Utilities helper functions */
  static void Shutdown();
  static void Restart();
  static void Dashboard();
  static void ExecuteScript(const char *script);
  static void ExecuteBuiltIn(const char *function);
  static char* ExecuteHttpApi(const char *httpcommand);
  static char* GetLocalizedString(const void* addonData, long dwCode);
  static char* GetSkinDir();
  static char* UnknownToUTF8(const char *sourceDest);
  static char* GetLanguage();
  static char* GetIPAddress();
  static int GetDVDState();
  static int GetFreeMem();
  static char* GetInfoLabel(const char *infotag);
  static char* GetInfoImage(const char *infotag);
  static bool GetCondVisibility(const char *condition);
  static bool CreateDirectory(const char *dir);
  static void EnableNavSounds(bool yesNo);
  static void PlaySFX(const char *filename);
  static int GetGlobalIdleTime();
  static char* GetCacheThumbName(const char *path);
  static char* MakeLegalFilename(const char *filename);
  static char* TranslatePath(const char *path);
  static char* GetRegion(int id);
  static char* GetSupportedMedia(int media);
  static bool SkinHasImage(const char *filename);
  static void FreeDemuxPacket(demux_packet* pPacket);
  static demux_packet* AllocateDemuxPacket(int iDataSize);


  /* Add-on Dialog helper functions */
  static bool OpenDialogOK(const char* heading, const char* line1, const char* line2, const char* line3);
  static bool OpenDialogYesNo(const char* heading, const char* line1, const char* line2, const char* line3, const char* nolabel, const char* yeslabel);
  static char* OpenDialogBrowse(int type, const char* heading, const char* shares, const char* mask, bool useThumbs, bool treatAsFolder, const char* default_folder);
  static char* OpenDialogNumeric(int type, const char* heading, const char* default_value);
  static char* OpenDialogKeyboard(const char* heading, const char* default_value, bool hidden);
  static int OpenDialogSelect(const char* heading, addon_string_list_s* list);
  static bool ProgressDialogCreate(const char* heading, const char* line1, const char* line2, const char* line3);
  static void ProgressDialogUpdate(int percent, const char* line1, const char* line2, const char* line3);
  static bool ProgressDialogIsCanceled();
  static void ProgressDialogClose();

  /* Add-on GUI helper functions */
  static void GUILock();
  static void GUIUnlock();
  static int GUIGetCurrentWindowId();
  static int GUIGetCurrentWindowDialogId();

private:
  static CStdString GetAddonTypeName(AddonType type);
  static int m_iGUILockRef;
};

}; /* namespace ADDON */
