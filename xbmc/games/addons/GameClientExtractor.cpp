/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientExtractor.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "games/GameUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstdint>
#include <vector>

using namespace KODI;
using namespace GAME;

namespace
{
/*!
 * \brief The folder an archive's games are copied to, for a client that reads
 * only local files
 *
 * It sits beside the folders that hold each game's saves and is named after
 * the archive the same way. The name has to stay the same between versions:
 * copies are kept and never overwritten, because a disk-based game saves onto
 * its own disks.
 */
std::string ExtractedFolder(const std::string& archivedPath)
{
  return CGameUtils::GetGameFolder(CURL(archivedPath).GetHostName()) + "_extracted/";
}

//! Whether a name read from an archive stays inside the folder it is copied to
bool IsSafeName(const std::string& name)
{
  return !name.empty() && name != "." && name != ".." &&
         name.find_first_of("/\\") == std::string::npos;
}

//! Whether a file names others that are played with it, as a cue sheet names its tracks
bool NamesOtherFiles(const std::string& path)
{
  return URIUtils::HasExtension(path, ".cue|.ccd|.gdi|.m3u|.mds|.toc");
}

//! A name to write a file under until it is complete, which no file in an
//! archive can share. Kept short, as the file's own name can already be
//! close to the filesystem's limit.
std::string PartialName(const std::string& target)
{
  return URIUtils::AddFileToFolder(URIUtils::GetDirectory(target),
                                   StringUtils::CreateUUID() + ".partial");
}

// Copied under a temporary name and then renamed, so a copy that was
// interrupted is never taken for a complete one on a later start
bool CopyWhole(const std::string& from, const std::string& to)
{
  const std::string partial = PartialName(to);
  if (XFILE::CFile::Copy(from, partial) && XFILE::CFile::Rename(partial, to))
    return true;

  XFILE::CFile::Delete(partial);
  return false;
}
} // namespace

CGameClientExtractor::CGameClientExtractor(bool supportsFolders, bool supportsPlaylists)
  : m_supportsFolders(supportsFolders),
    m_supportsPlaylists(supportsPlaylists)
{
}

std::string CGameClientExtractor::Extract(const std::string& archivedPath) const
{
  return m_supportsFolders ? ExtractArchive(archivedPath) : ExtractGame(archivedPath);
}

std::string CGameClientExtractor::ExtractGame(const std::string& archivedPath) const
{
  std::string folder = ExtractedFolder(archivedPath);

  if (!XFILE::CDirectory::Exists(folder) && !XFILE::CDirectory::Create(folder))
    return "";

  // Folders in an archive can hold files of the same name, so the copy keeps
  // the folder the game is in
  const std::string inside = URIUtils::GetDirectory(CURL(archivedPath).GetFileName());
  for (const std::string& part : StringUtils::Split(inside, '/'))
  {
    if (part.empty())
      continue;
    if (!IsSafeName(part))
      return "";
    folder = URIUtils::AddFileToFolder(folder, part) + "/";
    if (!XFILE::CDirectory::Exists(folder) && !XFILE::CDirectory::Create(folder))
      return "";
  }

  // The files a sheet or playlist names come with it, in the folders they are in
  if (NamesOtherFiles(archivedPath) && !CopyTree(URIUtils::GetDirectory(archivedPath), folder))
  {
    CLog::Log(LOGERROR, "GameClientExtractor: Failed to extract {}",
              CURL::GetRedacted(archivedPath));
    return "";
  }

  std::vector<std::string> disks{archivedPath};
  const std::string extension = URIUtils::GetExtension(archivedPath);
  CFileItemList siblings;
  // A playlist already lists the other disks
  if (!URIUtils::HasExtension(archivedPath, ".m3u") &&
      XFILE::CDirectory::GetDirectory(URIUtils::GetDirectory(archivedPath), siblings, extension,
                                      XFILE::DIR_FLAG_DEFAULTS))
  {
    for (const auto& sibling : siblings)
    {
      // Listed paths need not be spelled as the one asked for, so compare names
      if (!sibling->IsFolder() &&
          URIUtils::GetFileName(sibling->GetPath()) != URIUtils::GetFileName(archivedPath))
        disks.push_back(sibling->GetPath());
    }
  }
  std::sort(disks.begin() + 1, disks.end());

  std::vector<std::string> names;
  for (const std::string& disk : disks)
  {
    const std::string name = URIUtils::GetFileName(disk);
    if (!IsSafeName(name))
      return "";
    const std::string target = URIUtils::AddFileToFolder(folder, name);

    if (!XFILE::CFile::Exists(target) && !CopyWhole(disk, target))
    {
      CLog::Log(LOGERROR, "GameClientExtractor: Failed to extract {}", CURL::GetRedacted(disk));
      return "";
    }
    names.push_back(name);
  }

  std::string game = URIUtils::AddFileToFolder(folder, names.front());

  // A playlist lets the emulator swap to the other disks
  if (names.size() > 1 && m_supportsPlaylists)
  {
    // Named so it can't replace a playlist that came out of the archive
    std::string playlist = game;
    URIUtils::RemoveExtension(playlist);
    playlist += ".kodi.m3u";

    // A playlist is replaced only through a complete copy written aside, so a
    // failed write never leaves a partial one. Rename can't replace a file on
    // every platform, so an outdated playlist is removed first.
    const std::string content = StringUtils::Join(names, "\n") + "\n";
    std::vector<uint8_t> current;
    if (XFILE::CFile().LoadFile(playlist, current) < 0 ||
        std::string(current.begin(), current.end()) != content)
    {
      const std::string partial = PartialName(playlist);
      XFILE::CFile file;
      const bool written =
          file.OpenForWrite(partial, true) &&
          file.Write(content.data(), content.size()) == static_cast<ssize_t>(content.size());
      file.Close();
      if (!written || (XFILE::CFile::Exists(playlist) && !XFILE::CFile::Delete(playlist)) ||
          !XFILE::CFile::Rename(partial, playlist))
        XFILE::CFile::Delete(partial);
    }

    if (XFILE::CFile::Exists(playlist))
      game = playlist;
  }

  CLog::Log(LOGDEBUG, "GameClientExtractor: Playing {} from its extracted copy",
            CURL::GetRedacted(game));

  return CSpecialProtocol::TranslatePath(game);
}

std::string CGameClientExtractor::ExtractArchive(const std::string& archivedPath)
{
  CURL root(archivedPath);
  const std::string inside = root.GetFileName();
  root.SetFileName("");

  if (!std::ranges::all_of(StringUtils::Split(inside, '/'), IsSafeName))
    return "";

  const std::string folder = ExtractedFolder(archivedPath);

  // Every start fills in what an unpacking cut short left out
  if ((!XFILE::CDirectory::Exists(folder) && !XFILE::CDirectory::Create(folder)) ||
      !CopyTree(root.Get(), folder))
  {
    CLog::Log(LOGERROR, "GameClientExtractor: Failed to extract {}", CURL::GetRedacted(root.Get()));
    return "";
  }

  const std::string game = URIUtils::AddFileToFolder(folder, inside);
  CLog::Log(LOGDEBUG, "GameClientExtractor: Playing {} from its extracted folder",
            CURL::GetRedacted(game));

  return CSpecialProtocol::TranslatePath(game);
}

bool CGameClientExtractor::CopyTree(const std::string& from, const std::string& to)
{
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(from, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS))
    return false;

  for (const auto& item : items)
  {
    if (!IsSafeName(item->GetLabel()))
    {
      CLog::Log(LOGERROR, "GameClientExtractor: Refusing archive entry {}",
                CURL::GetRedacted(item->GetPath()));
      return false;
    }
    const std::string target = URIUtils::AddFileToFolder(to, item->GetLabel());
    if (item->IsFolder())
    {
      if ((!XFILE::CDirectory::Exists(target) && !XFILE::CDirectory::Create(target)) ||
          !CopyTree(item->GetPath(), URIUtils::AddFileToFolder(target, "")))
        return false;
    }
    else if (!XFILE::CFile::Exists(target) && !CopyWhole(item->GetPath(), target))
      return false;
  }

  return true;
}
