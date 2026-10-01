/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <functional>
#include <string>

/*!
 * Responsible for safely unpacking/copying/removing addon files from/to the addon folder.
 */
class CFilesystemInstaller
{
public:
  using ProgressCallback = std::function<void(unsigned int progress, unsigned int total)>;

  CFilesystemInstaller();

  /*!
   * @param archive Absolute path to zip file to install.
   * @param addonId
   * @param onProgress Called repeatedly while unpacking, may be empty.
   * @return true on success, otherwise false.
   */
  bool InstallToFilesystem(const std::string& archive,
                           const std::string& addonId,
                           const ProgressCallback& onProgress) const;

  bool UnInstallFromFilesystem(const std::string& addonPath) const;

private:
  std::string m_addonFolder;
  std::string m_tempFolder;
};
