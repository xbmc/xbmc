/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProjectParser.h"

#include "BitReader.h"
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
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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
constexpr unsigned int MAX_PLAYLIST = 1999; // playlists are 00000.mpls to 01999.mpls
constexpr float MIN_FRAME_RATE = 20.0f;
constexpr float MAX_FRAME_RATE = 60.0f;
constexpr float MAX_DURATION_SECONDS = 24.0f * 60.0f * 60.0f;

float ReadFloat(const std::span<const std::byte> buffer, unsigned int offset)
{
  return std::bit_cast<float>(GetDWord(buffer, offset));
}

/*!
 \brief Decode the record whose name ends at offset, or nullopt if there is not one there.
 */
std::optional<ProjectPlaylistInformation> ReadRecord(const std::span<const std::byte> buffer,
                                                     unsigned int offset,
                                                     std::string&& name)
{
  if (GetDWord(buffer, offset + OFFSET_RESERVED) != 0 ||
      GetWord(buffer, offset + OFFSET_MARKER) != RECORD_MARKER)
    return std::nullopt;

  ProjectPlaylistInformation information;
  information.playlist = GetWord(buffer, offset + OFFSET_PLAYLIST);
  if (information.playlist > MAX_PLAYLIST)
    return std::nullopt;

  information.frameRate = ReadFloat(buffer, offset + OFFSET_FRAME_RATE);
  if (!(information.frameRate >= MIN_FRAME_RATE && information.frameRate <= MAX_FRAME_RATE))
    return std::nullopt; // written so that a NaN fails too

  const float seconds{ReadFloat(buffer, offset + OFFSET_DURATION)};
  if (!(seconds >= 0.0f && seconds <= MAX_DURATION_SECONDS))
    return std::nullopt;

  information.name = std::move(name);
  information.presentation = GetString(buffer, offset + OFFSET_PRESENTATION, PRESENTATION_LENGTH);
  information.duration = std::chrono::milliseconds{std::lround(seconds * 1000.0f)};
  information.playItems = GetDWord(buffer, offset + OFFSET_PLAY_ITEMS);
  return information;
}

bool ParseProject(const std::span<const std::byte> buffer, ProjectInformation& projectInformation)
{
  const std::byte* const data = buffer.data();

  unsigned int offset{0};
  while (offset + 2 + MIN_NAME_LENGTH + RECORD_TAIL_SIZE <= buffer.size())
  {
    const uint16_t length{GetWord(buffer, offset)};
    const unsigned int name{offset + 2};

    if (length < MIN_NAME_LENGTH || length > MAX_NAME_LENGTH ||
        name + length + RECORD_TAIL_SIZE > buffer.size() ||
        !std::all_of(data + name, data + name + length,
                     [](std::byte c)
                     {
                       const auto character = std::to_integer<unsigned char>(c);
                       return character >= 0x20 && character < 0x7F;
                     }))
    {
      ++offset;
      continue;
    }

    const std::optional<ProjectPlaylistInformation> record{
        ReadRecord(buffer, name + length, GetString(buffer, name, length))};
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

/*! \brief Find the project file, which sits in whichever directory the BD-J application uses. */
std::string FindProjectFile(const CURL& url)
{
  const std::string jarPath{URIUtils::AddFileToFolder(url.GetHostName(), "BDMV", "JAR")};

  CFileItemList items;
  if (!CDirectory::GetDirectory(jarPath, items, "", DIR_FLAG_NO_FILE_DIRS))
    return {};

  for (const auto& item : items)
  {
    if (!item->IsFolder())
      continue;

    const std::string file{URIUtils::AddFileToFolder(item->GetPath(), PROJECT_FILE)};
    if (CFile::Exists(file))
      return file;
  }

  return {};
}
}

bool ProjectPlaylistInformation::IsFeature() const
{
  // FPL_MainFeature and the variants that follow it - _EXT, _Narrative, _EXT_Narrative, and one
  // per language. The prefix is not always FPL_, as "SEG FPL_MainFeature" also occurs, so this
  // looks for the name rather than testing the start of it.
  if (name.find("FPL_MainFeature") != std::string::npos)
    return true;

  // SEG_MainFeature on its own is the whole presentation too. Suffixed it is not - a disc that
  // assembles its feature from parts numbers them SEG_MainFeature_01, _EXT_12, _TH_23 and so on,
  // and those are minutes long where the feature is hours.
  return name == "SEG_MainFeature";
}

bool ProjectPlaylistInformation::IsEpisode() const
{
  return StringUtils::StartsWith(name, "EPL_") || StringUtils::StartsWith(name, "SEG_EPL_");
}

bool ProjectPlaylistInformation::IsSpecialFeature() const
{
  return StringUtils::StartsWith(name, "SF_") || StringUtils::StartsWith(name, "SEG_SF_");
}

bool ProjectPlaylistInformation::IsWarningOrLogo() const
{
  // The front matter a disc plays before anything else - studio idents, piracy and copyright
  // warnings, age certificates and disclaimers. Studios name these a dozen ways, so matching a
  // single prefix covers only one of them.
  static constexpr std::array MARKERS{"Warn", "Disclaimer", "Logo", "Parental", " AGE "};
  if (std::ranges::any_of(MARKERS, [this](std::string_view marker)
                          { return name.find(marker) != std::string::npos; }))
    return true;

  // The same thing under names that do not say so - an anti-piracy warning, a studio ident, an
  // intellectual property notice, a certificate
  return StringUtils::StartsWith(name, "WRN_") || StringUtils::StartsWith(name, "FBI") ||
         StringUtils::StartsWith(name, "Studio") || StringUtils::StartsWith(name, "IPR") ||
         name == "MPAA";
}

bool ProjectPlaylistInformation::IsMenu() const
{
  return name.find("Menu") != std::string::npos || StringUtils::StartsWith(name, "TMPL");
}

namespace
{
bool ReadProject(const CURL& url, ProjectInformation& projectInformation)
{
  const std::string projectFile{FindProjectFile(url)};
  if (projectFile.empty())
    return false; // most discs do not carry one

  CFile file;
  if (!file.Open(projectFile))
    return false;

  const int64_t size{file.GetLength()};
  if (size <= 0 || size > MAX_FILE_SIZE)
  {
    CLog::LogF(LOGDEBUG, "Invalid {} size {}", PROJECT_FILE, size);
    return false;
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
    CLog::LogF(LOGDEBUG, "Could not read {}", projectFile);
    return false;
  }

  try
  {
    return ParseProject(buffer, projectInformation);
  }
  catch (const std::exception& e)
  {
    CLog::LogF(LOGERROR, "Authoring project parsing failed - error {}", e.what());
    return false;
  }
}

} // namespace

void CProjectParser::LogProject(const ProjectInformation& projectInformation)
{
  if (!projectInformation.present)
    return; // most discs leave no project behind, and there is nothing to say about that


  CLog::LogF(LOGDEBUG, "Disc carries an authoring project naming {} playlist(s)",
              projectInformation.playlists.size());

  for (const auto& [playlist, information] : projectInformation.playlists)
  {
    std::string kind;
    if (information.IsFeature())
      kind = " - feature";
    else if (information.IsEpisode())
      kind = " - episode";
    else if (information.IsSpecialFeature())
      kind = " - special feature";
    else if (information.IsWarningOrLogo())
      kind = " - warning or logo";
    else if (information.IsMenu())
      kind = " - menu";

    CLog::LogF(LOGDEBUG, " Playlist {} is {}, {} {} of {}, {} play item(s){}", playlist,
                information.name, information.presentation, information.frameRate,
                fmt::format("{:%H:%M:%S}", information.duration), information.playItems, kind);
  }
}

bool CProjectParser::GetProject(const CURL& url, ProjectInformation& projectInformation)
{
  projectInformation = {};

  // Independent of the movie objects - a disc that left its authoring project behind names every
  // playlist, whether its navigation is HDMV or BD-J
  projectInformation.present = ReadProject(url, projectInformation);
  return projectInformation.present;
}
} // namespace XFILE
