/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidStorageProvider.h"

#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include "platform/android/activity/XBMCApp.h"

#include <androidjni/Context.h>
#include <androidjni/Environment.h>
#include <androidjni/File.h>
#include <androidjni/StatFs.h>
#include <androidjni/StorageManager.h>
#include <androidjni/StorageVolume.h>

std::unique_ptr<IStorageProvider> IStorageProvider::CreateInstance()
{
  return std::make_unique<CAndroidStorageProvider>();
}

CAndroidStorageProvider::CAndroidStorageProvider()
{
  PumpDriveChangeEvents(NULL);
}

void CAndroidStorageProvider::GetLocalDrives(std::vector<CMediaSource>& localDrives)
{
  CMediaSource share;

  // external directory
  std::string path;
  if (GetExternalStorage(path) && !path.empty() && XFILE::CDirectory::Exists(path))
  {
    share.strPath = path;
    share.strName = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21456);
    share.m_ignore = true;
    localDrives.push_back(share);
  }

  // root directory
  share.strPath = "/";
  share.strName = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21453);
  localDrives.push_back(share);
}

void CAndroidStorageProvider::GetRemovableDrives(std::vector<CMediaSource>& removableDrives)
{
  bool inError = false;

  CJNIStorageManager manager(CJNIContext::getSystemService(CJNIContext::STORAGE_SERVICE));
  if (xbmc_jnienv()->ExceptionCheck())
  {
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    inError = true;
  }

  if (!inError)
  {
    CJNIStorageVolumes vols = manager.getStorageVolumes();
    if (xbmc_jnienv()->ExceptionCheck())
    {
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
      inError = true;
    }

    if (!inError)
    {
      std::vector<CMediaSource> droidDrives;

      for (int i = 0; i < vols.size(); ++i)
      {
        CJNIStorageVolume vol = vols.get(i);
        // CLog::Log(LOGDEBUG, "-- Volume: {}({}) -- {}", vol.getDirectory().getAbsolutePath(), vol.getUserLabel(), vol.getState());

        bool removable = vol.isRemovable();
        if (xbmc_jnienv()->ExceptionCheck())
        {
          xbmc_jnienv()->ExceptionDescribe();
          xbmc_jnienv()->ExceptionClear();
          inError = true;
          break;
        }

        std::string state = vol.getState();
        if (xbmc_jnienv()->ExceptionCheck())
        {
          xbmc_jnienv()->ExceptionDescribe();
          xbmc_jnienv()->ExceptionClear();
          inError = true;
          break;
        }

        if (removable && state == CJNIEnvironment::MEDIA_MOUNTED)
        {
          CMediaSource share;

          if (CJNIBase::GetSDKVersion() >= 30)
            share.strPath = vol.getDirectory().getAbsolutePath();
          else
            share.strPath = vol.getPath();
          if (xbmc_jnienv()->ExceptionCheck())
          {
            xbmc_jnienv()->ExceptionDescribe();
            xbmc_jnienv()->ExceptionClear();
            inError = true;
            break;
          }

          share.strName = vol.getUserLabel();
          if (xbmc_jnienv()->ExceptionCheck())
          {
            xbmc_jnienv()->ExceptionDescribe();
            xbmc_jnienv()->ExceptionClear();
            inError = true;
            break;
          }

          StringUtils::Trim(share.strName);
          if (share.strName.empty() || share.strName == "?" ||
              StringUtils::EqualsNoCase(share.strName, "null"))
            share.strName = URIUtils::GetFileName(share.strPath);

          share.m_ignore = true;
          droidDrives.emplace_back(share);
        }
      }

      if (!inError)
      {
        removableDrives.insert(removableDrives.end(), droidDrives.begin(), droidDrives.end());
        return;
      }
    }
  }
}

std::vector<std::string> CAndroidStorageProvider::GetDiskUsage()
{
  std::vector<std::string> result;

  std::string usage;
  // add header
  GetStorageUsage("", usage);
  result.push_back(usage);

  usage.clear();
  // add rootfs
  if (GetStorageUsage("/", usage) && !usage.empty())
    result.push_back(usage);

  usage.clear();
  // add external storage if available
  std::string path;
  if (GetExternalStorage(path) && !path.empty() && GetStorageUsage(path, usage) && !usage.empty())
    result.push_back(usage);

  // add removable storage
  std::vector<CMediaSource> drives;
  GetRemovableDrives(drives);
  for (unsigned int i = 0; i < drives.size(); i++)
  {
    usage.clear();
    if (GetStorageUsage(drives[i].strPath, usage) && !usage.empty())
      result.push_back(usage);
  }

  return result;
}

bool CAndroidStorageProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  std::vector<CMediaSource> drives;
  GetRemovableDrives(drives);
  bool changed = m_removableDrives != drives;
  m_removableDrives = std::move(drives);
  return changed;
}

namespace
{
constexpr float GIGABYTES = 1073741824;
constexpr int PATH_MAXLEN = 38;
} // namespace

bool CAndroidStorageProvider::GetStorageUsage(const std::string& path, std::string& usage)
{
  if (path.empty())
  {
    usage = StringUtils::Format("{:<{}}{:>12}{:>12}{:>12}{:>12}", "Filesystem", PATH_MAXLEN, "Size",
                                "Used", "Avail", "Use %");
    return false;
  }

  CJNIStatFs fileStat(path);
  const int blockSize = fileStat.getBlockSize();
  const int blockCount = fileStat.getBlockCount();
  const int freeBlocks = fileStat.getFreeBlocks();

  if (blockSize <= 0 || blockCount <= 0 || freeBlocks < 0)
    return false;

  const float totalSize = static_cast<float>(blockSize) * blockCount / GIGABYTES;
  const float freeSize = static_cast<float>(blockSize) * freeBlocks / GIGABYTES;
  const float usedSize = totalSize - freeSize;
  const float usedPercentage = usedSize / totalSize * 100;

  usage = StringUtils::Format(
      "{:<{}}{:>11.1f}{}{:>11.1f}{}{:>11.1f}{}{:>11.0f}{}",
      path.size() < PATH_MAXLEN - 1 ? path : StringUtils::Left(path, PATH_MAXLEN - 4) + "...",
      PATH_MAXLEN, totalSize, "G", usedSize, "G", freeSize, "G", usedPercentage, "%");
  return true;
}

bool CAndroidStorageProvider::GetExternalStorage(std::string& path,
                                                 const std::string& type /* = "" */)
{
  std::string sType;
  std::string mountedState;
  bool mounted = false;

  if (CJNIBase::GetSDKVersion() <= 29)
  {
    CJNIFile external = CJNIEnvironment::getExternalStorageDirectory();
    if (external)
      path = external.getAbsolutePath();
  }
  else
  {
    CJNIStorageManager manager =
        (CJNIStorageManager)CXBMCApp::Get().getSystemService(CJNIContext::STORAGE_SERVICE);
    CJNIStorageVolume volume = manager.getPrimaryStorageVolume();
    path = volume.getDirectory().getAbsolutePath();
  }

  if (type == "music")
    sType = CJNIEnvironment::DIRECTORY_MUSIC;
  else if (type == "videos")
    sType = CJNIEnvironment::DIRECTORY_MOVIES;
  else if (type == "pictures")
    sType = CJNIEnvironment::DIRECTORY_PICTURES;
  else if (type == "photos")
    sType = CJNIEnvironment::DIRECTORY_DCIM;
  else if (type == "downloads")
    sType = CJNIEnvironment::DIRECTORY_DOWNLOADS;

  if (!sType.empty())
  {
    path.append("/");
    path.append(sType);
  }

  mountedState = CJNIEnvironment::getExternalStorageState();
  mounted = (mountedState == CJNIEnvironment::MEDIA_MOUNTED ||
             mountedState == CJNIEnvironment::MEDIA_MOUNTED_READ_ONLY);
  return mounted && !path.empty();
}
