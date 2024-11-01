/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "storage/IStorageProvider.h"

#include <set>
#include <string>
#include <vector>

class CAndroidStorageProvider : public IStorageProvider
{
public:
  CAndroidStorageProvider();
  ~CAndroidStorageProvider() override = default;

  void Initialize() override {}
  void Stop() override {}
  bool Eject(const std::string& mountpath) override { return false; }

  void GetLocalDrives(std::vector<CMediaSource>& localDrives) override;
  void GetRemovableDrives(std::vector<CMediaSource>& removableDrives) override;
  std::vector<std::string> GetDiskUsage() override;

  bool PumpDriveChangeEvents(IStorageEventsCallback* callback) override;

  /*!
   * \brief If external storage is available, it returns the path for the external storage (for the specified type)
   * \param path will contain the path of the external storage (for the specified type)
   * \param type optional type. Possible values are "", "files", "music", "videos", "pictures", "photos, "downloads"
   * \return true if external storage is available and a valid path has been stored in the path parameter
   */
  static bool GetExternalStorage(std::string& path, const std::string& type = "");

private:
  std::string unescape(const std::string& str);
  std::vector<CMediaSource> m_removableDrives;
  unsigned int m_removableLength;

  static std::set<std::string> GetRemovableDrives();
  static std::set<std::string> GetRemovableDrivesLinux();

  bool GetStorageUsage(const std::string& path, std::string& usage);
};
