/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MovieObjectParser.h"

#include "IndexParser.h"
#include "M2TSParser.h"
#include "MPLSParser.h"
#include "NavigationCommand.h"
#include "PlaylistStructure.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/ranges.h>

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
     CLog::LogF(LOGDEBUG, "Truncated MovieObject.bdmv - object {} is incomplete",
                  object);
      return false;
    }

    const uint16_t numberOfCommands = readWord(offset + 2);
    offset += 4;

    // The command block is fixed stride, so one check covers the whole inner loop
    if (static_cast<uint64_t>(offset) +
            static_cast<uint64_t>(numberOfCommands) * NAVIGATION_COMMAND_SIZE >
        end)
    {
     CLog::LogF(LOGDEBUG, "Truncated MovieObject.bdmv - object {} is incomplete",
                  object);
      return false;
    }

    MovieObject movieObject;
    movieObject.object = object;

    CRegisterFile registers;
    for (uint32_t i = 0; i < numberOfCommands; ++i, offset += NAVIGATION_COMMAND_SIZE)
    {
      const NavigationCommand command{DecodeNavigationCommand(data + offset)};

      if (command.IsJumpObject() || command.IsPlayPlaylist() || command.IsSetRegister())
        movieObject.commands.push_back(command);

      if (command.IsJumpObject())
        continue;

      if (!command.IsPlayPlaylist())
      {
        registers.Apply(command);
        continue;
      }

      // Resolved against this object alone
      movieObject.plays.emplace_back(
          MovieObjectPlay{.playlist = registers.ResolveDestination(command),
                          .registerNumber = RegisterNumber(command.destination),
                          .playerStatusRegister =
                              IsPlayerStatusRegister(command.destination)});
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
                      const CRegisterFile& seed,
                      std::set<unsigned int>& playlists)
{
  std::set<unsigned int> visited;
  std::vector<std::pair<unsigned int, CRegisterFile>> pending{{startObject, seed}};

  while (!pending.empty())
  {
    auto [object, registers] = std::move(pending.back());
    pending.pop_back();
    if (!visited.insert(object).second)
      continue; // objects call each other in cycles

    const auto it{byObject.find(object)};
    if (it == byObject.end())
      continue;

    for (const NavigationCommand& command : it->second->commands)
    {
      if (command.IsJumpObject())
      {
        // The object jumped to carries registers
        if (const std::optional<uint32_t> target{registers.ResolveDestination(command)})
          pending.emplace_back(*target, registers);
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

//! \brief Find the playlists reachable from the top menu, following the branches between objects.
void FindMenuPlaylists(MovieObjectInformation& information)
{
  if (information.index.topMenu.objectType != BLURAY_OBJECT_TYPE::HDMV ||
      information.index.topMenu.movieObject == MOVIE_OBJECT_NONE)
    return;

  ObjectMap byObject{MapObjects(information)};

  // Registers start empty
  CollectPlaylists(byObject, information.index.topMenu.movieObject, {}, information.menuPlaylists);
}


/*!
 \brief Follow the menu's buttons to playlists.

 A button jumps to a title. index.bdmv maps that title to a movie object,
 and that object does the playing.
 */
void FindMenuTargets(MovieObjectInformation& information)
{
  if (information.menus.empty())
    return;

  const ObjectMap byObject{MapObjects(information)};

  for (const IGMenuInformation& menu : information.menus | std::views::values)
  {
    for (const IGPageInformation& page : menu.pages)
    {
      for (const IGButtonInformation& button : page.buttons)
      {
        // The few buttons that do name a playlist need no resolving
        if (button.playlist)
        {
          information.menuTargetPlaylists.insert(*button.playlist);
          continue;
        }

        if (!button.title)
          continue;

        // Title 0 is the tomenu, so a button naming it is the one that goes back where it came from,
        // and TITLE_FIRST_PLAYBACK is what the disc opens with
        if (*button.title == TITLE_TOP_MENU || *button.title == TITLE_FIRST_PLAYBACK)
          continue;

        // Titles are numbered from 1, and a button can still name one the disc does not have
        if (*button.title > information.index.titles.size())
          continue;

        const IndexObjectInformation& title{information.index.titles[*button.title - 1]};
        if (title.objectType != BLURAY_OBJECT_TYPE::HDMV ||
            title.movieObject == MOVIE_OBJECT_NONE)
          continue; // a BD-J title plays nothing this parser can see

        // The button commonly puts the playlist number in a register before jumping
        CRegisterFile seed;
        seed.Seed(button.registers);
        CollectPlaylists(byObject, title.movieObject, seed, information.menuTargetPlaylists);
      }
    }
  }
}

/*! \brief Read the menu out of the clips the disc's menu playlists reference. */
void FindMenus(const CURL& url, MovieObjectInformation& information)
{
  // Following the branches out of the top menu can reach a good part of the disc, so the clips are
  // gathered first and only then scanned, cheapest and most likely first.
  std::map<unsigned int, ClipInformation> clipCache;
  std::vector<unsigned int> subPathClips; // a menu in a clip of its own - short
  std::vector<unsigned int> playItemClips; // a menu multiplexed into the video - feature sized
  std::set<unsigned int> seen;

  for (const unsigned int playlist : information.menuPlaylists)
  {
    // The stream details of the clips are not needed, only which of them carry a menu
    BlurayPlaylistInformation playlistInformation;
    if (!CMPLSParser::ReadMPLS(url, playlist, playlistInformation, clipCache, StreamDetails::DEFER))
      continue;

    for (const SubPlayItemInformation& subPlayItem : playlistInformation.subPlayItems)
    {
      if (subPlayItem.subPathType == BLURAY_SUBPATH_TYPE::INTERACTIVE_GRAPHICS_PRESENTATION_MENU &&
          !subPlayItem.clips.empty() && seen.insert(subPlayItem.clips.front().clip).second)
        subPathClips.emplace_back(subPlayItem.clips.front().clip);
    }

    for (const PlayItemInformation& playItem : playlistInformation.playItems)
    {
      if (!playItem.interactiveGraphicStreams.empty() && !playItem.angleClips.empty() &&
          seen.insert(playItem.angleClips.front().clip).second)
        playItemClips.emplace_back(playItem.angleClips.front().clip);
    }
  }

  for (const unsigned int clip : subPathClips)
  {
    IGMenuInformation menu;
    if (CM2TSParser::GetMenu(url, clip, menu))
    {
      information.menus.try_emplace(clip, std::move(menu));
      return; // one menu describes the whole disc
    }
  }

  // No menu of its own, so it can only be inside the video. Those clips are large, so stop at the
  // first that has one rather than reading through every candidate.
  for (const unsigned int clip : playItemClips)
  {
    IGMenuInformation menu;
    if (CM2TSParser::GetMenu(url, clip, menu))
    {
      information.menus.try_emplace(clip, std::move(menu));
      return;
    }
  }
}

/*! \brief Describe where an object is referenced from. */
std::string DescribeObject(unsigned int object, const MovieObjectInformation& information)
{
  const MovieObjectUsage usage{information.GetMovieObjectUsage(object)};

  std::vector<std::string> roles;
  if (usage.firstPlayback)
    roles.emplace_back("first playback");
  if (usage.topMenu)
    roles.emplace_back("top menu");
  if (!usage.titles.empty())
  {
    std::vector<std::string> titles;
    titles.reserve(usage.titles.size());
    for (const unsigned int title : usage.titles)
      titles.emplace_back(std::to_string(title));
    roles.emplace_back(StringUtils::Format(usage.titles.size() == 1 ? "title {}" : "titles {}",
                                           StringUtils::Join(titles, ", ")));
  }

  // Objects index.bdmv does not name are reached from another object, not selected by the player
  if (roles.empty())
    return StringUtils::Format("object {} (no title)", object);
  return StringUtils::Format("object {} ({})", object, StringUtils::Join(roles, ", "));
}

/*! \brief Log what the buttons of a decoded menu lead to. */
void LogMenu(unsigned int clip, const IGMenuInformation& menu)
{
 CLog::LogF(LOGDEBUG, "Clip {} carries a {}x{} {} menu with {} page(s)", clip,
              menu.width, menu.height, menu.popup ? "pop-up" : "top", menu.pages.size());

  for (const IGPageInformation& page : menu.pages)
  {
    const auto navigation = std::ranges::count_if(page.buttons, [](const IGButtonInformation& b)
                                                  { return b.IsNavigation(); });
   CLog::LogF(LOGDEBUG, " Page {} - {} button(s), {} of which navigate", page.page,
                page.buttons.size(), navigation);

    for (const IGButtonInformation& button : page.buttons)
    {
      if (!button.IsNavigation())
        continue;

      std::vector<std::string> details;
      if (button.playlist)
        details.emplace_back(StringUtils::Format("plays playlist {}", *button.playlist));
      if (button.playItem)
        details.emplace_back(StringUtils::Format("from play item {}", *button.playItem));
      if (button.playMark)
        details.emplace_back(StringUtils::Format("from chapter {}", *button.playMark));
      if (button.title)
        details.emplace_back(StringUtils::Format("jumps to title {}", *button.title));
      if (button.linkPlayItem)
        details.emplace_back(StringUtils::Format(
            "links to play item {} of the playing playlist", *button.linkPlayItem));
      if (button.linkPlayMark)
        details.emplace_back(StringUtils::Format("links to chapter {} of the playing playlist",
                                                 *button.linkPlayMark));
      for (const auto& [reg, value] : button.registers)
        details.emplace_back(StringUtils::Format("register {} = {}", reg, value));

     CLog::LogF(LOGDEBUG, "  Button {} {}", button.button,
                  StringUtils::Join(details, ", "));
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

  FindMenuPlaylists(information);
  FindMenus(url, information);
  FindMenuTargets(information);

  return true;
}

void CMovieObjectParser::LogMovieObject(const MovieObjectInformation& information)
{
  if (!information.indexRead)
    return;

  if (!information.index.HasHdmvObjects())
  {
   CLog::LogF(LOGDEBUG,
                "Disc navigation is entirely BD-J - MovieObject.bdmv has nothing to describe");
    return;
  }

  if (!information.index.HasHdmvTopMenu())
   CLog::LogF(LOGDEBUG,
                "Disc has a BD-J top menu - MovieObject.bdmv does not describe the menu structure");

  for (const MovieObject& movieObject : information.movieObjects)
  {
    for (const MovieObjectPlay& play : movieObject.plays)
    {
      if (CServiceBroker::GetLogging().CanLogComponent(LOGBLURAY))
      {
        if (play.playlist)
          CLog::LogF(LOGDEBUG, "{} plays playlist {}",
                     DescribeObject(movieObject.object, information), *play.playlist);
        else
          CLog::LogF(
              LOGDEBUG, "{} plays the playlist in {} register {}, which could not be followed",
              DescribeObject(movieObject.object, information),
              play.playerStatusRegister ? "player status" : "general purpose", play.registerNumber);
      }
    }
  }

  unsigned int played{0};
  unsigned int fromRegister{0};
  for (const MovieObject& movieObject : information.movieObjects)
  {
    for (const MovieObjectPlay& play : movieObject.plays)
    {
      if (play.playlist)
        ++played;
      else
        ++fromRegister;
    }
  }
  CLog::LogF(LOGDEBUG,
             "MovieObject.bdmv - {} object(s), {} playlist(s) played outright, {} from a register "
             "that could not be followed",
             information.movieObjects.size(), played, fromRegister);

  if (information.menuPlaylists.empty())
  {
    // A disc whose menu is BD-J can still name an HDMV object as its top menu, but that object
    // only sets up and hands over - it plays nothing, so there is no menu of ours to find. Where
    // index.bdmv named a BD-J top menu outright, that has already been reported above.
    if (information.index.HasHdmvTopMenu())
      CLog::LogF(LOGDEBUG,
                 "Top menu object plays no playlist - it only hands over to the disc's BD-J "
                 "application, so there is no menu to read");
    return;
  }

  if (CServiceBroker::GetLogging().CanLogComponent(LOGBLURAY))
  {
    for (const auto& [clip, menu] : information.menus)
      LogMenu(clip, menu);
  }

  if (!information.menuTargetPlaylists.empty())
  {
    CLog::LogF(LOGDEBUG, "The menu's buttons lead to playlist(s) {}",
               fmt::join(information.menuTargetPlaylists, ", "));
    return;
  }

  if (information.menus.empty())
  {
    // The playlists the top menu plays carry no interactive graphics, so there are no buttons to
    // follow. The menu is drawn some other way, or lives in a clip this did not look at.
    CLog::LogF(LOGDEBUG, "No menu could be read from the playlist(s) the top menu plays");
    return;
  }

  // Say where the buttons lead and what stopped the trail
  std::set<unsigned int> titles;
  for (const IGMenuInformation& menu : information.menus | std::views::values)
  {
    for (const IGPageInformation& page : menu.pages)
    {
      for (const IGButtonInformation& button : page.buttons)
      {
        if (button.title)
          titles.insert(*button.title);
      }
    }
  }

  std::set<unsigned int> waiting;
  for (const MovieObject& movieObject : information.movieObjects)
  {
    for (const MovieObjectPlay& play : movieObject.plays)
    {
      if (!play.playlist)
        waiting.insert(play.registerNumber);
    }
  }

  CLog::LogF(LOGDEBUG,
             "The menu was read but none of its buttons lead to a playlist that could be followed "
             "- they jump to title(s) {}, whose object(s) play from register(s) {}",
             titles.empty() ? std::string{"none"} : fmt::format("{}", fmt::join(titles, ", ")),
             waiting.empty() ? std::string{"none"} : fmt::format("{}", fmt::join(waiting, ", ")));
}

} // namespace XFILE
