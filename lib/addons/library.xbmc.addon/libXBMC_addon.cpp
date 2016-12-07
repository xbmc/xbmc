/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <sys/stat.h>
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_addon_types.h"
#include "addons/kodi-addon-dev-kit/include/kodi/libXBMC_addon.h"
#include "addons/binary/interfaces/api1/Addon/AddonCallbacksAddon.h"

#ifdef _WIN32
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif


using namespace std;
using namespace ADDON;
using namespace KodiAPI::V1::AddOn;

extern "C"
{

DLLEXPORT void* XBMC_register_me(void *hdl)
{
  CB_AddOnLib *cb = NULL;
  if (!hdl)
    fprintf(stderr, "libXBMC_addon-ERROR: XBMC_register_me is called with NULL handle !!!\n");
  else
  {
    cb = (CB_AddOnLib*)((AddonCB*)hdl)->AddOnLib_RegisterMe(((AddonCB*)hdl)->addonData);
    if (!cb)
      fprintf(stderr, "libXBMC_addon-ERROR: XBMC_register_me can't get callback table from XBMC !!!\n");
  }
  return cb;
}

DLLEXPORT void XBMC_unregister_me(void *hdl, void* cb)
{
  if (hdl && cb)
    ((AddonCB*)hdl)->AddOnLib_UnRegisterMe(((AddonCB*)hdl)->addonData, ((CB_AddOnLib*)cb));
}

DLLEXPORT void XBMC_log(void *hdl, void* cb, const addon_log_t loglevel, const char *msg)
{
  if (cb == NULL)
    return;

  ((CB_AddOnLib*)cb)->Log(((AddonCB*)hdl)->addonData, loglevel, msg);
}

DLLEXPORT bool XBMC_get_setting(void *hdl, void* cb, const char* settingName, void *settingValue)
{
  if (cb == NULL)
    return false;

  return ((CB_AddOnLib*)cb)->GetSetting(((AddonCB*)hdl)->addonData, settingName, settingValue);
}

DLLEXPORT char* XBMC_translate_special(void *hdl, void* cb, const char* source)
{
  if (cb == NULL)
    return NULL;

  return ((CB_AddOnLib*)cb)->TranslateSpecialProtocol(source);
}

DLLEXPORT void XBMC_queue_notification(void *hdl, void* cb, const queue_msg_t type, const char *msg)
{
  if (cb == NULL)
    return;

  ((CB_AddOnLib*)cb)->QueueNotification(((AddonCB*)hdl)->addonData, type, msg);
}

DLLEXPORT bool XBMC_wake_on_lan(void* hdl, void* cb, char* mac)
{
  if (cb == NULL)
    return false;

  return ((CB_AddOnLib*)cb)->WakeOnLan(mac);
}

DLLEXPORT char* XBMC_unknown_to_utf8(void *hdl, void* cb, const char* str)
{
  if (cb == NULL)
    return NULL;

  return ((CB_AddOnLib*)cb)->UnknownToUTF8(str);
}

DLLEXPORT char* XBMC_get_localized_string(void *hdl, void* cb, int dwCode)
{
  if (cb == NULL)
    return "";

  return ((CB_AddOnLib*)cb)->GetLocalizedString(((AddonCB*)hdl)->addonData, dwCode);
}

DLLEXPORT char* XBMC_get_dvd_menu_language(void *hdl, void* cb)
{
  if (cb == NULL)
    return "";

  return ((CB_AddOnLib*)cb)->GetDVDMenuLanguage(((AddonCB*)hdl)->addonData);
}

DLLEXPORT void XBMC_free_string(void* hdl, void* cb, char* str)
{
  if (cb == NULL)
    return;

  ((CB_AddOnLib*)cb)->FreeString(((AddonCB*)hdl)->addonData, str);
}

DLLEXPORT void* XBMC_open_file(void *hdl, void* cb, const char* strFileName, unsigned int flags)
{
  if (cb == NULL)
    return NULL;

  return ((CB_AddOnLib*)cb)->OpenFile(((AddonCB*)hdl)->addonData, strFileName, flags);
}

DLLEXPORT void* XBMC_open_file_for_write(void *hdl, void* cb, const char* strFileName, bool bOverWrite)
{
  if (cb == NULL)
    return NULL;

  return ((CB_AddOnLib*)cb)->OpenFileForWrite(((AddonCB*)hdl)->addonData, strFileName, bOverWrite);
}

DLLEXPORT ssize_t XBMC_read_file(void *hdl, void* cb, void* file, void* lpBuf, size_t uiBufSize)
{
  if (cb == NULL)
    return -1;

  return ((CB_AddOnLib*)cb)->ReadFile(((AddonCB*)hdl)->addonData, file, lpBuf, uiBufSize);
}

DLLEXPORT bool XBMC_read_file_string(void *hdl, void* cb, void* file, char *szLine, int iLineLength)
{
  if (cb == NULL)
    return false;

  return ((CB_AddOnLib*)cb)->ReadFileString(((AddonCB*)hdl)->addonData, file, szLine, iLineLength);
}

DLLEXPORT ssize_t XBMC_write_file(void *hdl, void* cb, void* file, const void* lpBuf, size_t uiBufSize)
{
  if (cb == NULL)
    return -1;

  return ((CB_AddOnLib*)cb)->WriteFile(((AddonCB*)hdl)->addonData, file, lpBuf, uiBufSize);
}

DLLEXPORT void XBMC_flush_file(void *hdl, void* cb, void* file)
{
  if (cb == NULL)
    return;

  ((CB_AddOnLib*)cb)->FlushFile(((AddonCB*)hdl)->addonData, file);
}

DLLEXPORT int64_t XBMC_seek_file(void *hdl, void* cb, void* file, int64_t iFilePosition, int iWhence)
{
  if (cb == NULL)
    return 0;

  return ((CB_AddOnLib*)cb)->SeekFile(((AddonCB*)hdl)->addonData, file, iFilePosition, iWhence);
}

DLLEXPORT int XBMC_truncate_file(void *hdl, void* cb, void* file, int64_t iSize)
{
  if (cb == NULL)
    return 0;

  return ((CB_AddOnLib*)cb)->TruncateFile(((AddonCB*)hdl)->addonData, file, iSize);
}

DLLEXPORT int64_t XBMC_get_file_position(void *hdl, void* cb, void* file)
{
  if (cb == NULL)
    return 0;

  return ((CB_AddOnLib*)cb)->GetFilePosition(((AddonCB*)hdl)->addonData, file);
}

DLLEXPORT int64_t XBMC_get_file_length(void *hdl, void* cb, void* file)
{
  if (cb == NULL)
    return 0;

  return ((CB_AddOnLib*)cb)->GetFileLength(((AddonCB*)hdl)->addonData, file);
}

DLLEXPORT double XBMC_get_file_download_speed(void *hdl, void* cb, void* file)
{
  if (cb == NULL)
    return 0.0f;

  return ((CB_AddOnLib*)cb)->GetFileDownloadSpeed(((AddonCB*)hdl)->addonData, file);
}

DLLEXPORT void XBMC_close_file(void *hdl, void* cb, void* file)
{
  if (cb == NULL)
    return;

  ((CB_AddOnLib*)cb)->CloseFile(((AddonCB*)hdl)->addonData, file);
}

DLLEXPORT int XBMC_get_file_chunk_size(void *hdl, void* cb, void* file)
{
  if (cb == NULL)
    return 0;

  return ((CB_AddOnLib*)cb)->GetFileChunkSize(((AddonCB*)hdl)->addonData, file);
}

DLLEXPORT bool XBMC_file_exists(void *hdl, void* cb, const char *strFileName, bool bUseCache)
{
  if (cb == NULL)
    return false;

  return ((CB_AddOnLib*)cb)->FileExists(((AddonCB*)hdl)->addonData, strFileName, bUseCache);
}

DLLEXPORT int XBMC_stat_file(void *hdl, void* cb, const char *strFileName, struct __stat64* buffer)
{
  if (cb == NULL)
    return -1;

  return ((CB_AddOnLib*)cb)->StatFile(((AddonCB*)hdl)->addonData, strFileName, buffer);
}

DLLEXPORT bool XBMC_delete_file(void *hdl, void* cb, const char *strFileName)
{
  if (cb == NULL)
    return false;

  return ((CB_AddOnLib*)cb)->DeleteFile(((AddonCB*)hdl)->addonData, strFileName);
}

DLLEXPORT bool XBMC_can_open_directory(void *hdl, void* cb, const char* strURL)
{
  if (cb == NULL)
    return false;

  return ((CB_AddOnLib*)cb)->CanOpenDirectory(((AddonCB*)hdl)->addonData, strURL);
}

DLLEXPORT bool XBMC_create_directory(void *hdl, void* cb, const char *strPath)
{
  if (cb == NULL)
    return false;

  return ((CB_AddOnLib*)cb)->CreateDirectory(((AddonCB*)hdl)->addonData, strPath);
}

DLLEXPORT bool XBMC_directory_exists(void *hdl, void* cb, const char *strPath)
{
  if (cb == NULL)
    return false;

  return ((CB_AddOnLib*)cb)->DirectoryExists(((AddonCB*)hdl)->addonData, strPath);
}

DLLEXPORT bool XBMC_remove_directory(void *hdl, void* cb, const char *strPath)
{
  if (cb == NULL)
    return false;

  return ((CB_AddOnLib*)cb)->RemoveDirectory(((AddonCB*)hdl)->addonData, strPath);
}

DLLEXPORT bool XBMC_get_directory(void *hdl, void* cb, const char *strPath, const char* mask, VFSDirEntry** items, unsigned int* num_items)
{
  if (cb == NULL)
    return false;

  return ((CB_AddOnLib*)cb)->GetDirectory(((AddonCB*)hdl)->addonData, strPath, mask, items, num_items);
}

DLLEXPORT void XBMC_free_directory(void *hdl, void* cb, VFSDirEntry* items, unsigned int num_items)
{
  if (cb == NULL)
    return;

  ((CB_AddOnLib*)cb)->FreeDirectory(((AddonCB*)hdl)->addonData, items, num_items);
}

DLLEXPORT void* XBMC_curl_create(void *hdl, void* cb, const char* strURL)
{
  if (cb == NULL)
    return NULL;

  return ((CB_AddOnLib*)cb)->CURLCreate(((AddonCB*)hdl)->addonData, strURL);
}

DLLEXPORT bool XBMC_curl_add_option(void *hdl, void* cb, void *file, XFILE::CURLOPTIONTYPE type, const char* name, const char *value)
{
  if (cb == NULL)
    return false;

  return ((CB_AddOnLib*)cb)->CURLAddOption(((AddonCB*)hdl)->addonData, file, type, name, value);
}

DLLEXPORT bool XBMC_curl_open(void *hdl, void* cb, void *file, unsigned int flags)
{
  if (cb == NULL)
    return false;

  return ((CB_AddOnLib*)cb)->CURLOpen(((AddonCB*)hdl)->addonData, file, flags);
}

};
