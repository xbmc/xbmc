/*
 *  Copyright (C) 2005-2026 Team Kodi
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
#include "bluray/M2TSParser.h"
#include "bluray/MPLSParser.h"
#include "bluray/PlaylistStructure.h"
#include "bluray/StreamParser.h"
#include "filesystem/BlurayCallback.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryFactory.h"
#if defined(HAS_UDFREAD)
#include "filesystem/UDFContext.h"
#endif
#include "utils/EpisodeUtils.h"
#include "utils/LanguageTag.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
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
// Average number of times a playlist's clips must be played for it to be considered a loop
// rather than a movie that revisits a clip
constexpr size_t MIN_LOOPED_CLIP_PLAYS{3};

void AddOptionsAndSortMethods(const CURL& url,
                              CFileItemList& items,
                              CDiscDirectoryHelper::AllTitles allTitlesType,
                              bool blurayMenuSupport)
{
  // Add all titles and menu options
  CDiscDirectoryHelper::AddRootOptions(url, items, allTitlesType,
                                       blurayMenuSupport ? AddMenuOption::ADD_MENU
                                                         : AddMenuOption::NO_MENU);

  items.AddSortMethod(SortBy::TRACK_NUMBER, 554,
                      LABEL_MASKS("%L", "%D", "%L", "")); // FileName, Duration | Foldername, empty
  items.AddSortMethod(SortBy::SIZE, 553,
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
                              StreamDetails streamDetails,
                              BlurayPlaylistInformation& bpi,
                              std::map<unsigned int, ClipInformation>& clipCache)
{
  const std::string path{GetCachePath(url, realPath)};

  // Check cache
  if (CServiceBroker::GetBlurayDiscCache()->GetPlaylistInfo(path, playlist, bpi))
  {
    if (streamDetails == StreamDetails::DEFER || bpi.clipStreamsRead)
      return true;

    // The cached playlist was read without its clips' stream information
    // Retrieve that rather than parsing the .mpls a second time
    if (CMPLSParser::ReadClipStreams(url, bpi, clipCache))
    {
      CServiceBroker::GetBlurayDiscCache()->SetPlaylistInfo(path, playlist, bpi);
      return true;
    }
  }

  // Retrieve from disc
  bpi = {}; // May hold a cached entry that could not be upgraded
  if (!CMPLSParser::ReadMPLS(url, playlist, bpi, clipCache, streamDetails))
    return false;

  // Cache and return
  CServiceBroker::GetBlurayDiscCache()->SetPlaylistInfo(path, playlist, bpi);

  return true;
}

bool GetPlaylistInfoFromDisc(const CURL& url,
                             const std::string& realPath,
                             unsigned int playlist,
                             StreamDetails streamDetails,
                             PlaylistInformation& playlistInformation,
                             std::map<unsigned int, ClipInformation>& clipCache)
{
  BlurayPlaylistInformation bpi;
  if (!GetPlaylistInfoFromCache(url, realPath, playlist, streamDetails, bpi, clipCache))
    return false;

  StreamMap streams;
  if (streamDetails == StreamDetails::INCLUDE)
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

  CStreamParser::ConvertBlurayPlaylistInformation(bpi, playlistInformation, streams, streamDetails);

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

      PlaylistInformation& t = playlists.emplace_back();
      if (!GetPlaylistInfoFromDisc(url, realPath, playlist, StreamDetails::DEFER, t, clipCache))
      {
        CLog::LogF(LOGDEBUG, "Unable to get playlist {}", playlist);
        playlists.pop_back();
      }
    }
  }

  CLog::LogF(LOGDEBUG, "{} - {} files, {} playlists read", CURL::GetRedacted(url2.Get()),
             allTitles.Size(), playlists.size());

  if (playlists.empty())
  {
    CLog::LogF(LOGERROR, "No playlists could be read from {}", CURL::GetRedacted(url2.Get()));
    return false;
  }
  return true;
}

void RemoveDuplicatePlaylists(std::vector<PlaylistInformation>& playlists)
{
  // The stream number table describes what a playlist exposes rather than what its clip contains,
  // so two playlists sharing a clip but offering different streams are not seen as identical.
  // The duration is compared as well as the chapters, as two playlists can play the same clips
  // from the same chapter starts but to different out times (ie. they are distinct cuts).
  std::unordered_set<unsigned int> duplicatePlaylists;
  for (size_t i = 0; i + 1 < playlists.size(); ++i)
  {
    for (size_t j = i + 1; j < playlists.size(); ++j)
    {
      if (playlists[i].duration == playlists[j].duration &&
          playlists[i].audioStreams == playlists[j].audioStreams &&
          playlists[i].pgStreams == playlists[j].pgStreams &&
          playlists[i].chapters == playlists[j].chapters &&
          playlists[i].clips == playlists[j].clips)
      {
        duplicatePlaylists.emplace(std::max(playlists[i].playlist, playlists[j].playlist));
      }
    }
  }
  std::erase_if(playlists, [&duplicatePlaylists](const PlaylistInformation& p)
                { return duplicatePlaylists.contains(p.playlist); });
}

bool SetStreamDetails(const CURL& url,
                      const std::string& realPath,
                      CFileItem& item,
                      PlaylistInformation& title,
                      std::map<unsigned int, ClipInformation>& clipCache)
{
  if (!GetPlaylistInfoFromDisc(url, realPath, title.playlist, StreamDetails::INCLUDE, title,
                               clipCache))
    return false;

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

  info->m_streamDetails.DetermineBestStreams();
  return true;
}

std::shared_ptr<CFileItem> GetFileItem(const CURL& url,
                                       const std::string& realPath,
                                       PlaylistInformation& title,
                                       std::map<unsigned int, ClipInformation>& clipCache,
                                       StreamDetails getStreamDetails)
{
  CURL path{url};
  path.SetFileName(StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", title.playlist));
  const auto item{std::make_shared<CFileItem>(path.Get(), false)};
  const int duration{static_cast<int>(title.duration.count() / 1000)};
  item->GetVideoInfoTag()->SetDuration(duration);
  item->SetProperty("bluray_playlist", title.playlist);

  // Stream details are deferred when the playlist is only a candidate
  // as parsing the m2ts is expensive
  if (getStreamDetails == StreamDetails::INCLUDE &&
      !SetStreamDetails(url, realPath, *item, title, clipCache))
    CLog::LogF(LOGDEBUG, "Unable to get stream details for playlist {} of {}", title.playlist,
               CURL::GetRedacted(url.Get()));

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

    if (playlist >= 0)
      CLog::LogF(LOGDEBUG, "disc.inf main playlist {}", playlist);
    else
      CLog::LogF(LOGDEBUG, "disc.inf found but no main playlist");
  }
  return playlist;
}

} // namespace

bool CBlurayDirectory::FilterPlaylists(std::vector<PlaylistInformation>& playlists)
{
  // Log each removal, as a playlist dropped here is otherwise indistinguishable from one that
  // isn't on the disc at all
  const auto Remove{[&playlists](std::string_view reason, const auto& shouldRemove)
                    {
                      for (const auto& playlist : playlists | std::views::filter(shouldRemove))
                        CLog::LogF(LOGDEBUG, "Discarding playlist {} - {}", playlist.playlist,
                                   reason);
                      std::erase_if(playlists, shouldRemove);
                    }};

  // Remove playlists with no clips
  Remove("no clips", [](const PlaylistInformation& playlist) { return playlist.clips.empty(); });

  // Remove all clips less than a second in length
  Remove("shorter than a second",
         [](const PlaylistInformation& playlist) { return playlist.duration < 1s; });

  // Remove looping playlists - a few clips played over and over, as a menu background or a reel
  // assembled from everything on the disc (some discs have a playlist of 3 clips repeated 80 times).
  // A movie may revisit a clip, so a playlist is only discarded when its clips are played
  // MIN_LOOPED_CLIP_PLAYS times over
  Remove(
      "looping clips",
      [](const PlaylistInformation& playlist)
      {
        const std::unordered_set<unsigned int> clips{playlist.clips.begin(), playlist.clips.end()};
        if (clips.size() == playlist.clips.size())
          return false; // No clip is played more than once

        return clips.size() == 1 || playlist.clips.size() >= clips.size() * MIN_LOOPED_CLIP_PLAYS;
      });

  // Remove duplicate playlists
  RemoveDuplicatePlaylists(playlists);

  return !playlists.empty();
}

namespace
{
void AddPlaylists(const CURL& url,
                  const std::string& realPath,
                  CFileItemList& items,
                  std::vector<PlaylistInformation>& playlists,
                  std::map<unsigned int, ClipInformation>& clipCache)
{
  if (playlists.empty())
    return;

  for (auto& title : playlists)
    items.Add(GetFileItem(url, realPath, title, clipCache, StreamDetails::DEFER));
}
} // namespace

bool CBlurayDirectory::GetPlaylists(const CURL& url,
                                    const std::string& realPath,
                                    int flags,
                                    int playlist,
                                    CFileItemList& items,
                                    std::map<unsigned int, ClipInformation>& clipCache)
{
  try
  {
    std::vector<PlaylistInformation> playlists;
    if (playlist >= 0)
    {
      // Single playlist. Read the stream details now, as GetFileItem wants them straight away and
      // asking for them later would mean reading the .mpls a second time.
      PlaylistInformation& t = playlists.emplace_back();
      if (!GetPlaylistInfoFromDisc(url, realPath, playlist, StreamDetails::INCLUDE, t, clipCache))
      {
        CLog::LogF(LOGDEBUG, "Unable to get playlist {}", playlist);
        playlists.pop_back();
        return false;
      }

      // Generate FileItem including stream details
      items.Add(GetFileItem(url, realPath, playlists[0], clipCache, StreamDetails::INCLUDE));
    }
    else
    {
      // Get all playlists for movie/episode determination in DiscDirectoryHelper
      // or to retrieve all playlists

      // Get all playlists on disc (parse all .mpls files)
      if (!GetPlaylistsFromDisc(url, realPath, flags, playlists, clipCache))
        return false;

      // Remove invalid playlists (no clips, looping clips, duplicate playlists, length < 1s)
      if (!FilterPlaylists(playlists))
        return false; // No playlists remain

      // Generate FileItemList (stream details are filled in later, per selected playlist)
      AddPlaylists(url, realPath, items, playlists, clipCache);
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

namespace
{
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
    info.clips.push_back(clip);

    // Add/extend clip information
    const auto& it = clips.find(clip);
    if (it == clips.end())
    {
      // First reference to clip
      ClipInfo clipInfo;
      clipInfo.duration = titleInfo.clipDuration[clip];
      clipInfo.playlists.push_back(playlist);
      clips[clip] = clipInfo;
    }
    else
    {
      // Additional reference to clip, add this playlist
      if (std::ranges::find(it->second.playlists, playlist) == it->second.playlists.end())
        it->second.playlists.push_back(playlist);
    }
  }

  info.videoStreams = titleInfo.videoStreams;
  info.audioStreams = titleInfo.audioStreams;
  info.pgStreams = titleInfo.pgStreams;

  // Get languages
  const std::string langs{
      fmt::format("{}", fmt::join(titleInfo.audioStreams |
                                      std::views::transform([](const auto& stream)
                                                            { return stream.language.AsBcp47(); }),
                                  ","))};
  info.languages = langs;
  titleInfo.languages = langs;

  playlists[playlist] = info;
}
} // namespace

bool CBlurayDirectory::GetPlaylistsInformation(const CURL& url,
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
    if (CServiceBroker::GetBlurayDiscCache()->GetMaps(path, playlists, clips, allTitles))
    {
      CLog::LogF(LOGDEBUG, "Playlist information for {} retrieved from cache", path);
      return false;
    }

    // Get all titles on disc
    GetPlaylists(url, realPath, flags, ALL_PLAYLISTS, allTitles, clipCache);

    // Get information on all playlists
    // Including relationship between clips and playlists
    // List all playlists
    CLog::LogF(LOGDEBUG, "*** Playlist information ***");

    for (const auto& title : allTitles)
    {
      const int playlist{title->GetProperty("bluray_playlist").asInteger32(-1)};
      PlaylistInformation titleInfo;
      if (playlist == -1 || !GetPlaylistInfoFromDisc(url, realPath, playlist, StreamDetails::DEFER,
                                                     titleInfo, clipCache))
      {
        CLog::LogF(LOGDEBUG, "Unable to get playlist {}", playlist);
        continue;
      }

      ProcessPlaylist(playlists, titleInfo, clips);

      CLog::LogF(LOGDEBUG, "Playlist {}, Duration {}, Langs {}, Subs {}, Clips {} ", playlist,
                 title->GetVideoInfoTag()->GetDuration(), titleInfo.languages,
                 fmt::join(titleInfo.pgStreams |
                               std::views::transform([](const auto& stream)
                                                     { return stream.language.AsBcp47(); }),
                           ","),
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

    // Nothing could be read from the disc
    // Don't cache in case temporary read error etc.
    if (playlists.empty() || clips.empty())
    {
      CLog::LogF(LOGERROR, "No playlist information could be read from {}, so not caching it",
                 CURL::GetRedacted(path));
      return false;
    }

    // Cache
    CServiceBroker::GetBlurayDiscCache()->SetMaps(path, playlists, clips, allTitles);
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
  m_blurayInitialized = false;
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

void CBlurayDirectory::SetPlaylistStreamDetails(unsigned int playlist, CFileItem& item)
{
  // Only the playlist number is needed, the rest is read from the disc (or the disc cache)
  PlaylistInformation information;
  information.playlist = playlist;

  if (!XFILE::SetStreamDetails(m_url, m_realPath, item, information, m_clipCache))
    CLog::LogF(LOGDEBUG, "Unable to get stream details for playlist {} of {}", playlist,
               CURL::GetRedacted(m_url.Get()));
}

std::string CBlurayDirectory::GetBlurayTitle()
{
  return GetDiscInfoString(DiscInfo::TITLE);
}

std::string CBlurayDirectory::GetBlurayID()
{
  return GetDiscInfoString(DiscInfo::ID);
}

std::string CBlurayDirectory::GetDiscInfoString(DiscInfo info)
{
  if (!EnsureBlurayOpen())
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

  // Deal with options
  unsigned int duration{0};
  if (m_url.HasOption("duration"))
  {
    duration = StringUtils::ToUint32(m_url.GetOption("duration"), 0);
    m_url.RemoveOption("duration");
  }

  std::string root{m_url.GetHostName()};
  std::string file{m_url.GetFileName()};
  URIUtils::RemoveSlashAtEnd(file);
  URIUtils::RemoveSlashAtEnd(root);

  // Resolve the path but leave the disc closed.
  // Most requests are now served from the disc cache or by parsing a single playlist.
  // Neither needs libbluray or disc.inf, so both are deferred.
  SetRealPath(root);

  //
  // These options also return 'All Titles' and 'Menu' options (if supported on disc)
  //
  // /root/titles                       - get main (length >70% longest) playlists (sorted by longest -> shortest - for movies)
  // /root/titles/episodes              - get main playlists (sorted by longest -> shortest - for episodes)
  //
  // These options just return the requested playlist(s) (or nothing if not found)
  //
  // /root/main	                        - get the single main movie playlist only (assumes longest)
  // /root/main/all                     - get all possible main movie playlists (ie. multiple versions on disc)
  // /root/episode/<season>/<episode>   - get playlists that correspond with S<season>E<episode>
  // /root/episode/all                  - get all episodes
  //
  // /root/titles/all                   - get all playlists (sorted by longest -> shortest - for movies)
  // /root/titles/episodes/all          - get all playlists (sorted by playlist number - for episodes)
  //

  if (StringUtils::StartsWith(file, "root"))
  {
    ClipMap clips;
    PlaylistMap playlists;
    CFileItemList allTitles;
    GetPlaylistsInformation(m_url, m_realPath, m_flags, allTitles, clips, playlists, m_clipCache);

    CDiscDirectoryHelper helper{[this](unsigned int playlist, CFileItem& item)
                                { SetPlaylistStreamDetails(playlist, item); }};

    if (StringUtils::StartsWith(file, "root/titles") && file != "root/titles/episodes")
    {

      if (file == "root/titles")
        helper.GetMoviePlaylists(m_url, items, allTitles, GetMainPlaylist(), GetTitle::MAIN, clips,
                                 playlists);
      else if (file == "root/titles/all")
        helper.GetMoviePlaylists(m_url, items, allTitles, GetMainPlaylist(), GetTitle::ALL, clips,
                                 playlists);
      else if (file == "root/titles/episodes/all")
        helper.GetAllEpisodePlaylists(m_url, items, allTitles, GetTitle::ALL, {}, clips, playlists);
      else
        CLog::LogF(LOGDEBUG, "Invalid path {} for bluray playlist parsing", file);

      const bool success{!items.IsEmpty()};

      // Add all titles and menu option (if menus supported on disc)
      if (!StringUtils::EndsWith(file, "/all"))
        AddOptionsAndSortMethods(m_url, items, CDiscDirectoryHelper::AllTitles::MOVIES,
                                 HasMenuSupport());

      return success;
    }

    if (StringUtils::StartsWith(file, "root/main"))
    {
      if (file == "root/main")
        helper.GetMoviePlaylists(m_url, items, allTitles, GetMainPlaylist(), GetTitle::SINGLE,
                                 clips, playlists);
      else if (file == "root/main/all")
        helper.GetMoviePlaylists(m_url, items, allTitles, GetMainPlaylist(), GetTitle::MAIN, clips,
                                 playlists);
      else
        CLog::LogF(LOGDEBUG, "Invalid path {} for bluray playlist parsing", file);

      return !items.IsEmpty();
    }

    if (StringUtils::StartsWith(file, "root/episode") || file == "root/titles/episodes")
    {
      // Get episodes on disc by parsing file/path
      // Done first as if called from VideoInfoScanner during library scan
      //  not all episodes may be in database yet
      std::string path = URIUtils::GetDiscBase(m_url.Get());
      URIUtils::RemoveSlashAtEnd(path);
      CFileItem item(path, false);
      Episodes episodesOnDisc;
      CEpisodeUtils::EnumerateEpisodeItem(&item, episodesOnDisc);

      // Now get any available information from database
      const std::vector<CVideoInfoTag> episodesInDatabase{
          CDiscDirectoryHelper::GetEpisodesOnDisc(m_url)};
      if (!episodesInDatabase.empty())
      {
        // Update data with database information (where available)
        for (auto& episode : episodesOnDisc)
        {
          const auto& it{std::ranges::find_if(
              episodesInDatabase, [&episode](const CVideoInfoTag& e)
              { return e.m_iSeason == episode.iSeason && e.m_iEpisode == episode.iEpisode; })};
          if (it != episodesInDatabase.end())
          {
            episode.duration = it->GetDuration();
            episode.strTitle = it->GetTitle();
          }
        }
      }

      int episodeIndex{-1};
      if (file != "root/episode/all" && file != "root/titles/episodes")
      {
        // Get desired episode from path
        CRegExp regex{true, CRegExp::autoUtf8, R"((root\/episode\/)(\d{1,4})\/(\d{1,4}))"};
        if (regex.RegFind(file) == -1)
          return false; // Invalid episode path
        const int season{std::stoi(regex.GetMatch(2))};
        const int episode{std::stoi(regex.GetMatch(3))};

        // Check desired episode is on disc
        const auto& it{
            std::ranges::find_if(episodesOnDisc, [&season, &episode](const Episode& e)
                                 { return e.iSeason == season && e.iEpisode == episode; })};
        if (it == episodesOnDisc.end())
          return false; // Episode not on disc
        episodeIndex = static_cast<int>(std::distance(episodesOnDisc.begin(), it));

        // Add duration from scraper
        it->duration = duration;
      }

      // Get episode playlists
      bool success{false};
      if (file == "root/titles/episodes")
      {
        helper.GetAllEpisodePlaylists(m_url, items, allTitles, GetTitle::MAIN, episodesOnDisc,
                                      clips, playlists);
        success = !items.IsEmpty();
        AddOptionsAndSortMethods(m_url, items, CDiscDirectoryHelper::AllTitles::EPISODES,
                                 HasMenuSupport());
      }
      else
      {
        helper.GetEpisodePlaylists(m_url, items, allTitles, episodeIndex, episodesOnDisc, clips,
                                   playlists);
        success = !items.IsEmpty();
      }

      return success;
    }

    return false;
  }

  // Single playlist (eg. bluray://host/BDMV/PLAYLIST/00001.mpls)
  if (URIUtils::IsBlurayPath(m_url.Get()))
  {
    if (const int playlist{URIUtils::GetBlurayPlaylistFromPath(m_url.Get())}; playlist >= 0)
      return GetPlaylists(m_url, m_realPath, m_flags, playlist, items, m_clipCache);
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

void CBlurayDirectory::SetRealPath(const std::string& root)
{
  m_realPath = root;

  if (const auto fileHandler{CDirectoryFactory::Create(CURL{root})}; fileHandler)
    m_realPath = fileHandler->ResolveMountPoint(root);

#if defined(HAS_UDFREAD)
  m_udfMount.emplace(root);
#endif
}

bool CBlurayDirectory::EnsureBlurayOpen()
{
  if (m_blurayInitialized)
    return true;

  if (m_realPath.empty())
  {
    CLog::LogF(LOGERROR, "No disc path, SetRealPath must be called first");
    return false;
  }

  bd_set_debug_handler(CBlurayCallback::bluray_logger);
  bd_set_debug_mask(DBG_CRIT | DBG_BLURAY | DBG_NAV);

  m_bd = bd_init();

  if (!m_bd)
  {
    CLog::LogF(LOGERROR, "Failed to initialize libbluray");
    return false;
  }

  const std::string menuLang{g_langInfo.GetDVDMenuLanguage().AsIso6392T()};
  bd_set_player_setting_str(m_bd, BLURAY_PLAYER_SETTING_MENU_LANG, menuLang.c_str());

  if (!bd_open_files(m_bd, &m_realPath, CBlurayCallback::dir_open, CBlurayCallback::file_open))
  {
    CLog::LogF(LOGERROR, "Failed to open {}", CURL::GetRedacted(m_realPath));
    Dispose();
    return false;
  }
  m_blurayInitialized = true;

  return true;
}

bool CBlurayDirectory::InitializeBluray(const std::string& root)
{
  SetRealPath(root);
  return EnsureBlurayOpen();
}

bool CBlurayDirectory::HasMenuSupport()
{
  const std::string path{GetCachePath(m_url, m_realPath)};

  if (bool menuSupport{false};
      CServiceBroker::GetBlurayDiscCache()->GetMenuSupport(path, menuSupport))
    return menuSupport;

  // Only libbluray can answer this, so the disc has to be opened
  if (!EnsureBlurayOpen())
    return false; // Not cached, so a disc that failed to open is retried rather than written off

  const BLURAY_DISC_INFO* discInfo{GetDiscInfo()};
  const bool menuSupport{discInfo && !discInfo->no_menu_support};
  CServiceBroker::GetBlurayDiscCache()->SetMenuSupport(path, menuSupport);

  CLog::LogF(LOGDEBUG, "Disc {} {} menus", CURL::GetRedacted(m_realPath),
             menuSupport ? "supports" : "does not support");

  return menuSupport;
}

int CBlurayDirectory::GetMainPlaylist()
{
  const std::string path{GetCachePath(m_url, m_realPath)};

  if (int mainPlaylist{-1};
      CServiceBroker::GetBlurayDiscCache()->GetMainPlaylist(path, mainPlaylist))
    return mainPlaylist;

  // Cache main playlist from disc.inf (or -1 if not found)
  const int mainPlaylist{GetMainPlaylistFromDisc(m_url)};
  CServiceBroker::GetBlurayDiscCache()->SetMainPlaylist(path, mainPlaylist);

  return mainPlaylist;
}

const BLURAY_DISC_INFO* CBlurayDirectory::GetDiscInfo() const
{
  return bd_get_disc_info(m_bd);
}
} // namespace XFILE
