/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MovieObjectParser.h"

#include "IndexParser.h"
#include "NavigationCommand.h"
#include "URL.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace XFILE
{
namespace
{
// Constants for magic values
constexpr std::string_view MOBJ_HEADER = "MOBJ";
constexpr unsigned int MIN_BUFFER_SIZE = 50;
constexpr unsigned int HEADER_OFFSET = 44;
constexpr unsigned int DATA_LENGTH_OFFSET = 40;
constexpr unsigned int NUM_OBJECTS_OFFSET = 48;
constexpr unsigned int INITIAL_OFFSET = 50;

// MovieObject.bdmv is a small index file - anything larger is not worth reading
constexpr int64_t MAX_FILE_SIZE = 4 * 1024 * 1024;

/*! \brief Walk the movie objects, recording what each one plays and branches to. */
bool ParseMovieObject(const std::span<const std::byte> buffer, MovieObjectInformation& information)
{
  // Check minimum size and header
  if (buffer.size() < MIN_BUFFER_SIZE)
  {
    CLog::LogF(LOGDEBUG, "Invalid MovieObject.bdmv - too small");
    return false;
  }

  const std::byte* const data = buffer.data();

  if (!std::equal(MOBJ_HEADER.begin(), MOBJ_HEADER.end(), reinterpret_cast<const char*>(data)))
  {
    CLog::LogF(LOGDEBUG, "Invalid MovieObject.bdmv header");
    return false;
  }

  // Big endian reads without a per-access bounds check - every offset used below is
  // validated against the declared table size first.
  const auto readWord = [data](unsigned int offset) -> uint16_t
  {
    return static_cast<uint16_t>(std::to_integer<uint16_t>(data[offset + 1]) |
                                 std::to_integer<uint16_t>(data[offset]) << 8);
  };
  const auto readDWord = [data](unsigned int offset) -> uint32_t
  {
    return std::to_integer<uint32_t>(data[offset + 3]) |
           std::to_integer<uint32_t>(data[offset + 2]) << 8 |
           std::to_integer<uint32_t>(data[offset + 1]) << 16 |
           std::to_integer<uint32_t>(data[offset]) << 24;
  };

  const uint32_t dataLength = readDWord(DATA_LENGTH_OFFSET);
  const uint16_t numberOfObjects = readWord(NUM_OBJECTS_OFFSET);

  if (buffer.size() < static_cast<uint64_t>(dataLength) + HEADER_OFFSET)
  {
    CLog::LogF(LOGDEBUG, "Invalid MovieObject.bdmv - too small");
    return false;
  }

  // The whole movie object table is known to be present, so reads below can skip
  // the per-access bounds check in favour of validating each object once.
  const unsigned int end = HEADER_OFFSET + dataLength;

  information.movieObjects.reserve(numberOfObjects);

  unsigned int offset = INITIAL_OFFSET;
  for (uint32_t object = 0; object < numberOfObjects; ++object)
  {
    if (offset + 4 > end)
    {
      CLog::LogF(LOGDEBUG, "Truncated MovieObject.bdmv - object {} is incomplete", object);
      return false;
    }

    const uint16_t numberOfCommands = readWord(offset + 2);
    offset += 4;

    // The command block is fixed stride, so one check covers the whole inner loop
    if (static_cast<uint64_t>(offset) +
            static_cast<uint64_t>(numberOfCommands) * NAVIGATION_COMMAND_SIZE >
        end)
    {
      CLog::LogF(LOGDEBUG, "Truncated MovieObject.bdmv - object {} is incomplete", object);
      return false;
    }

    MovieObject movieObject;
    movieObject.object = object;

    for (uint32_t i = 0; i < numberOfCommands; ++i, offset += NAVIGATION_COMMAND_SIZE)
    {
      const NavigationCommand command{DecodeNavigationCommand(data + offset)};

      if (command.IsJumpObject() || command.IsPlayPlaylist() || command.IsSetRegister())
        movieObject.commands.push_back(command);
    }

    information.movieObjects.emplace_back(std::move(movieObject));
  }
  return true;
}

//! \brief The disc's movie objects by object number.
using ObjectMap = std::map<unsigned int, const MovieObject*>;

ObjectMap MapObjects(const MovieObjectInformation& information)
{
  ObjectMap byObject;
  for (const MovieObject& movieObject : information.movieObjects)
    byObject[movieObject.object] = &movieObject;
  return byObject;
}

//! \brief Every playlist reachable from an object, by following the jumps from it
void CollectPlaylists(const ObjectMap& byObject,
                      unsigned int startObject,
                      std::set<unsigned int>& playlists,
                      size_t& budget)
{
  // What an object plays can depend on the registers it is reached with, so it is followed again
  // with registers it has not been followed with yet. Objects call each other in cycles, and a cycle
  // counting in a register would never end, hence the limit.
  constexpr size_t MAX_REGISTER_STATES{16};
  std::map<unsigned int, std::set<std::map<unsigned int, uint32_t>>> visited;
  std::vector<std::pair<unsigned int, CRegisterFile>> pending{{startObject, {}}};

  while (!pending.empty())
  {
    auto [object, registers] = std::move(pending.back());
    pending.pop_back();
    if (auto& states{visited[object]};
        states.size() >= MAX_REGISTER_STATES || !states.insert(registers.GetValues()).second)
      continue;

    const auto it{byObject.find(object)};
    if (it == byObject.end())
      continue;

    for (const NavigationCommand& command : it->second->commands)
    {
      if (budget == 0)
        return;
      --budget;

      if (command.IsJumpObject())
      {
        // The object jumped to carries registers, which a crafted disc can make many of
        if (const std::optional<uint32_t> target{registers.ResolveDestination(command)})
        {
          const size_t cost{1 + registers.GetValues().size()};
          if (cost > budget)
            return;
          budget -= cost;
          pending.emplace_back(*target, registers);
        }
        continue;
      }

      if (!command.IsPlayPlaylist())
      {
        registers.Apply(command);
        continue;
      }

      if (const std::optional<uint32_t> playlist{registers.ResolveDestination(command)})
        playlists.insert(*playlist);
    }
  }
}

//! \brief The playlists the disc's titles play, in the order of the titles
void FindTitlePlaylists(MovieObjectInformation& information)
{
  const ObjectMap byObject{MapObjects(information)};

  // Shared by all the titles, so a crafted disc can't make the work unbounded
  size_t budget{1'000'000};
  std::set<unsigned int> found;
  for (const IndexObjectInformation& title : information.index.titles)
  {
    if (title.objectType != BLURAY_OBJECT_TYPE::HDMV || title.movieObject == MOVIE_OBJECT_NONE)
      continue; // a BD-J title plays nothing this parser can see

    std::set<unsigned int> playlists;
    CollectPlaylists(byObject, title.movieObject, playlists, budget);
    for (const unsigned int playlist : playlists)
    {
      if (found.insert(playlist).second)
        information.titlePlaylists.emplace_back(playlist);
    }
  }
}
} // namespace

bool CMovieObjectParser::GetMovieObject(const CURL& url, MovieObjectInformation& information)
{
  information = {};

  // index.bdmv is the disc's table of contents. It says whether MovieObject.bdmv is used at all,
  // and gives the real title numbers, which the movie objects themselves do not carry.
  if (!CIndexParser::ReadIndex(url, information.index))
    return false;
  information.indexRead = true;

  // A disc that navigates entirely through BD-J has nothing in MovieObject.bdmv to describe. The
  // disc has still been read, so say so and let the caller hold that rather than looking again.
  if (!information.index.HasHdmvObjects())
    return true;

  const std::string movieObjectFile{
      URIUtils::AddFileToFolder(url.GetHostName(), "BDMV", "MovieObject.bdmv")};

  CFile file;
  if (!file.Open(movieObjectFile))
    return true;

  const int64_t size{file.GetLength()};
  if (size < MIN_BUFFER_SIZE || size > MAX_FILE_SIZE)
  {
    CLog::LogF(LOGDEBUG, "Invalid MovieObject.bdmv size {}", size);
    return true;
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
    CLog::LogF(LOGDEBUG, "Could not read MovieObject.bdmv");
    return true;
  }

  if (!ParseMovieObject(buffer, information))
    return true;
  information.movieObjectsRead = true;

  FindTitlePlaylists(information);

  return true;
}

} // namespace XFILE
