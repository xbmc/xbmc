/*
 *      Copyright (C) 2005-2017 Team Kodi
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

#include "FileBrowser.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/dialogs/FileBrowser.h"

#include "URL.h"
#include "addons/AddonDll.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "settings/MediaSourceSettings.h"
#include "storage/MediaManager.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIDialogFileBrowser::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogFileBrowser = static_cast<AddonToKodiFuncTable_kodi_gui_dialogFileBrowser*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_dialogFileBrowser)));

  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_directory = show_and_get_directory;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_file = show_and_get_file;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_file_from_dir = show_and_get_file_from_dir;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_file_list = show_and_get_file_list;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_source = show_and_get_source;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_image = show_and_get_image;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_image_list = show_and_get_image_list;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->clear_file_list = clear_file_list;
}

void Interface_GUIDialogFileBrowser::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->dialogFileBrowser);
}

bool Interface_GUIDialogFileBrowser::show_and_get_directory(void* kodiBase, const char* shares, const char* heading,
                                                            const char* path_in, char** path_out, bool write_only)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!shares || !heading || !path_in || !path_out)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid handler data (shares='%p', heading='%p', path_in='%p', path_out='%p') on addon '%s'",
                          __FUNCTION__, shares, heading, path_in, path_out, addon->ID().c_str());
    return false;
  }

  std::string strPath = path_in;

  VECSOURCES vecShares;
  GetVECShares(vecShares, shares, strPath);
  bool bRet = CGUIDialogFileBrowser::ShowAndGetDirectory(vecShares, heading, strPath, write_only);
  if (bRet)
    *path_out = strdup(strPath.c_str());
  return bRet;
}

bool Interface_GUIDialogFileBrowser::show_and_get_file(void* kodiBase, const char* shares, const char* mask, const char* heading,
                                                       const char* path_in, char** path_out, bool use_thumbs, bool use_file_directories)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!shares || !mask || !heading || !path_in || !path_out)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid handler data (shares='%p', mask='%p', heading='%p', path_in='%p', path_out='%p') on addon '%s'",
                          __FUNCTION__, shares, mask, heading, path_in, path_out, addon->ID().c_str());
    return false;
  }

  std::string strPath = path_in;

  VECSOURCES vecShares;
  GetVECShares(vecShares, shares, strPath);
  bool bRet = CGUIDialogFileBrowser::ShowAndGetFile(vecShares, mask, heading, strPath, use_thumbs, use_file_directories);
  if (bRet)
    *path_out = strdup(strPath.c_str());
  return bRet;
}

bool Interface_GUIDialogFileBrowser::show_and_get_file_from_dir(void* kodiBase, const char* directory, const char* mask,
                                                                const char* heading, const char* path_in, char** path_out,
                                                                bool use_thumbs, bool use_file_directories, bool single_list)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!directory || !mask || !heading || !path_in || !path_out)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid handler data (directory='%p', mask='%p', heading='%p', path_in='%p', path_out='%p') on addon '%s'",
                          __FUNCTION__, directory, mask, heading, path_in, path_out, addon->ID().c_str());
    return false;
  }

  std::string strPath = path_in;
  bool bRet = CGUIDialogFileBrowser::ShowAndGetFile(directory, mask, heading, strPath, use_thumbs, use_file_directories, single_list);
  if (bRet)
    *path_out = strdup(strPath.c_str());
  return bRet;
}

bool Interface_GUIDialogFileBrowser::show_and_get_file_list(void* kodiBase, const char* shares, const char* mask,
                                                            const char* heading, char*** file_list, unsigned int* entries,
                                                            bool use_thumbs, bool use_file_directories)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!shares || !mask || !heading || !file_list || !entries)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid handler data (shares='%p', mask='%p', heading='%p', file_list='%p', entries='%p') on addon '%s'",
                          __FUNCTION__, shares, mask, heading, file_list, entries, addon->ID().c_str());
    return false;
  }

  VECSOURCES vecShares;
  GetVECShares(vecShares, shares, "");

  std::vector<std::string> pathsInt;
  bool bRet = CGUIDialogFileBrowser::ShowAndGetFileList(vecShares, mask, heading, pathsInt, use_thumbs, use_file_directories);
  if (bRet)
  {
    *entries = pathsInt.size();
    *file_list = static_cast<char**>(malloc(*entries*sizeof(char*)));
    for (unsigned int i = 0; i < *entries; ++i)
      (*file_list)[i] = strdup(pathsInt[i].c_str());
  }
  else
    *entries = 0;
  return bRet;
}

bool Interface_GUIDialogFileBrowser::show_and_get_source(void* kodiBase, const char* path_in, char** path_out,
                                                         bool allowNetworkShares, const char* additionalShare,
                                                         const char* strType)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!strType || !additionalShare || !path_in || !path_out)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid handler data (additionalShare='%p', strType='%p', path_in='%p', path_out='%p') on addon '%s'",
                          __FUNCTION__, additionalShare, strType, path_in, path_out, addon->ID().c_str());
    return false;
  }

  std::string strPath = path_in;

  VECSOURCES vecShares;
  if (additionalShare)
    GetVECShares(vecShares, additionalShare, strPath);
  bool bRet = CGUIDialogFileBrowser::ShowAndGetSource(strPath, allowNetworkShares, &vecShares, strType);
  if (bRet)
    *path_out = strdup(strPath.c_str());
  return bRet;
}

bool Interface_GUIDialogFileBrowser::show_and_get_image(void* kodiBase, const char* shares, const char* heading,
                                                        const char* path_in, char** path_out)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!shares || !heading)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid handler data (shares='%p', heading='%p') on addon '%s'", __FUNCTION__, shares, heading, addon->ID().c_str());
    return false;
  }

  std::string strPath = path_in;

  VECSOURCES vecShares;
  GetVECShares(vecShares, shares, strPath);
  bool bRet = CGUIDialogFileBrowser::ShowAndGetImage(vecShares, heading, strPath);
  if (bRet)
    *path_out = strdup(strPath.c_str());
  return bRet;
}

bool Interface_GUIDialogFileBrowser::show_and_get_image_list(void* kodiBase, const char* shares, const char* heading, char*** file_list, unsigned int* entries)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!shares || !heading || !file_list || !entries)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid handler data (shares='%p', heading='%p', file_list='%p', entries='%p') on addon '%s'",
                          __FUNCTION__, shares, heading, file_list, entries, addon->ID().c_str());
    return false;
  }

  VECSOURCES vecShares;
  GetVECShares(vecShares, shares, "");

  std::vector<std::string> pathsInt;
  bool bRet = CGUIDialogFileBrowser::ShowAndGetImageList(vecShares, heading, pathsInt);
  if (bRet)
  {
    *entries = pathsInt.size();
    *file_list = static_cast<char**>(malloc(*entries*sizeof(char*)));
    for (unsigned int i = 0; i < *entries; ++i)
      (*file_list)[i] = strdup(pathsInt[i].c_str());
  }
  else
    *entries = 0;
  return bRet;
}

void Interface_GUIDialogFileBrowser::clear_file_list(void* kodiBase, char*** file_list, unsigned int entries)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid data", __FUNCTION__);
    return;
  }

  if (*file_list)
  {
    for (unsigned int i = 0; i < entries; ++i)
      free((*file_list)[i]);
    free(*file_list);
    *file_list = nullptr;
  }
  else
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::%s - invalid handler data (file_list='%p') on addon '%s'", __FUNCTION__, file_list, addon->ID().c_str());
}

void Interface_GUIDialogFileBrowser::GetVECShares(VECSOURCES& vecShares, const std::string& strShares, const std::string& strPath)
{
  std::size_t found;
  found = strShares.find("local");
  if (found!=std::string::npos)
    g_mediaManager.GetLocalDrives(vecShares);
  found = strShares.find("network");
  if (found!=std::string::npos)
    g_mediaManager.GetNetworkLocations(vecShares);
  found = strShares.find("removable");
  if (found!=std::string::npos)
    g_mediaManager.GetRemovableDrives(vecShares);
  found = strShares.find("programs");
  if (found!=std::string::npos)
  {
    VECSOURCES* sources = CMediaSourceSettings::GetInstance().GetSources("programs");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }
  found = strShares.find("files");
  if (found!=std::string::npos)
  {
    VECSOURCES* sources = CMediaSourceSettings::GetInstance().GetSources("files");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }
  found = strShares.find("music");
  if (found!=std::string::npos)
  {
    VECSOURCES* sources = CMediaSourceSettings::GetInstance().GetSources("music");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }
  found = strShares.find("video");
  if (found!=std::string::npos)
  {
    VECSOURCES* sources = CMediaSourceSettings::GetInstance().GetSources("video");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }
  found = strShares.find("pictures");
  if (found!=std::string::npos)
  {
    VECSOURCES* sources = CMediaSourceSettings::GetInstance().GetSources("pictures");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }

  if (vecShares.empty())
  {
    CMediaSource share;
    std::string basePath = strPath;
    std::string tempPath;
    while (URIUtils::GetParentPath(basePath, tempPath))
      basePath = tempPath;
    share.strPath = basePath;
    // don't include the user details in the share name
    CURL url(share.strPath);
    share.strName = url.GetWithoutUserDetails();
    vecShares.push_back(share);
  }
}

} /* namespace ADDON */
} /* extern "C" */
