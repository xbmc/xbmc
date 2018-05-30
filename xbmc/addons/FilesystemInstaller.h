/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
