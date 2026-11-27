/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PartyMode.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/ApplicationPlayLists.h"
#include "dialogs/GUIDialogProgress.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "media/LibraryQuery.h"
#include "media/LibraryQueryDatabase.h"
#include "media/LibraryQueryReferences.h"
#include "media/LibraryQueryType.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayListTypes.h"
#include "playlists/SmartFeed.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/SortUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"
#include "windows/GUIWindowPlayList.h"

#include <map>
#include <optional>

using namespace KODI;
using namespace KODI::MESSAGING;

namespace LIBRARY_QUERY = KODI::MEDIA::LIBRARY_QUERY;

namespace
{
using PARTYMODE::Library;
using PARTYMODE::Match;

//! The library party mode plays a source of a smart playlist type from, if it plays it
std::optional<Library> LibraryOf(const LIBRARY_QUERY::Source& source)
{
  if (source.itemType == MEDIA::TYPE::SONG)
  {
    return Library::Song;
  }
  if (source.itemType == MEDIA::TYPE::MUSIC_VIDEO)
  {
    return Library::MusicVideo;
  }
  return std::nullopt;
}

LIBRARY_QUERY::Source SourceOf(Library library)
{
  return library == Library::Song
             ? LIBRARY_QUERY::Source{MEDIA::MediaSection::MUSIC, MEDIA::TYPE::SONG}
             : LIBRARY_QUERY::Source{MEDIA::MediaSection::VIDEO, MEDIA::TYPE::MUSIC_VIDEO};
}

std::vector<int> IdsIn(const std::vector<Match>& matches, Library library)
{
  std::vector<int> ids;
  for (const auto& [matchLibrary, id] : matches)
    if (matchLibrary == library)
      ids.push_back(id);
  return ids;
}

std::vector<std::shared_ptr<CFileItem>> Fetch(const std::vector<Match>& matches)
{
  CFileItemList fetched;
  for (const Library library : {Library::Song, Library::MusicVideo})
  {
    if (const std::vector<int> ids{IdsIn(matches, library)}; !ids.empty())
      LIBRARY_QUERY::ItemsById(SourceOf(library), ids, fetched);
  }
  return PARTYMODE::InMatchOrder(matches, fetched);
}

bool Fail(int error, const std::string& message)
{
  HELPERS::ShowOKDialogLines(CVariant{257}, CVariant{16030}, CVariant{error}, CVariant{0});
  CLog::Log(LOGERROR, "PARTY MODE: {}, aborting", message);
  return false;
}

bool StartFeed(std::optional<PLAYLIST::Type> named, const std::string& xspPath)
{
  MEDIA::CLibraryQuery rules;
  const std::string rulesPath =
      xspPath.empty() ? PARTYMODE::RulesPath(named.value_or(PLAYLIST::Audio)) : xspPath;

  const bool rulesLoaded = rules.Load(rulesPath);
  if (!rulesLoaded && XFILE::CFile::Exists(rulesPath))
  {
    return Fail(477, "the rules in " + CURL::GetRedacted(rulesPath) + " could not be loaded");
  }
  const PLAYLIST::Type playList =
      named.value_or(rulesLoaded ? LIBRARY_QUERY::PlayListOf(rules.GetType()) : PLAYLIST::Audio);
  const bool isVideo = playList == PLAYLIST::Video;
  const LIBRARY_QUERY::TYPE content{
      rulesLoaded ? rules.GetType()
                  : (isVideo ? LIBRARY_QUERY::TYPE::MUSIC_VIDEOS : LIBRARY_QUERY::TYPE::SONGS)};

  CGUIDialogProgress* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(
          WINDOW_DIALOG_PROGRESS);
  dialog->SetHeading(CVariant{isVideo ? 20250 : 20121});
  dialog->SetLine(0, CVariant{isVideo ? 20251 : 20123});
  dialog->SetLine(1, CVariant{""});
  dialog->SetLine(2, CVariant{""});
  dialog->Open();

  std::optional<std::vector<Match>> found =
      PARTYMODE::FindMatches(rulesLoaded ? &rules : nullptr, content);
  if (!found)
  {
    dialog->Close();
    return Fail(16033, "could not open a database");
  }
  if (found->empty())
  {
    dialog->Close();
    return Fail(16031, "nothing matched");
  }

  const auto matches = std::make_shared<const std::vector<Match>>(std::move(*found));
  const auto feed = std::make_shared<PLAYLIST::CSmartFeed>(static_cast<int>(matches->size()),
                                                 [matches](const std::vector<int>& slice)
                                                 {
                                                   std::vector<Match> taken;
                                                   taken.reserve(slice.size());
                                                   for (const int entry : slice)
                                                     taken.push_back((*matches)[entry]);
                                                   return Fetch(taken);
                                                 });

  dialog->SetLine(0, CVariant{isVideo ? 20252 : 20124});
  dialog->Progress();

  // a party carries on past the end of its feed
  const bool playing =
      CServiceBroker::GetPlayLists()->PlayFeed(playList, feed, PLAYLIST::Repeat::All);
  dialog->Close();
  if (!playing)
    return Fail(16031, "nothing could be placed");

  CLog::Log(LOGINFO, "PARTY MODE: started, {} matching", feed->GetTotal());
  if (playList == PLAYLIST::Audio)
    ShowPlayListWindow(PLAYLIST::Audio);
  return true;
}
} // namespace

namespace KODI::PARTYMODE
{

bool Start(PLAYLIST::Type playList)
{
  return StartFeed(playList, "");
}

bool Start(const std::string& rulesPath)
{
  return StartFeed(std::nullopt, rulesPath);
}

std::optional<std::vector<Match>> FindMatches(const MEDIA::CLibraryQuery* rules,
                                              LIBRARY_QUERY::TYPE content)
{
  std::optional<MEDIA::CLibraryQueryReferences> references;
  if (rules)
  {
    references = MEDIA::CLibraryQueryReferences::Resolve(*rules);
    if (!references)
    {
      return std::nullopt;
    }
  }

  SortDescription shuffled;
  shuffled.sortBy = SortBy::RANDOM;
  std::vector<Match> matches;
  for (const LIBRARY_QUERY::Source& source : LIBRARY_QUERY::SourcesOf(content))
  {
    const std::optional<Library> library{LibraryOf(source)};
    if (!library)
    {
      continue;
    }
    const std::optional<std::vector<int>> ids{
        LIBRARY_QUERY::IdsOf(source, rules, references ? &*references : nullptr, shuffled)};
    if (!ids)
    {
      return std::nullopt;
    }
    for (const int id : *ids)
    {
      matches.emplace_back(*library, id);
    }
  }
  return matches;
}

bool CanStart(const std::string& rulesPath)
{
  MEDIA::CLibraryQuery rules;
  return rules.OpenAndReadName(CURL{rulesPath}) && LIBRARY_QUERY::CanFeedPartyMode(rules.GetType());
}

void Stop()
{
  CServiceBroker::GetPlayLists()->DropFeed();
}

bool Toggle(PLAYLIST::Type playList)
{
  if (!IsRunning(playList))
    return Start(playList);
  Stop();
  return true;
}

bool IsRunning()
{
  return CServiceBroker::GetPlayLists()->GetFedType().has_value();
}

bool IsRunning(PLAYLIST::Type playList)
{
  return CServiceBroker::GetPlayLists()->GetFedType() == playList;
}

std::string RulesPath(PLAYLIST::Type playList)
{
  return CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetUserDataItem(
      playList == PLAYLIST::Video ? "PartyMode-Video.xsp" : "PartyMode.xsp");
}

bool IsRulesPath(const std::string& path)
{
  return path == RulesPath(PLAYLIST::Audio) || path == RulesPath(PLAYLIST::Video);
}

std::vector<std::shared_ptr<CFileItem>> InMatchOrder(const std::vector<Match>& matches,
                                                     const CFileItemList& fetched)
{
  std::map<Match, std::shared_ptr<CFileItem>> byMatch;
  for (const auto& item : fetched)
  {
    if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDatabaseId() > 0)
      byMatch.try_emplace({Library::Song, item->GetMusicInfoTag()->GetDatabaseId()}, item);
    else if (item->HasVideoInfoTag())
      byMatch.try_emplace({Library::MusicVideo, item->GetVideoInfoTag()->m_iDbId}, item);
  }

  std::vector<std::shared_ptr<CFileItem>> ordered;
  for (const Match& match : matches)
    if (const auto it = byMatch.find(match); it != byMatch.end())
      ordered.push_back(it->second);
  return ordered;
}

} // namespace KODI::PARTYMODE
