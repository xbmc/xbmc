/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscM3U.h"

#include "URL.h"
#include "filesystem/File.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

namespace
{
constexpr int64_t MAX_M3U_SIZE = 10 * 1024 * 1024; // 10 MiB
}

bool CGameClientDiscM3U::Load(const std::string& m3uPath, CGameClientDiscModel& model)
{
  model.Clear();

  if (m3uPath.empty())
    return true;

  if (!CFileUtils::Exists(m3uPath))
  {
    CLog::Log(LOGDEBUG, "Playlist M3U {} does not exist, proceeding with empty disc model",
              CURL::GetRedacted(m3uPath));
    return true;
  }

  CLog::Log(LOGDEBUG, "Loading playlist M3U {}", CURL::GetRedacted(m3uPath));

  std::string m3u;
  {
    XFILE::CFile file;
    if (!file.Open(m3uPath))
    {
      CLog::Log(LOGERROR, "Failed to open playlist M3U {}", CURL::GetRedacted(m3uPath));
      return false;
    }

    const int64_t size = file.GetLength();
    if (size < 0)
    {
      CLog::Log(LOGERROR, "Failed to get size of playlist M3U {}", CURL::GetRedacted(m3uPath));
      file.Close();
      return false;
    }

    if (size > MAX_M3U_SIZE)
    {
      CLog::Log(LOGERROR, "Playlist M3U {} is too large ({} bytes, max {} bytes)",
                CURL::GetRedacted(m3uPath), size, MAX_M3U_SIZE);
      file.Close();
      return false;
    }

    m3u.resize(static_cast<size_t>(size));
    const ssize_t read = file.Read(m3u.data(), m3u.size());
    file.Close();

    if (read != static_cast<ssize_t>(m3u.size()))
    {
      CLog::Log(LOGERROR, "Failed to read playlist M3U {}, only {} of {} bytes read",
                CURL::GetRedacted(m3uPath), read, m3u.size());
      return false;
    }
  }

  const std::string playlistDirectory = URIUtils::GetDirectory(m3uPath);

  std::vector<std::string> lines = StringUtils::Split(m3u, '\n');
  for (std::string& line : lines)
  {
    StringUtils::Trim(line);

    if (line.empty() || StringUtils::StartsWith(line, "#"))
      continue;

    // M3U entries are relative to the playlist path. Normalize for
    // stable restore
    if (!URIUtils::IsURL(line) && !URIUtils::IsDOSPath(line) &&
        !URIUtils::IsAbsolutePOSIXPath(line))
      line = URIUtils::AddFileToFolder(playlistDirectory, line);

    model.AddDisc(line);
  }

  return true;
}
