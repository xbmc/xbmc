/*
 *      Copyright (C) 2012-2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "SavestateUtils.h"
#include "Savestate.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "profiles/ProfilesManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>

#define SAVESTATE_EXTENSION      ".sav"
#define SAVESTATE_AUTO_PREFIX    "auto_"
#define SAVESTATE_SLOT_PREFIX    "slot%d_"
#define SAVESTATE_MANUAL_PREFIX  "save_"

using namespace KODI;
using namespace GAME;

namespace
{
  void make_safe_path(std::string& path)
  {
    // Replace unsafe characters with "_"
    std::transform(path.begin(), path.end(), path.begin(),
      [](char c)
      {
        if (std::isalnum(c))
          return c;

        switch (c)
        {
        case '-':
        case '.':
        case '_':
        case '[':
        case ']':
        case '(':
        case ')':
        case '!':
          return c;
        default:
          break;
        }

        return '_';
      });

    // Combine successive runs of underscores
    path.erase(std::unique(path.begin(), path.end(),
      [](char a, char b)
      {
        return a == '_' && b == '_';
      }), path.end());

    // Limit folderName to a sane number of characters
    if (path.length() > 40)
      path.erase(path.begin() + 40, path.end());

    // Trim trailing underscores
    StringUtils::TrimRight(path, "_");
  }
}

std::string CSavestateUtils::MakePath(const CSavestate& save)
{
  using namespace XFILE;

  if (save.GameClient().empty())
    return "";

  // Build path
  std::string savePath = CProfilesManager::GetInstance().GetSavestatesFolder();

  // Append game client
  savePath = URIUtils::AddFileToFolder(savePath, save.GameClient());

  // Generate a folder name based on game path and CRC
  std::string folderName = URIUtils::GetFileName(save.GamePath()) + "_" + save.GameCRC();

  make_safe_path(folderName);

  savePath = URIUtils::AddFileToFolder(savePath, folderName);

  if (!CDirectory::Exists(savePath))
    CDirectory::Create(savePath);

  // Generate a filename based on type
  std::string filename;
  switch (save.Type())
  {
  case SAVETYPE::AUTO:
    filename = SAVESTATE_AUTO_PREFIX + save.Timestamp().GetAsDBDateTime();
    break;
  case SAVETYPE::SLOT:
    filename = StringUtils::Format(SAVESTATE_SLOT_PREFIX, save.Slot()) + save.Timestamp().GetAsDBDateTime();
    break;
  case SAVETYPE::MANUAL:
    filename = SAVESTATE_MANUAL_PREFIX + save.Timestamp().GetAsDBDateTime();
    break;
  default:
    return ""; // Invalid savestate, bail out
  }

  make_safe_path(filename);

  filename += SAVESTATE_EXTENSION;

  savePath = URIUtils::AddFileToFolder(savePath, filename);

  for (unsigned int i = 1; i <= 9; i++)
  {
    if (!CFile::Exists(savePath))
      break;

    if (i != 1)
      savePath.erase(savePath.end() - 1, savePath.end());

    savePath += StringUtils::Format("%u", i);
  }

  return savePath;
}

std::string CSavestateUtils::MakeThumbPath(const std::string& savePath)
{
  return URIUtils::ReplaceExtension(savePath, ".jpg");
}
