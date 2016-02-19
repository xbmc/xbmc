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

#include "InterProcess.h"
#include "kodi/api2/gui/DialogFileBrowser.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
namespace DialogFileBrowser
{

  bool ShowAndGetDirectory(
          const std::string&      shares,
          const std::string&      heading,
          std::string&            path,
          bool                    bWriteOnly)
  {
    return g_interProcess.Dialogs_FileBrowser_ShowAndGetDirectory(shares, heading, path, bWriteOnly);
  }

  bool ShowAndGetFile(
          const std::string&      shares,
          const std::string&      mask,
          const std::string&      heading,
          std::string&            file,
          bool                    useThumbs,
          bool                    useFileDirectories)
  {
    return g_interProcess.Dialogs_FileBrowser_ShowAndGetFile(shares, mask, heading, file, useThumbs, useFileDirectories);
  }

  bool ShowAndGetFileFromDir(
          const std::string&      directory,
          const std::string&      mask,
          const std::string&      heading,
          std::string&            path,
          bool                    useThumbs,
          bool                    useFileDirectories,
          bool                    singleList)
  {
    return g_interProcess.Dialogs_FileBrowser_ShowAndGetFileFromDir(directory, mask, heading, path, useThumbs, useFileDirectories, singleList);
  }

  bool ShowAndGetFileList(
          const std::string&      shares,
          const std::string&      mask,
          const std::string&      heading,
          std::vector<std::string> &path,
          bool                    useThumbs,
          bool                    useFileDirectories)
  {
    return g_interProcess.Dialogs_FileBrowser_ShowAndGetFileList(shares, mask, heading, path, useThumbs, useFileDirectories);
  }

  bool ShowAndGetSource(
          std::string&            path,
          bool                    allowNetworkShares,
          const std::string&      additionalShare,
          const std::string&      strType)
  {
    return g_interProcess.Dialogs_FileBrowser_ShowAndGetSource(path, allowNetworkShares, additionalShare, strType);
  }

  bool ShowAndGetImage(
          const std::string&      shares,
          const std::string&      heading,
          std::string&            path)
  {
    return g_interProcess.Dialogs_FileBrowser_ShowAndGetImage(shares, heading, path);
  }

  bool ShowAndGetImageList(
          const std::string&      shares,
          const std::string&      heading,
          std::vector<std::string> &path)
  {
    return g_interProcess.Dialogs_FileBrowser_ShowAndGetImageList(shares, heading, path);
  }

}; /* namespace DialogFileBrowser */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
