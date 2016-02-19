#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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

#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_GUI_DialogFileBrowser
  {
    bool Dialogs_FileBrowser_ShowAndGetDirectory(
          const std::string&      shares,
          const std::string&      heading,
          std::string&            path,
          bool                    bWriteOnly);
    bool Dialogs_FileBrowser_ShowAndGetFile(
          const std::string&      shares,
          const std::string&      mask,
          const std::string&      heading,
          std::string&            file,
          bool                    useThumbs,
          bool                    useFileDirectories);
    bool Dialogs_FileBrowser_ShowAndGetFileFromDir(
          const std::string&      directory,
          const std::string&      mask,
          const std::string&      heading,
          std::string&            path,
          bool                    useThumbs,
          bool                    useFileDirectories,
          bool                    singleList);
    bool Dialogs_FileBrowser_ShowAndGetFileList(
          const std::string&      shares,
          const std::string&      mask,
          const std::string&      heading,
          std::vector<std::string> &path,
          bool                    useThumbs,
          bool                    useFileDirectories);
    bool Dialogs_FileBrowser_ShowAndGetSource(
          std::string&            path,
          bool                    allowNetworkShares,
          const std::string&      additionalShare,
          const std::string&      strType);
    bool Dialogs_FileBrowser_ShowAndGetImage(
          const std::string&      shares,
          const std::string&      heading,
          std::string&            path);
    bool Dialogs_FileBrowser_ShowAndGetImageList(
          const std::string&      shares,
          const std::string&      heading,
          std::vector<std::string> &path);
  };

}; /* extern "C" */
