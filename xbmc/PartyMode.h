/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class CFileItem;
class CFileItemList;

namespace KODI::MEDIA
{
class CLibraryQuery;
namespace LIBRARY_QUERY
{
enum class TYPE;
}
} // namespace KODI::MEDIA

namespace KODI::PLAYLIST
{
enum class Type;
}

/*!
 * \brief Party mode: a smart feed played in random order, wrapping, with anything queued playing
 * next. The party buttons, labels, builtin and JSON-RPC all report a playing feed as party mode.
 */
namespace KODI::PARTYMODE
{

/*!
 * \brief Play the party rules for this playlist, PartyMode.xsp or PartyMode-Video.xsp; with no
 * rules file, everything of the playlist's kind.
 */
bool Start(PLAYLIST::Type playList);

/*!
 * \brief Play the rules in a smart playlist file, on the playlist its type gives.
 */
bool Start(const std::string& rulesPath);

//! Whether Start() can play the rules in this smart playlist file: songs, music videos or mixed.
bool CanStart(const std::string& rulesPath);

//! Stop party mode; see CApplicationPlayLists::DropFeed().
void Stop();

/*!
 * \brief Stop party mode if it plays on this playlist; otherwise start it there.
 * \return false if it was to start and did not.
 */
bool Toggle(PLAYLIST::Type playList);

//! Whether a feed is playing on either playlist.
bool IsRunning();
//! Whether a feed is playing on this playlist.
bool IsRunning(PLAYLIST::Type playList);

//! The playlist's party rules file in the profile: PartyMode.xsp, or PartyMode-Video.xsp for Video.
std::string RulesPath(PLAYLIST::Type playList);
//! Whether the path is either playlist's party rules file.
bool IsRulesPath(const std::string& path);

enum class Library
{
  Song,
  MusicVideo
};
using Match = std::pair<Library, int>;

/*!
 * \brief What the rules match of the songs and music videos this type lists. With no rules, all
 * of them match.
 * \return nullopt if a database could not be opened or the rules could not be put into SQL.
 */
std::optional<std::vector<Match>> FindMatches(const MEDIA::CLibraryQuery* rules,
                                              MEDIA::LIBRARY_QUERY::TYPE content);

/*!
 * \brief The fetched items the matches name, in the matches' order. Matches that were not fetched
 * are left out.
 */
std::vector<std::shared_ptr<CFileItem>> InMatchOrder(const std::vector<Match>& matches,
                                                     const CFileItemList& fetched);

} // namespace KODI::PARTYMODE
