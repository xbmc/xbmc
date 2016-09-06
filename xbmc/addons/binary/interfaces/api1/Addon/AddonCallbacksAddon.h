#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      Copyright (C) 2015-2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/binary/interfaces/AddonInterfaces.h"
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

namespace V1
{
namespace KodiAPI
{

namespace AddOn
{

typedef void (*AddOnLogCallback)(void *addonData, const ADDON::addon_log_t loglevel, const char *msg);
typedef void (*AddOnQueueNotification)(void *addonData, const ADDON::queue_msg_t type, const char *msg);
typedef bool (*AddOnWakeOnLan)(const char* mac);
typedef bool (*AddOnGetSetting)(void *addonData, const char *settingName, void *settingValue);
typedef char* (*AddonTranslateSpecialProtocol)(const char *strSource);
typedef char* (*AddOnUnknownToUTF8)(const char *sourceDest);
typedef char* (*AddOnGetLocalizedString)(const void* addonData, long dwCode);
typedef char* (*AddOnGetDVDMenuLanguage)(const void* addonData);
typedef void (*AddOnFreeString)(const void* addonData, char* str);

typedef void* (*AddOnOpenFile)(const void* addonData, const char* strFileName, unsigned int flags);
typedef void* (*AddOnOpenFileForWrite)(const void* addonData, const char* strFileName, bool bOverWrite);
typedef ssize_t (*AddOnReadFile)(const void* addonData, void* file, void* lpBuf, size_t uiBufSize);
typedef bool (*AddOnReadFileString)(const void* addonData, void* file, char *szLine, int iLineLength);
typedef ssize_t (*AddOnWriteFile)(const void* addonData, void* file, const void* lpBuf, size_t uiBufSize);
typedef void (*AddOnFlushFile)(const void* addonData, void* file);
typedef int64_t (*AddOnSeekFile)(const void* addonData, void* file, int64_t iFilePosition, int iWhence);
typedef int (*AddOnTruncateFile)(const void* addonData, void* file, int64_t iSize);
typedef int64_t (*AddOnGetFilePosition)(const void* addonData, void* file);
typedef int64_t (*AddOnGetFileLength)(const void* addonData, void* file);
typedef double (*AddOnGetFileDownloadSpeed)(const void* addonData, void* file);
typedef void (*AddOnCloseFile)(const void* addonData, void* file);
typedef int (*AddOnGetFileChunkSize)(const void* addonData, void* file);
typedef bool (*AddOnFileExists)(const void* addonData, const char *strFileName, bool bUseCache);
typedef int (*AddOnStatFile)(const void* addonData, const char *strFileName, struct __stat64* buffer);
typedef bool (*AddOnDeleteFile)(const void* addonData, const char *strFileName);
typedef bool (*AddOnCanOpenDirectory)(const void* addonData, const char* strURL);
typedef bool (*AddOnCreateDirectory)(const void* addonData, const char *strPath);
typedef bool (*AddOnDirectoryExists)(const void* addonData, const char *strPath);
typedef bool (*AddOnRemoveDirectory)(const void* addonData, const char *strPath);
typedef bool (*AddOnGetDirectory)(const void* addonData, const char *strPath, const char* mask, VFSDirEntry** items, unsigned int* num_items);
typedef void (*AddOnFreeDirectory)(const void* addonData, VFSDirEntry* items, unsigned int num_items);
typedef void* (*AddOnCURLCreate)(const void* addonData, const char* strURL);
typedef bool (*AddOnCURLAddOption)(const void* addonData, void* file, XFILE::CURLOPTIONTYPE type, const char* name, const char * value);
typedef bool (*AddOnCURLOpen)(const void* addonData, void* file, unsigned int flags);

typedef struct CB_AddOn
{
  AddOnLogCallback            Log;
  AddOnQueueNotification      QueueNotification;
  AddOnWakeOnLan              WakeOnLan;
  AddOnGetSetting             GetSetting;
  AddonTranslateSpecialProtocol TranslateSpecialProtocol;
  AddOnUnknownToUTF8          UnknownToUTF8;
  AddOnGetLocalizedString     GetLocalizedString;
  AddOnGetDVDMenuLanguage     GetDVDMenuLanguage;
  AddOnFreeString             FreeString;

  AddOnOpenFile               OpenFile;
  AddOnOpenFileForWrite       OpenFileForWrite;
  AddOnReadFile               ReadFile;
  AddOnReadFileString         ReadFileString;
  AddOnWriteFile              WriteFile;
  AddOnFlushFile              FlushFile;
  AddOnSeekFile               SeekFile;
  AddOnTruncateFile           TruncateFile;
  AddOnGetFilePosition        GetFilePosition;
  AddOnGetFileLength          GetFileLength;
  AddOnGetFileDownloadSpeed   GetFileDownloadSpeed;
  AddOnCloseFile              CloseFile;
  AddOnGetFileChunkSize       GetFileChunkSize;
  AddOnFileExists             FileExists;
  AddOnStatFile               StatFile;
  AddOnDeleteFile             DeleteFile;
  AddOnCanOpenDirectory       CanOpenDirectory;
  AddOnCreateDirectory        CreateDirectory;
  AddOnDirectoryExists        DirectoryExists;
  AddOnRemoveDirectory        RemoveDirectory;
  AddOnGetDirectory           GetDirectory;
  AddOnFreeDirectory          FreeDirectory;
  AddOnCURLCreate             CURLCreate;
  AddOnCURLAddOption          CURLAddOption;
  AddOnCURLOpen               CURLOpen;
} CB_AddOnLib;

class CAddonCallbacksAddon : public ADDON::IAddonInterface
{
public:
  CAddonCallbacksAddon(ADDON::CAddon* addon);
  virtual ~CAddonCallbacksAddon();

  /*!
   * @return The callback table.
   */
  CB_AddOnLib *GetCallbacks() { return m_callbacks; }

  static void AddOnLog(void *addonData, const ADDON::addon_log_t addonLogLevel, const char *strMessage);
  static bool GetAddonSetting(void *addonData, const char *strSettingName, void *settingValue);
  static char *TranslateSpecialProtocol(const char *strSource);
  static void QueueNotification(void *addonData, const ADDON::queue_msg_t type, const char *strMessage);
  static bool WakeOnLan(const char *mac);
  static char* UnknownToUTF8(const char *strSource);
  static char* GetLocalizedString(const void* addonData, long dwCode);
  static char* GetDVDMenuLanguage(const void* addonData);
  static void FreeString(const void* addonData, char* str);

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
  CB_AddOnLib  *m_callbacks; /*!< callback addresses */
};

} /* namespace AddOn */

} /* namespace KodiAPI */
} /* namespace V1 */
