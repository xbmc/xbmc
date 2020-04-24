/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/interfaces/AddonInterfaces.h"
#include "addons/kodi-addon-dev-kit/include/kodi/libXBMC_addon.h"

struct VFSDirEntry;

#ifdef TARGET_WINDOWS
#ifndef _SSIZE_T_DEFINED
typedef intptr_t ssize_t;
#define _SSIZE_T_DEFINED
#endif // !_SSIZE_T_DEFINED
#endif // TARGET_WINDOWS

namespace ADDON
{
  class CAddon;
};

namespace KodiAPI
{
namespace AddOn
{

class CAddonCallbacksAddon
{
public:
  explicit CAddonCallbacksAddon(ADDON::CAddon* addon);
  virtual ~CAddonCallbacksAddon();

  /*!
   * @return The callback table.
   */
  CB_AddOnLib *GetCallbacks() { return m_callbacks; }

  static void AddOnLog(void *addonData, const int addonLogLevel, const char *strMessage);
  static bool GetAddonSetting(void *addonData, const char *strSettingName, void *settingValue);
  static char *TranslateSpecialProtocol(const char *strSource);
  static void QueueNotification(void *addonData, const int type, const char *strMessage);
  static bool WakeOnLan(const char *mac);
  static char* UnknownToUTF8(const char *strSource);
  static char* GetLocalizedString(const void* addonData, long dwCode);
  static char* GetDVDMenuLanguage(const void* addonData);
  static void FreeString(const void* addonData, char* str);
  static void FreeStringArray(const void* addonData, char** arr, int numElements);

  // file operations
  static void* OpenFile(const void* addonData, const char* strFileName, unsigned int flags);
  static void* OpenFileForWrite(const void* addonData, const char* strFileName, bool bOverwrite);
  static ssize_t ReadFile(const void* addonData, void* file, void* lpBuf, size_t uiBufSize);
  static bool ReadFileString(const void* addonData, void* file, char *szLine, int iLineLength);
  static ssize_t WriteFile(const void* addonData, void* file, const void* lpBuf, size_t uiBufSize);
  static void FlushFile(const void* addonData, void* file);
  static int64_t SeekFile(const void* addonData, void* file, int64_t iFilePosition, int iWhence);
  static int TruncateFile(const void* addonData, void* file, int64_t iSize);
  static int64_t GetFilePosition(const void* addonData, void* file);
  static int64_t GetFileLength(const void* addonData, void* file);
  static double GetFileDownloadSpeed(const void* addonData, void* file);
  static void CloseFile(const void* addonData, void* file);
  static int GetFileChunkSize(const void* addonData, void* file);
  static bool FileExists(const void* addonData, const char *strFileName, bool bUseCache);
  static int StatFile(const void* addonData, const char *strFileName, struct __stat64* buffer);
  static char *GetFilePropertyValue(const void* addonData, void* file, XFILE::FileProperty type, const char *name);
  static char **GetFilePropertyValues(const void* addonData, void* file, XFILE::FileProperty type, const char *name, int *numValues);
  static bool DeleteFile(const void* addonData, const char *strFileName);
  static bool CanOpenDirectory(const void* addonData, const char* strURL);
  static bool CreateDirectory(const void* addonData, const char *strPath);
  static bool DirectoryExists(const void* addonData, const char *strPath);
  static bool RemoveDirectory(const void* addonData, const char *strPath);
  static bool GetDirectory(const void* addondata, const char* strPath, const char* mask, VFSDirEntry** items, unsigned int* num_items);
  static void FreeDirectory(const void* addondata, VFSDirEntry* items, unsigned int num_items);
  static void* CURLCreate(const void* addonData, const char* strURL);
  static bool CURLAddOption(const void* addonData, void* curl, XFILE::CURLOPTIONTYPE type, const char* name, const char * value);
  static bool CURLOpen(const void* addonData, void* curl, unsigned int flags);

private:
  CAddonCallbacksAddon(const CAddonCallbacksAddon&) = delete;
  CAddonCallbacksAddon& operator=(const CAddonCallbacksAddon&) = delete;

  ADDON::CAddon* m_addon; /*!< the addon */
  CB_AddOnLib  *m_callbacks; /*!< callback addresses */
};

} /* namespace AddOn */
} /* namespace KodiAPI */
