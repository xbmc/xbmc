#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AddonCallbacks.h"

namespace ADDON
{

class CAddonCallbacksAddon
{
public:
  CAddonCallbacksAddon(CAddon* addon);
  ~CAddonCallbacksAddon();

  /*!
   * @return The callback table.
   */
  CB_AddOnLib *GetCallbacks() { return m_callbacks; }

  static void AddOnLog(void *addonData, const addon_log_t addonLogLevel, const char *strMessage);
  static bool GetAddonSetting(void *addonData, const char *strSettingName, void *settingValue);
  static void QueueNotification(void *addonData, const queue_msg_t type, const char *strMessage);
  static char* UnknownToUTF8(const char *strSource);
  static char* GetLocalizedString(const void* addonData, long dwCode);
  static char* GetDVDMenuLanguage(const void* addonData);
  static void FreeString(const void* addonData, char* str);

  // file operations
  static void* OpenFile(const void* addonData, const char* strFileName, unsigned int flags);
  static void* OpenFileForWrite(const void* addonData, const char* strFileName, bool bOverwrite);
  static unsigned int ReadFile(const void* addonData, void* file, void* lpBuf, int64_t uiBufSize);
  static bool ReadFileString(const void* addonData, void* file, char *szLine, int iLineLength);
  static int WriteFile(const void* addonData, void* file, const void* lpBuf, int64_t uiBufSize);
  static void FlushFile(const void* addonData, void* file);
  static int64_t SeekFile(const void* addonData, void* file, int64_t iFilePosition, int iWhence);
  static int TruncateFile(const void* addonData, void* file, int64_t iSize);
  static int64_t GetFilePosition(const void* addonData, void* file);
  static int64_t GetFileLength(const void* addonData, void* file);
  static void CloseFile(const void* addonData, void* file);
  static int GetFileChunkSize(const void* addonData, void* file);
  static bool FileExists(const void* addonData, const char *strFileName, bool bUseCache);
  static int StatFile(const void* addonData, const char *strFileName, struct __stat64* buffer);
  static bool DeleteFile(const void* addonData, const char *strFileName);
  static bool CanOpenDirectory(const void* addonData, const char* strURL);
  static bool CreateDirectory(const void* addonData, const char *strPath);
  static bool DirectoryExists(const void* addonData, const char *strPath);
  static bool RemoveDirectory(const void* addonData, const char *strPath);

private:
  CB_AddOnLib  *m_callbacks; /*!< callback addresses */
  CAddon       *m_addon;     /*!< the add-on */
};

}; /* namespace ADDON */
