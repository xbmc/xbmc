/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProjectParser.h"

#include "BitReader.h"
#include "BlurayPlaylistHints.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/chrono.h>

namespace XFILE
{
namespace
{
const std::string PROJECT_FILE{"bluray_project.bin"};

// A project is a few tens of kilobytes - anything larger is not one
constexpr int64_t MAX_FILE_SIZE = 4 * 1024 * 1024;

//
// A playlist record, as observed on every disc carrying the file:
//
//   u16    name length
//   char   name[length]          printable ASCII
//   u32    reserved              zero throughout
//   u16    playlist
//   u16    0x0002                marks the record - the field it belongs to is unknown
//   char   presentation[2]       "2D"
//   f32    frame rate            big endian, eg. 23.976
//   f32    duration in seconds   big endian, matches the playlist's own duration
//   u32    play items
//
// The records are not indexed or counted anywhere that has been identified, so they are found by
// scanning for that shape. The reserved field and the marker make a false positive unlikely, and
// a record that does not decode cleanly is skipped rather than ending the parse.
//
constexpr unsigned int OFFSET_RESERVED = 0;
constexpr unsigned int OFFSET_PLAYLIST = 4;
constexpr unsigned int OFFSET_MARKER = 6;
constexpr unsigned int OFFSET_PRESENTATION = 8;
constexpr unsigned int OFFSET_FRAME_RATE = 10;
constexpr unsigned int OFFSET_DURATION = 14;
constexpr unsigned int OFFSET_PLAY_ITEMS = 18;
constexpr unsigned int RECORD_TAIL_SIZE = 22; // from the end of the name to the end of the record

constexpr uint16_t RECORD_MARKER = 0x0002;
constexpr unsigned int PRESENTATION_LENGTH = 2;
constexpr unsigned int MIN_NAME_LENGTH = 3;
constexpr unsigned int MAX_NAME_LENGTH = 64;

// Beyond these a record is not describing a playlist of a Blu-ray disc
constexpr float MIN_FRAME_RATE = 20.0f;
constexpr float MAX_FRAME_RATE = 60.0f;
constexpr float MAX_DURATION_SECONDS = 24.0f * 60.0f * 60.0f;

// A record's duration agrees with the playlist it names to within rounding. One that does not is
// describing something else, or the layout has changed under it.
constexpr std::chrono::milliseconds MAX_DURATION_DIFFERENCE{2s};

bool IsPrintable(const std::span<const std::byte> text)
{
  return std::ranges::all_of(text,
                             [](std::byte c)
                             {
                               const auto character = std::to_integer<unsigned char>(c);
                               return character >= 0x20 && character < 0x7F;
                             });
}

float ReadFloat(const std::span<const std::byte> buffer, unsigned int offset)
{
  return std::bit_cast<float>(GetDWord(buffer, offset));
}

/*!
 \brief Decode the record whose name ends at offset, or nullopt if there is not one there.
 */
std::optional<ProjectPlaylistInformation> ReadRecord(const std::span<const std::byte> buffer,
                                                     unsigned int offset,
                                                     std::string&& name,
                                                     const DiscPlaylistDurations& discPlaylists)
{
  if (GetDWord(buffer, offset + OFFSET_RESERVED) != 0 ||
      GetWord(buffer, offset + OFFSET_MARKER) != RECORD_MARKER ||
      !IsPrintable(buffer.subspan(offset + OFFSET_PRESENTATION, PRESENTATION_LENGTH)))
    return std::nullopt;

  ProjectPlaylistInformation information;
  information.playlist = GetWord(buffer, offset + OFFSET_PLAYLIST);
  const auto discPlaylist{discPlaylists.find(information.playlist)};
  if (discPlaylist == discPlaylists.end())
    return std::nullopt;

  information.frameRate = ReadFloat(buffer, offset + OFFSET_FRAME_RATE);
  if (!(information.frameRate >= MIN_FRAME_RATE && information.frameRate <= MAX_FRAME_RATE))
    return std::nullopt; // written so that a NaN fails too

  const float seconds{ReadFloat(buffer, offset + OFFSET_DURATION)};
  if (!(seconds >= 0.0f && seconds <= MAX_DURATION_SECONDS))
    return std::nullopt;

  information.duration = std::chrono::milliseconds{std::lround(seconds * 1000.0f)};
  if (std::chrono::abs(information.duration - discPlaylist->second) > MAX_DURATION_DIFFERENCE)
  {
    CLog::LogF(LOGDEBUG, "Ignoring {} for playlist {} - {} does not match the playlist's {}", name,
               information.playlist, fmt::format("{:%H:%M:%S}", information.duration),
               fmt::format("{:%H:%M:%S}", discPlaylist->second));
    return std::nullopt;
  }

  information.name = std::move(name);
  information.presentation = GetString(buffer, offset + OFFSET_PRESENTATION, PRESENTATION_LENGTH);
  information.playItems = GetDWord(buffer, offset + OFFSET_PLAY_ITEMS);
  return information;
}

bool ReadRecords(const std::span<const std::byte> buffer,
                 const DiscPlaylistDurations& discPlaylists,
                 ProjectInformation& projectInformation)
{
  unsigned int offset{0};
  while (offset + 2 + MIN_NAME_LENGTH + RECORD_TAIL_SIZE <= buffer.size())
  {
    const uint16_t length{GetWord(buffer, offset)};
    const unsigned int name{offset + 2};

    if (length < MIN_NAME_LENGTH || length > MAX_NAME_LENGTH ||
        name + length + RECORD_TAIL_SIZE > buffer.size() ||
        !IsPrintable(buffer.subspan(name, length)))
    {
      ++offset;
      continue;
    }

    const std::optional<ProjectPlaylistInformation> record{
        ReadRecord(buffer, name + length, GetString(buffer, name, length), discPlaylists)};
    if (!record)
    {
      ++offset;
      continue;
    }

    // A playlist is named once. Should a disc name one twice, the first name wins rather than
    // the parse being abandoned - the rest of the file is still worth having.
    const auto [it, inserted] = projectInformation.playlists.try_emplace(record->playlist, *record);
    if (!inserted)
      CLog::LogF(LOGDEBUG, "Playlist {} is named both {} and {} - keeping the first",
                 record->playlist, it->second.name, record->name);

    // Past the whole record. Its tail has been validated as belonging to it, so it cannot also be
    // the start of the next one.
    offset = name + length + RECORD_TAIL_SIZE;
  }

  return !projectInformation.playlists.empty();
}

/*!
 \brief Find the project file, which sits in whichever directory the BD-J application uses.
 \return the file, empty when the disc carries none, or nullopt when the disc could not be read

 BDMV is listed first to tell those last two apart. It is on every disc, so a disc that cannot
 produce it is one that cannot be read rather than one that carries no project.
 */
std::optional<std::string> FindProjectFile(const CURL& url)
{
  const std::string bdmvPath{URIUtils::AddFileToFolder(url.GetHostName(), "BDMV")};

  CFileItemList bdmv;
  if (!CDirectory::GetDirectory(bdmvPath, bdmv, "", DIR_FLAG_NO_FILE_DIRS))
  {
    CLog::LogF(LOGDEBUG, "Could not read BDMV of {}", CURL::GetRedacted(url.Get()));
    return std::nullopt;
  }

  const auto isJar{[](const std::shared_ptr<CFileItem>& item)
                   {
                     if (!item->IsFolder())
                       return false;
                     std::string path{item->GetPath()};
                     URIUtils::RemoveSlashAtEnd(path);
                     return StringUtils::EqualsNoCase(URIUtils::GetFileName(path), "JAR");
                   }};
  if (std::ranges::none_of(bdmv, isJar))
    return std::string{}; // no BD-J application, so no project to carry

  const std::string jarPath{URIUtils::AddFileToFolder(bdmvPath, "JAR")};
  CFileItemList items;
  if (!CDirectory::GetDirectory(jarPath, items, "", DIR_FLAG_NO_FILE_DIRS))
  {
    CLog::LogF(LOGDEBUG, "Could not read BDMV/JAR of {}", CURL::GetRedacted(url.Get()));
    return std::nullopt;
  }

  for (const auto& item : items)
  {
    if (!item->IsFolder())
      continue;

    // Listed rather than asked for by name, as CFile::Exists says the same about a file that is
    // not there and one that could not be reached
    CFileItemList application;
    if (!CDirectory::GetDirectory(item->GetPath(), application, "", DIR_FLAG_NO_FILE_DIRS))
    {
      CLog::LogF(LOGDEBUG, "Could not read {}", CURL::GetRedacted(item->GetPath()));
      return std::nullopt;
    }

    for (const auto& file : application)
    {
      if (!file->IsFolder() &&
          StringUtils::EqualsNoCase(URIUtils::GetFileName(file->GetPath()), PROJECT_FILE))
        return file->GetPath();
    }
  }

  return std::string{};
}

ProjectReadResult ReadProject(const CURL& url,
                              const DiscPlaylistDurations& discPlaylists,
                              ProjectInformation& projectInformation)
{
  const std::optional<std::string> projectFile{FindProjectFile(url)};
  if (!projectFile)
    return ProjectReadResult::FAILED;
  if (projectFile->empty())
    return ProjectReadResult::ABSENT; // most discs do not carry one

  CFile file;
  if (!file.Open(*projectFile))
    return ProjectReadResult::FAILED;

  // Of a size no project has, the file would be the same the next time it was looked at
  const int64_t size{file.GetLength()};
  if (size < 0)
    return ProjectReadResult::FAILED;
  if (size == 0 || size > MAX_FILE_SIZE)
  {
    CLog::LogF(LOGDEBUG, "Ignoring {} of size {}", PROJECT_FILE, size);
    return ProjectReadResult::ABSENT;
  }

  std::vector<std::byte> buffer(static_cast<size_t>(size));
  size_t total{0};
  while (total < buffer.size())
  {
    const ssize_t read{file.Read(buffer.data() + total, buffer.size() - total)};
    if (read <= 0)
      break;
    total += static_cast<size_t>(read);
  }

  if (total != buffer.size())
  {
    CLog::LogF(LOGDEBUG, "Could not read {}", *projectFile);
    return ProjectReadResult::FAILED;
  }

  return CProjectParser::ParseProject(buffer, discPlaylists, projectInformation);
}

} // namespace

ProjectReadResult CProjectParser::ParseProject(const std::span<const std::byte> buffer,
                                               const DiscPlaylistDurations& discPlaylists,
                                               ProjectInformation& projectInformation)
{
  try
  {
    // A file that names nothing recognisable has still been read, and reading it again would say
    // the same, so that counts as the disc having no project rather than as a failure
    return ReadRecords(buffer, discPlaylists, projectInformation) ? ProjectReadResult::READ
                                                                  : ProjectReadResult::ABSENT;
  }
  catch (const std::exception& e)
  {
    CLog::LogF(LOGERROR, "Authoring project parsing failed - error {}", e.what());
    return ProjectReadResult::FAILED;
  }
}

void CProjectParser::LogProject(const ProjectInformation& projectInformation)
{
  if (!projectInformation.present)
    return; // most discs leave no project behind, and there is nothing to say about that

  CLog::LogF(LOGDEBUG, "Disc carries an authoring project naming {} playlist(s)",
             projectInformation.playlists.size());

  for (const auto& [playlist, information] : projectInformation.playlists)
  {
    const PlaylistRole role{GetProjectPlaylistRole(information.name)};
    const std::string kind{role == PlaylistRole::UNKNOWN
                               ? std::string{}
                               : StringUtils::Format(" - {}", GetPlaylistRoleName(role))};

    CLog::LogF(LOGDEBUG, " Playlist {} is {}, {} {} of {}, {} play item(s){}", playlist,
               information.name, information.presentation, information.frameRate,
               fmt::format("{:%H:%M:%S}", information.duration), information.playItems, kind);
  }
}

ProjectReadResult CProjectParser::GetProject(const CURL& url,
                                             const DiscPlaylistDurations& discPlaylists,
                                             ProjectInformation& projectInformation)
{
  projectInformation = {};

  // Without the disc's playlists every record would be rejected as naming one it does not have,
  // which is not an answer about the project
  if (discPlaylists.empty())
    return ProjectReadResult::FAILED;

  // Independent of the movie objects - a disc that left its authoring project behind names every
  // playlist, whether its navigation is HDMV or BD-J
  const ProjectReadResult result{ReadProject(url, discPlaylists, projectInformation)};

  // Nothing is kept from a project that was not read in full
  if (result != ProjectReadResult::READ)
    projectInformation = {};
  projectInformation.present = result == ProjectReadResult::READ;
  return result;
}
} // namespace XFILE
