/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

/*!
 * Responsible for safely unpacking/copying/removing addon files from/to the addon folder.
 */
class CFilesystemInstaller
{
public:

  CFilesystemInstaller();

  /*!
   * @param archive Absolute path to zip file to install.
   * @param addonId
   * @return true on success, otherwise false.
   */
  bool InstallToFilesystem(const std::string& archive, const std::string& addonId);

  bool UnInstallFromFilesystem(const std::string& addonPath);

private:
  static bool UnpackArchive(std::string path, const std::string& dest);

  std::string m_addonFolder;
  std::string m_tempFolder;
};
