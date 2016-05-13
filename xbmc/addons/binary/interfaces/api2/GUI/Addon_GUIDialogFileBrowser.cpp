/*
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

#include "Addon_GUIDialogFileBrowser.h"

#include "URL.h"
#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "dialogs/GUIDialogFileBrowser.h"
#include "settings/MediaSourceSettings.h"
#include "storage/MediaManager.h"
#include "utils/URIUtils.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnDialog_FileBrowser::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetDirectory = CAddOnDialog_FileBrowser::ShowAndGetDirectory;
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetFile = CAddOnDialog_FileBrowser::ShowAndGetFile;
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetFileFromDir = CAddOnDialog_FileBrowser::ShowAndGetFileFromDir;
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetFileList = CAddOnDialog_FileBrowser::ShowAndGetFileList;
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetSource = CAddOnDialog_FileBrowser::ShowAndGetSource;
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetImage = CAddOnDialog_FileBrowser::ShowAndGetImage;
  interfaces->GUI.Dialogs.FileBrowser.ShowAndGetImageList = CAddOnDialog_FileBrowser::ShowAndGetImageList;
  interfaces->GUI.Dialogs.FileBrowser.ClearList = CAddOnDialog_FileBrowser::ClearList;
}

bool CAddOnDialog_FileBrowser::ShowAndGetDirectory(const char *shares, const char *heading, char &path, unsigned int &iMaxStringSize, bool bWriteOnly)
{
  try
  {
    std::string strPath = &path;

    VECSOURCES vecShares;
    GetVECShares(vecShares, shares, strPath);
    bool bRet = CGUIDialogFileBrowser::ShowAndGetDirectory(vecShares, heading, strPath, bWriteOnly);
    if (bRet)
      strncpy(&path, strPath.c_str(), iMaxStringSize);
    iMaxStringSize = strPath.length();
    return bRet;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDialog_FileBrowser::ShowAndGetFile(const char *shares, const char *mask, const char *heading, char &path, unsigned int &iMaxStringSize, bool useThumbs, bool useFileDirectories)
{
  try
  {
    std::string strPath = &path;

    VECSOURCES vecShares;
    GetVECShares(vecShares, shares, strPath);
    bool bRet = CGUIDialogFileBrowser::ShowAndGetFile(vecShares, mask, heading, strPath, useThumbs, useFileDirectories);
    if (bRet)
      strncpy(&path, strPath.c_str(), iMaxStringSize);
    iMaxStringSize = strPath.length();
    return bRet;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDialog_FileBrowser::ShowAndGetFileFromDir(const char *directory, const char *mask, const char *heading, char &path, unsigned int &iMaxStringSize, bool useThumbs, bool useFileDirectories, bool singleList)
{
  try
  {
    std::string strPath = &path;
    bool bRet = CGUIDialogFileBrowser::ShowAndGetFile(directory, mask, heading, strPath, useThumbs, useFileDirectories, singleList);
    if (bRet)
      strncpy(&path, strPath.c_str(), iMaxStringSize);
    iMaxStringSize = strPath.length();
    return bRet;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDialog_FileBrowser::ShowAndGetFileList(const char* shares, const char* mask, const char* heading, char**& path, unsigned int& entries, bool useThumbs, bool useFileDirectories)
{
  try
  {
    VECSOURCES vecShares;
    GetVECShares(vecShares, shares, "");

    std::vector<std::string> pathsInt;
    bool bRet = CGUIDialogFileBrowser::ShowAndGetFileList(vecShares, mask, heading, pathsInt, useThumbs, useFileDirectories);
    if (bRet)
    {
      entries = pathsInt.size();
      path = (char**)malloc(entries*sizeof(char*));
      for (unsigned int i = 0; i < entries; ++i)
        path[i] = strdup(pathsInt[i].c_str());
    }
    else
      entries = 0;
    return bRet;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDialog_FileBrowser::ShowAndGetSource(char &path, unsigned int &iMaxStringSize, bool allowNetworkShares, const char *additionalShare, const char *strType)
{
  try
  {
    std::string strPath = &path;

    VECSOURCES vecShares;
    if (additionalShare)
      GetVECShares(vecShares, additionalShare, strPath);
    bool bRet = CGUIDialogFileBrowser::ShowAndGetSource(strPath, allowNetworkShares, &vecShares, strType);
    if (bRet)
      strncpy(&path, strPath.c_str(), iMaxStringSize);
    iMaxStringSize = strPath.length();
    return bRet;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDialog_FileBrowser::ShowAndGetImage(const char *shares, const char *heading, char &path, unsigned int &iMaxStringSize)
{
  try
  {
    std::string strPath = &path;

    VECSOURCES vecShares;
    GetVECShares(vecShares, shares, strPath);
    bool bRet = CGUIDialogFileBrowser::ShowAndGetImage(vecShares, heading, strPath);
    if (bRet)
      strncpy(&path, strPath.c_str(), iMaxStringSize);
    iMaxStringSize = strPath.length();
    return bRet;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDialog_FileBrowser::ShowAndGetImageList(const char* shares, const char* heading, char**& path, unsigned int& entries)
{
  try
  {
    VECSOURCES vecShares;
    GetVECShares(vecShares, shares, "");

    std::vector<std::string> pathsInt;
    bool bRet = CGUIDialogFileBrowser::ShowAndGetImageList(vecShares, heading, pathsInt);
    if (bRet)
    {
      entries = pathsInt.size();
      path = (char**)malloc(entries*sizeof(char*));
      for (unsigned int i = 0; i < entries; ++i)
        path[i] = strdup(pathsInt[i].c_str());
    }
    else
      entries = 0;
    return bRet;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnDialog_FileBrowser::ClearList(char**& path, unsigned int entries)
{
  try
  {
    if (path)
    {
      for (unsigned int i = 0; i < entries; ++i)
      {
        if (path[i])
          free(path[i]);
      }
      free(path);
    }
    else
      throw ADDON::WrongValueException("CAddOnDialog_FileBrowser - %s - invalid add-on data for path", __FUNCTION__);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnDialog_FileBrowser::GetVECShares(VECSOURCES &vecShares, std::string strShares, std::string strPath)
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
    VECSOURCES *sources = CMediaSourceSettings::GetInstance().GetSources("programs");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }
  found = strShares.find("files");
  if (found!=std::string::npos)
  {
    VECSOURCES *sources = CMediaSourceSettings::GetInstance().GetSources("files");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }
  found = strShares.find("music");
  if (found!=std::string::npos)
  {
    VECSOURCES *sources = CMediaSourceSettings::GetInstance().GetSources("music");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }
  found = strShares.find("video");
  if (found!=std::string::npos)
  {
    VECSOURCES *sources = CMediaSourceSettings::GetInstance().GetSources("video");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }
  found = strShares.find("pictures");
  if (found!=std::string::npos)
  {
    VECSOURCES *sources = CMediaSourceSettings::GetInstance().GetSources("pictures");
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

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
