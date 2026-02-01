/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "BlurayDirectory.h"

#include "BlurayDiscCache.h"
#include "File.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "bluray/BitReader.h"
#include "bluray/M2TSParser.h"
#include "bluray/MPLSParser.h"
#include "bluray/PlaylistStructure.h"
#include "bluray/StreamParser.h"
#include "filesystem/BlurayCallback.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryFactory.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/LangCodeExpander.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <map>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <libbluray/bluray-version.h>
#include <libbluray/bluray.h>
#include <libbluray/log_control.h>

using namespace std::chrono_literals;

namespace XFILE
{
namespace // Bluray parsing
{
void AddOptionsAndSort(const CURL& url, CFileItemList& items, bool blurayMenuSupport)
{
  // Add all titles and menu options
  CDiscDirectoryHelper::AddRootOptions(
      url, items, blurayMenuSupport ? AddMenuOption::ADD_MENU : AddMenuOption::NO_MENU);

  items.AddSortMethod(SortByTrackNumber, 554,
                      LABEL_MASKS("%L", "%D", "%L", "")); // FileName, Duration | Foldername, empty
  items.AddSortMethod(SortBySize, 553,
                      LABEL_MASKS("%L", "%I", "%L", "%I")); // FileName, Size | Foldername, Size
}

std::string GetCachePath(const CURL& url, const std::string& realPath)
{
  if (url.Get().empty())
    return realPath;
  std::string path{url.GetHostName()};
  if (path.empty())
    path = url.Get(); // Could be drive letter
  return path;
}

bool GetPlaylistInfoFromCache(const CURL& url,
                              const std::string& realPath,
                              unsigned int playlist,
                              BlurayPlaylistInformation& bpi,
                              std::map<unsigned int, ClipInformation>& clipCache)
{
  // Check cache
  if (const std::string path{GetCachePath(url, realPath)};
      !CServiceBroker::GetBlurayDiscCache()->GetPlaylistInfo(path, playlist, bpi))
  {
    // Retrieve from disc
    if (!CMPLSParser::ReadMPLS(url, playlist, bpi, clipCache))
      return false;

    // Cache and return
    CServiceBroker::GetBlurayDiscCache()->SetPlaylistInfo(path, playlist, bpi);
  }

  return true;
}

bool GetPlaylistInfoFromDisc(const CURL& url,
                             const std::string& realPath,
                             unsigned int playlist,
                             bool parseM2TS,
                             PlaylistInformation& playlistInformation,
                             std::map<unsigned int, ClipInformation>& clipCache)
{
  BlurayPlaylistInformation bpi;
  if (!GetPlaylistInfoFromCache(url, realPath, playlist, bpi, clipCache))
    return false;

  StreamMap streams;
  if (parseM2TS)
  {
    const std::string path{GetCachePath(url, realPath)};

    // Check cache
    if (!CServiceBroker::GetBlurayDiscCache()->GetPlaylistStreamInfo(path, playlist, streams))
    {
      // Retrieve from disc
      if (!CM2TSParser::GetStreams(url, bpi, streams))
        return false;

      // Cache and return
      CServiceBroker::GetBlurayDiscCache()->SetPlaylistStreamInfo(path, playlist, streams);
    }
  }

  CStreamParser::ConvertBlurayPlaylistInformation(bpi, playlistInformation, streams);

  return true;
}

bool GetPlaylistsFromDisc(const CURL& url,
                          const std::string& realPath,
                          int flags,
                          std::vector<PlaylistInformation>& playlists,
                          std::map<unsigned int, ClipInformation>& clipCache)
{
  const CURL url2{URIUtils::AddFileToFolder(url.GetHostName(), "BDMV", "PLAYLIST", "")};
  CDirectory::CHints hints;
  hints.flags = flags;
  CFileItemList allTitles;
  if (!CDirectory::GetDirectory(url2, allTitles, hints))
    return false;

  // Get information on all playlists
  CRegExp pl{true, CRegExp::autoUtf8, R"((\d{5}.mpls))"};
  for (const auto& title : allTitles)
  {
    const CURL url3{title->GetPath()};
    const std::string filename{URIUtils::GetFileName(url3.GetFileName())};
    if (pl.RegFind(filename) != -1)
    {
      const unsigned int playlist{static_cast<unsigned int>(std::stoi(pl.GetMatch(1)))};

      PlaylistInformation t{};
      if (!GetPlaylistInfoFromDisc(url, realPath, playlist, false, t, clipCache))
        CLog::LogF(LOGDEBUG, "Unable to get playlist {}", playlist);
      else
        playlists.emplace_back(t);
    }
  }
  return true;
}

void RemoveDuplicatePlaylists(std::vector<PlaylistInformation>& playlists)
{
  std::unordered_set<unsigned int> duplicatePlaylists;
  for (unsigned int i = 0; i < playlists.size() - 1; ++i)
  {
    for (unsigned int j = i + 1; j < playlists.size(); ++j)
    {
      if (playlists[i].audioStreams == playlists[j].audioStreams &&
          playlists[i].pgStreams == playlists[j].pgStreams &&
          playlists[i].chapters == playlists[j].chapters &&
          playlists[i].clips == playlists[j].clips)
      {
        duplicatePlaylists.emplace(playlists[j].playlist);
      }
    }
  }
  std::erase_if(playlists, [&duplicatePlaylists](const PlaylistInformation& p)
                { return duplicatePlaylists.contains(p.playlist); });
}

void RemoveShortPlaylists(std::vector<PlaylistInformation>& playlists)
{
  const std::chrono::milliseconds minimumDuration{CServiceBroker::GetSettingsComponent()
                                                      ->GetAdvancedSettings()
                                                      ->m_minimumEpisodePlaylistDuration *
                                                  1000};
  if (std::ranges::any_of(playlists, [&minimumDuration](const PlaylistInformation& playlist)
                          { return playlist.duration >= minimumDuration; }))
  {
    std::erase_if(playlists, [&minimumDuration](const PlaylistInformation& playlist)
                  { return playlist.duration < minimumDuration; });
  }
}

void SortPlaylists(std::vector<PlaylistInformation>& playlists, SortTitles sort, int mainPlaylist)
{
  std::ranges::sort(playlists,
                    [&sort](const PlaylistInformation& i, const PlaylistInformation& j)
                    {
                      if (sort == SortTitles::SORT_TITLES_MOVIE)
                      {
                        if (i.duration == j.duration)
                          return i.playlist < j.playlist;
                        return i.duration > j.duration;
                      }
                      return i.playlist < j.playlist;
                    });

  const auto& pivot{
      std::ranges::find_if(playlists, [&mainPlaylist](const PlaylistInformation& title)
                           { return title.playlist == static_cast<unsigned int>(mainPlaylist); })};
  if (pivot != playlists.end())
    std::rotate(playlists.begin(), pivot, pivot + 1);
}

bool IncludePlaylist(GetTitle job,
                     const PlaylistInformation& title,
                     std::chrono::milliseconds minDuration,
                     int mainPlaylist,
                     unsigned int maxPlaylist)
{
  using enum GetTitle;
  return job == GET_TITLES_ALL || job == GET_TITLES_EPISODES ||
         (job == GET_TITLES_MAIN && title.duration >= minDuration) ||
         (job == GET_TITLES_ONE && (title.playlist == static_cast<unsigned int>(mainPlaylist) ||
                                    (mainPlaylist == -1 && title.playlist == maxPlaylist)));
}

void SetStreamDetails(const CURL& url,
                      const std::string& realPath,
                      CFileItem& item,
                      PlaylistInformation& title,
                      std::map<unsigned int, ClipInformation>& clipCache)
{
  GetPlaylistInfoFromDisc(url, realPath, title.playlist, true, title, clipCache);

  // Video stream (first one only)
  CVideoInfoTag* info{item.GetVideoInfoTag()};
  if (!title.videoStreams.empty())
    info->m_streamDetails.SetStreams(title.videoStreams[0],
                                     static_cast<int>(title.duration.count() / 1000),
                                     AudioStreamInfo{}, SubtitleStreamInfo{}, CStreamDetail::MEDIA);
  else
    info->m_streamDetails.SetStreams(VideoStreamInfo{}, 0, AudioStreamInfo{}, SubtitleStreamInfo{},
                                     CStreamDetail::MEDIA);

  // Audio streams
  for (const auto& audio : title.audioStreams)
    info->m_streamDetails.AddStream(new CStreamDetailAudio(audio, CStreamDetail::MEDIA));

  // Subtitles
  for (const auto& subtitle : title.pgStreams)
    info->m_streamDetails.AddStream(new CStreamDetailSubtitle(subtitle, CStreamDetail::MEDIA));
}

std::shared_ptr<CFileItem> GetFileItem(const CURL& url,
                                       const std::string& realPath,
                                       PlaylistInformation& title,
                                       const std::string& label,
                                       std::map<unsigned int, ClipInformation>& clipCache)
{
  CURL path{url};
  path.SetFileName(StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", title.playlist));
  const auto item{std::make_shared<CFileItem>(path.Get(), false)};
  const int duration{static_cast<int>(title.duration.count() / 1000)};
  item->GetVideoInfoTag()->SetDuration(duration);
  item->SetProperty("bluray_playlist", title.playlist);
  const std::string buf{StringUtils::Format(label, title.playlist)};
  item->SetTitle(buf);
  item->SetLabel(buf);
  const std::string chap{StringUtils::Format(g_localizeStrings.Get(25007), title.chapters.size(),
                                             StringUtils::SecondsToTimeString(duration))};
  item->SetLabel2(chap);
  item->SetSize(0);
  item->SetArt("icon", "DefaultVideo.png");

  SetStreamDetails(url, realPath, *item, title, clipCache);

  return item;
}

int GetMainPlaylistFromDisc(const CURL& url)
{
  const std::string& root{url.GetHostName()};
  const std::string discInfPath{URIUtils::AddFileToFolder(root, "disc.inf")};
  CFile file;
  std::string line;
  line.reserve(1024);
  int playlist{-1};

  if (file.Open(discInfPath))
  {
    CLog::LogF(LOGDEBUG, "disc.inf found");
    CRegExp pl{true, CRegExp::autoUtf8, R"((?:playlists=)(\d+))"};
    uint8_t maxLines{100};
    while ((maxLines > 0) && file.ReadLine(line))
    {
      maxLines--;
      if (pl.RegFind(line) != -1)
      {
        playlist = std::stoi(pl.GetMatch(1));
        break;
      }
    }
    file.Close();
  }
  return playlist;
}

bool FilterPlaylists(std::vector<PlaylistInformation>& playlists,
                     GetTitle job,
                     SortTitles sort,
                     int mainPlaylist)
{
  // Remove playlists with no clips
  std::erase_if(playlists,
                [](const PlaylistInformation& playlist) { return playlist.clips.empty(); });

  // Remove all clips less than a second in length
  std::erase_if(playlists,
                [](const PlaylistInformation& playlist) { return playlist.duration < 1s; });

  // Remove playlists with duplicate clips
  std::erase_if(playlists,
                [](const PlaylistInformation& playlist)
                {
                  std::unordered_set<unsigned int> clips;
                  for (const auto& clip : playlist.clips)
                    clips.emplace(clip);
                  return clips.size() < playlist.clips.size();
                });

  // Remove duplicate playlists
  // For episodes playlist selection happens in CDiscDirectoryHelper
  if (job != GetTitle::GET_TITLES_ALL && job != GetTitle::GET_TITLES_EPISODES &&
      playlists.size() > 1)
    RemoveDuplicatePlaylists(playlists);

  // Remove playlists below minimum duration (default 5 minutes) unless that would leave no playlists
  if (job != GetTitle::GET_TITLES_ALL)
    RemoveShortPlaylists(playlists);

  // No playlists found
  if (playlists.empty())
    return false;

  // Sort
  // Movies - placing main title - if present - first, then by duration
  // Episodes - by playlist number
  if (sort != SortTitles::SORT_TITLES_NONE)
    SortPlaylists(playlists, sort, mainPlaylist);

  return true;
}

void AddPlaylists(const CURL& url,
                  const std::string& realPath,
                  GetTitle job,
                  CFileItemList& items,
                  int mainPlaylist,
                  std::vector<PlaylistInformation>& playlists,
                  std::map<unsigned int, ClipInformation>& clipCache)
{
  if (playlists.empty())
    return;

  // Now we have curated playlists, find longest (for main title derivation)
  const auto& it{std::ranges::max_element(playlists, {}, &PlaylistInformation::duration)};
  const std::chrono::milliseconds maxDuration{it->duration};
  const unsigned int maxPlaylist{it->playlist};

  const std::chrono::milliseconds minDuration{maxDuration * MAIN_TITLE_LENGTH_PERCENT / 100};
  for (auto& title : playlists)
  {
    if (IncludePlaylist(job, title, minDuration, mainPlaylist, maxPlaylist))
    {
      items.Add(GetFileItem(url, realPath, title,
                            title.playlist == static_cast<unsigned int>(mainPlaylist)
                                ? g_localizeStrings.Get(25004) /* Main Title */
                                : g_localizeStrings.Get(25005) /* Title */,
                            clipCache));
    }
  }
}

bool GetPlaylists(const CURL& url,
                  const std::string& realPath,
                  int flags,
                  GetTitle job,
                  CFileItemList& items,
                  SortTitles sort,
                  std::map<unsigned int, ClipInformation>& clipCache)
{
  try
  {
    std::vector<PlaylistInformation> playlists;
    int mainPlaylist{-1};

    // See if disc.inf for main playlist
    if (job == GetTitle::GET_TITLES_MAIN)
    {
      mainPlaylist = GetMainPlaylistFromDisc(url);
      if (mainPlaylist != -1)
      {
        // Only main playlist is needed
        PlaylistInformation t{};
        if (!GetPlaylistInfoFromDisc(url, realPath, mainPlaylist, false, t, clipCache))
        {
          CLog::LogF(LOGDEBUG, "Unable to get playlist {}", mainPlaylist);
          mainPlaylist = -1;
        }
        else
          playlists.emplace_back(t);
      }
    }
    else if (static_cast<int>(job) >= 0)
    {
      // Single playlist
      PlaylistInformation t{};
      mainPlaylist = static_cast<int>(job);
      if (!GetPlaylistInfoFromDisc(url, realPath, mainPlaylist, false, t, clipCache))
      {
        CLog::LogF(LOGDEBUG, "Unable to get playlist {}", mainPlaylist);
        return false;
      }
      playlists.emplace_back(t);
    }

    if (mainPlaylist >= 0)
    {
      items.Add(GetFileItem(url, realPath, playlists[0], g_localizeStrings.Get(25005) /* Title */,
                            clipCache));
    }
    else
    {
      if (playlists.empty() && !GetPlaylistsFromDisc(url, realPath, flags, playlists, clipCache))
        return false;

      if (!FilterPlaylists(playlists, job, sort, mainPlaylist))
        return false; // No playlists remain

      AddPlaylists(url, realPath, job, items, mainPlaylist, playlists, clipCache);
    }

    return !items.IsEmpty();
  }
  catch (const std::out_of_range& e)
  {
    CLog::LogF(LOGERROR, "Exception getting playlists - error {}", e.what());
    return false;
  }
  catch (const std::exception& e)
  {
    CLog::LogF(LOGERROR, "Exception getting playlists - error {}", e.what());
    return false;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Exception getting playlists");
    return false;
  }
}

void ProcessPlaylist(PlaylistMap& playlists, PlaylistInformation& titleInfo, ClipMap& clips)
{
  const unsigned int playlist{titleInfo.playlist};

  // Save playlist
  PlaylistInformation info;
  info.playlist = playlist;

  // Save playlist duration and chapters
  info.duration = titleInfo.duration;
  info.chapters = titleInfo.chapters;

  // Get clips
  for (const auto& clip : titleInfo.clips)
  {
    // Add clip to playlist
    info.clips.emplace_back(clip);

    // Add/extend clip information
    const auto& it = clips.find(clip);
    if (it == clips.end())
    {
      // First reference to clip
      ClipInfo clipInfo;
      clipInfo.duration = titleInfo.clipDuration[clip];
      clipInfo.playlists.emplace_back(playlist);
      clips[clip] = clipInfo;
    }
    else
    {
      // Additional reference to clip, add this playlist
      it->second.playlists.emplace_back(playlist);
    }
  }

  // Get languages
  const std::string langs{fmt::format(
      "{}", fmt::join(titleInfo.audioStreams | std::views::transform(&StreamInfo::language), ","))};
  info.languages = langs;
  titleInfo.languages = langs;

  playlists[playlist] = info;
}

bool GetPlaylistsInformation(const CURL& url,
                             const std::string& realPath,
                             int flags,
                             CFileItemList& allTitles,
                             ClipMap& clips,
                             PlaylistMap& playlists,
                             std::map<unsigned int, ClipInformation>& clipCache)
{
  try
  {
    // Check cache
    const std::string& path{url.GetHostName()};
    if (CServiceBroker::GetBlurayDiscCache()->GetMaps(path, playlists, clips))
    {
      CLog::LogF(LOGDEBUG, "Playlist information for {} retrieved from cache", path);
      return false;
    }

    // Get all titles on disc
    // Sort by playlist for grouping later
    GetPlaylists(url, realPath, flags, GetTitle::GET_TITLES_EPISODES, allTitles,
                 SortTitles::SORT_TITLES_EPISODE, clipCache);

    // Get information on all playlists
    // Including relationship between clips and playlists
    // List all playlists
    CLog::LogF(LOGDEBUG, "*** Playlist information ***");

    for (const auto& title : allTitles)
    {
      const int playlist{title->GetProperty("bluray_playlist").asInteger32(0)};
      PlaylistInformation titleInfo;
      if (!GetPlaylistInfoFromDisc(url, realPath, playlist, false, titleInfo, clipCache))
      {
        CLog::LogF(LOGDEBUG, "Unable to get playlist {}", playlist);
        continue;
      }

      ProcessPlaylist(playlists, titleInfo, clips);

      CLog::LogF(LOGDEBUG, "Playlist {}, Duration {}, Langs {}, Clips {} ", playlist,
                 title->GetVideoInfoTag()->GetDuration(), titleInfo.languages,
                 fmt::join(titleInfo.clips, ","));
    }

    // List clip info (automatically sorted as map)
    for (const auto& c : clips)
    {
      const auto& [clip, clipInformation] = c;
      CLog::LogF(LOGDEBUG, "Clip {:d} duration {:d} - playlists {}", clip,
                 clipInformation.duration.count() / 1000,
                 fmt::join(clipInformation.playlists, ","));
    }

    CLog::LogF(LOGDEBUG, "*** Playlist information End ***");

    // Cache
    CServiceBroker::GetBlurayDiscCache()->SetMaps(path, playlists, clips);
    CLog::LogF(LOGDEBUG, "Playlist information for {} cached", path);

    return true;
  }
  catch (const std::out_of_range& e)
  {
    CLog::LogF(LOGERROR, "Error getting playlists information - error {}", e.what());
  }
  catch (const std::exception& e)
  {
    CLog::LogF(LOGERROR, "Error getting playlists information - error {}", e.what());
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Error getting playlists information");
  }
  return false;
}
} // namespace

CBlurayDirectory::CBlurayDirectory()
{
  m_clipCache.clear();
}

CBlurayDirectory::~CBlurayDirectory()
{
  Dispose();
}

void CBlurayDirectory::Dispose()
{
  if (m_bd)
  {
    bd_close(m_bd);
    m_bd = nullptr;
  }
}

bool CBlurayDirectory::Resolve(CFileItem& item) const
{
  const std::string originalPath{item.GetDynPath()};
  if (CURL::Decode(originalPath).find("removable://") != std::string::npos)
  {
    std::string newPath;
    if (URIUtils::GetExtension(originalPath) == ".mpls")
    {
      // Playlist (.mpls) so return bluray:// path with removable:// resolved to physical disc
      const CURL pathUrl{originalPath};
      newPath = URIUtils::GetBlurayPlaylistPath(item.GetPath());
      newPath = URIUtils::AddFileToFolder(newPath, pathUrl.GetFileNameWithoutPath());
    }
    else
    {
      // Not a playlist resolve removable:// to physical disc
      newPath = item.GetPath();
    }

    item.SetDynPath(newPath);
    CLog::LogF(LOGDEBUG, "Resolved removable bluray path from {} to {}", originalPath, newPath);
  }
  return true;
}

std::string CBlurayDirectory::GetBasePath(const CURL& url)
{
  if (!url.IsProtocol("bluray"))
    return {};

  const CURL url2(url.GetHostName()); // strip bluray://
  if (url2.IsProtocol("udf")) // ISO
    return URIUtils::GetDirectory(url2.GetHostName()); // strip udf://
  return url2.Get(); // BDMV
}

std::string CBlurayDirectory::GetBlurayTitle() const
{
  return GetDiscInfoString(DiscInfo::TITLE);
}

std::string CBlurayDirectory::GetBlurayID() const
{
  return GetDiscInfoString(DiscInfo::ID);
}

std::string CBlurayDirectory::GetDiscInfoString(DiscInfo info) const
{
  if (!m_blurayInitialized)
    return "";

  const BLURAY_DISC_INFO* discInfo{GetDiscInfo()};
  if (!discInfo || !discInfo->bluray_detected)
    return {};

  switch (info)
  {
    case DiscInfo::TITLE:
    {
      std::string title;

#if (BLURAY_VERSION > BLURAY_VERSION_CODE(1, 0, 0))
      title = discInfo->disc_name ? discInfo->disc_name : "";
#endif

      return title;
    }
    case DiscInfo::ID:
    {
      std::string id;

#if (BLURAY_VERSION > BLURAY_VERSION_CODE(1, 0, 0))
      id = discInfo->udf_volume_id ? discInfo->udf_volume_id : "";
      if (id.empty())
        id = CUtil::GetHexString(discInfo->disc_id, 10);
#endif

      return id;
    }
  }

  return "";
}

bool CBlurayDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  Dispose();
  m_url = url;
  std::string root{m_url.GetHostName()};
  std::string file{m_url.GetFileName()};
  URIUtils::RemoveSlashAtEnd(file);
  URIUtils::RemoveSlashAtEnd(root);

  if (!InitializeBluray(root))
    return false;

  // /root                              - get main (length >70% longest) playlists
  // /root/titles                       - get all playlists
  // /root/episode/<season>/<episode>   - get playlists that correspond with S<season>E<episode>
  //                                      if none found then return all playlists
  // /root/episode/all                  - get all episodes
  if (file == "root")
  {
    GetPlaylists(url, m_realPath, m_flags, GetTitle::GET_TITLES_MAIN, items,
                 SortTitles::SORT_TITLES_MOVIE, m_clipCache);
    AddOptionsAndSort(m_url, items, m_blurayMenuSupport);
    return (items.Size() > 2);
  }

  if (file == "root/titles")
    return GetPlaylists(url, m_realPath, m_flags, GetTitle::GET_TITLES_ALL, items,
                        SortTitles::SORT_TITLES_MOVIE, m_clipCache);

  if (StringUtils::StartsWith(file, "root/episode"))
  {
    // Get episodes on disc
    const std::vector<CVideoInfoTag> episodesOnDisc{CDiscDirectoryHelper::GetEpisodesOnDisc(url)};

    int season{-1};
    int episode{-1};
    int episodeIndex{-1};
    if (file != "root/episode/all")
    {
      // Get desired episode from path
      CRegExp regex{true, CRegExp::autoUtf8, R"((root\/episode\/)(\d{1,4})\/(\d{1,4}))"};
      if (regex.RegFind(file) == -1)
        return false; // Invalid episode path
      season = std::stoi(regex.GetMatch(2));
      episode = std::stoi(regex.GetMatch(3));

      // Check desired episode is on disc
      const auto& it{
          std::ranges::find_if(episodesOnDisc, [&season, &episode](const CVideoInfoTag& e)
                               { return e.m_iSeason == season && e.m_iEpisode == episode; })};
      if (it == episodesOnDisc.end())
        return false; // Episode not on disc
      episodeIndex = static_cast<int>(std::distance(episodesOnDisc.begin(), it));
    }

    // Get playlist, clip and language information
    ClipMap clips;
    PlaylistMap playlists;
    CFileItemList allTitles;
    GetPlaylistsInformation(url, m_realPath, m_flags, allTitles, clips, playlists, m_clipCache);

    // Get episode playlists
    CDiscDirectoryHelper helper;
    helper.GetEpisodePlaylists(m_url, items, allTitles, episodeIndex, episodesOnDisc, clips,
                               playlists);

    // Heuristics failed so return all playlists
    if (items.IsEmpty())
      GetPlaylists(url, m_realPath, m_flags, GetTitle::GET_TITLES_EPISODES, items,
                   SortTitles::SORT_TITLES_EPISODE, m_clipCache);

    // Add all titles and menu options
    AddOptionsAndSort(m_url, items, m_blurayMenuSupport);

    return (items.Size() > 2);
  }

  if (URIUtils::IsBlurayPath(url.Get()))
  {
    if (int playlist{URIUtils::GetBlurayPlaylistFromPath(url.Get())}; playlist >= 0)
      return GetPlaylists(url, m_realPath, m_flags, static_cast<GetTitle>(playlist), items,
                          SortTitles::SORT_TITLES_NONE, m_clipCache);
    return false;
  }

  const CURL url2{CURL(URIUtils::GetDiscUnderlyingFile(m_url))};
  CDirectory::CHints hints;
  hints.flags = m_flags;
  if (!CDirectory::GetDirectory(url2, items, hints))
    return false;

  // Found items will have underlying protocol (eg. udf:// or smb://)
  // in path so add back bluray://
  // (so properly recognised in cache as bluray:// files for CFile:Exists() etc..)
  CURL url3{m_url};
  const std::string baseFileName{url3.GetFileName()};
  for (const auto& item : items)
  {
    std::string path{item->GetPath()};
    URIUtils::RemoveSlashAtEnd(path);
    std::string fileName{URIUtils::GetFileName(path)};

    if (URIUtils::HasSlashAtEnd(item->GetPath()))
      URIUtils::AddSlashAtEnd(fileName);

    url3.SetFileName(URIUtils::AddFileToFolder(baseFileName, fileName));
    item->SetPath(url3.Get());
  }

  url3.SetFileName("menu");
  const std::shared_ptr<CFileItem> item{std::make_shared<CFileItem>()};
  item->SetPath(url3.Get());
  items.Add(item);

  return true;
}

bool CBlurayDirectory::InitializeBluray(const std::string& root)
{
  bd_set_debug_handler(CBlurayCallback::bluray_logger);
  bd_set_debug_mask(DBG_CRIT | DBG_BLURAY | DBG_NAV);

  m_bd = bd_init();

  if (!m_bd)
  {
    CLog::LogF(LOGERROR, "Failed to initialize libbluray");
    return false;
  }

  std::string langCode;
  g_LangCodeExpander.ConvertToISO6392T(g_langInfo.GetDVDMenuLanguage(), langCode);
  bd_set_player_setting_str(m_bd, BLURAY_PLAYER_SETTING_MENU_LANG, langCode.c_str());

  m_realPath = root;

  if (const auto fileHandler{CDirectoryFactory::Create(CURL{root})}; fileHandler)
    m_realPath = fileHandler->ResolveMountPoint(root);

  if (!bd_open_files(m_bd, &m_realPath, CBlurayCallback::dir_open, CBlurayCallback::file_open))
  {
    CLog::LogF(LOGERROR, "Failed to open {}", CURL::GetRedacted(root));
    return false;
  }
  m_blurayInitialized = true;

  const BLURAY_DISC_INFO* discInfo{GetDiscInfo()};
  m_blurayMenuSupport = discInfo && !discInfo->no_menu_support;

  return true;
}

const BLURAY_DISC_INFO* CBlurayDirectory::GetDiscInfo() const
{
  return bd_get_disc_info(m_bd);
}
} // namespace XFILE
