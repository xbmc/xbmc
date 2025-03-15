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
#include <memory>
#include <regex>
#include <set>
#include <string>

#include <fmt/chrono.h>
#include <libbluray/bluray-version.h>
#include <libbluray/bluray.h>
#include <libbluray/log_control.h>

using namespace XFILE;

CBlurayDirectory::CBlurayDirectory()
{
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

std::string CBlurayDirectory::GetBasePath(const CURL& url)
{
  if (!url.IsProtocol("bluray"))
    return {};

  const CURL url2(url.GetHostName()); // strip bluray://
  if (url2.IsProtocol("udf")) // ISO
    return url2.GetHostName(); // strip udf://
  return url2.Get(); // BDMV
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
        id = CDiscDirectoryHelper::HexToString(discInfo->disc_id, 10);
#endif

      return id;
    }
  }

  return "";
}

void CBlurayDirectory::GetPlaylistsInformation(ClipMap& clips, PlaylistMap& playlists)
{
  // Check cache
  const std::string path{m_url.GetHostName()};
  if (CServiceBroker::GetBlurayDiscCache()->GetMaps(path, playlists, clips))
  {
    CLog::LogF(LOGDEBUG, "Playlist information for {} retrieved from cache", path);
    return;
  }

  // Get all titles on disc
  // Sort by playlist for grouping later
  CFileItemList allTitles;
  GetPlaylists(GetTitlesJob::GET_TITLES_ALL, allTitles, SortTitlesJob::SORT_TITLES_EPISODE);

  // Get information on all playlists
  // Including relationship between clips and playlists
  // List all playlists
  CLog::LogF(LOGDEBUG, "*** Playlist information ***");

  for (const auto& title : allTitles)
  {
    const int playlist{title->GetVideoInfoTag()->m_iTrack};
    PlaylistInfo titleInfo;
    if (!GetPlaylistInfoFromDisc(playlist, titleInfo))
    {
      CLog::LogF(LOGDEBUG, "Unable to get playlist {}", playlist);
    }
    else
    {
      // Save playlist
      PlaylistInfo info;
      info.playlist = playlist;

      // Save playlist duration and chapters
      info.duration = titleInfo.duration;
      info.chapters = titleInfo.chapters;

      // Get clips
      std::string clipsStr;
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

        clipsStr += std::to_string(clip) + ",";
      }
      if (!clipsStr.empty())
        clipsStr.pop_back(); // Remove last ','

      playlists[playlist] = info;

      // Get languages
      std::string langs;
      for (const auto& audio : titleInfo.audioStreams)
        langs += audio.lang + ",";
      if (!langs.empty())
        langs.pop_back(); // Remove last ','

      playlists[playlist].languages = langs;

      CLog::LogF(LOGDEBUG, "Playlist {}, Duration {}, Langs {}, Clips {} ", playlist,
                 title->GetVideoInfoTag()->GetDuration(), langs, clipsStr);
    }
  }

  // List clip info (automatically sorted as map)
  for (const auto& c : clips)
  {
    const auto& [clip, clipInformation] = c;
    std::string ps{StringUtils::Format("Clip {0:d} duration {1:d} - playlists ", clip,
                                       clipInformation.duration)};
    for (const auto& playlist : clipInformation.playlists)
      ps += std::to_string(playlist) + ",";
    ps.pop_back(); // Remove last ','
    CLog::LogF(LOGDEBUG, "{}", ps);
  }

  CLog::LogF(LOGDEBUG, "*** Playlist information End ***");

  // Cache
  CServiceBroker::GetBlurayDiscCache()->SetMaps(path, playlists, clips);
  CLog::LogF(LOGDEBUG, "Playlist information for {} cached", path);
}

std::shared_ptr<CFileItem> CBlurayDirectory::GetFileItem(const PlaylistInfo& title,
                                                         const std::string& label) const
{
  const auto item{std::make_shared<CFileItem>("", false)};
  CURL path{m_url};
  std::string buf{StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", title.playlist)};
  path.SetFileName(buf);
  item->SetPath(path.Get());
  const int duration = static_cast<int>(title.duration);
  item->GetVideoInfoTag()->SetDuration(duration);
  item->GetVideoInfoTag()->m_iTrack = static_cast<int>(title.playlist);
  buf = StringUtils::Format(label, title.playlist);
  item->m_strTitle = buf;
  item->SetLabel(buf);
  const std::string chap{StringUtils::Format(g_localizeStrings.Get(25007), title.chapters.size(),
                                             StringUtils::SecondsToTimeString(duration))};
  item->SetLabel2(chap);
  item->m_dwSize = 0;
  item->SetArt("icon", "DefaultVideo.png");

  // Generate streamdetails

  // Populate videoInfo
  VideoStreamInfo videoInfo;
  if (!title.videoStreams.empty())
  {
    const auto& video{title.videoStreams[0]};
    videoInfo.valid = true;
    videoInfo.bitrate = 0;
    switch (video.format)
    {
      case BLURAY_VIDEO_FORMAT_480I:
      case BLURAY_VIDEO_FORMAT_480P:
        videoInfo.height = 480;
        videoInfo.width = 640; // Guess but never displayed
        break;
      case BLURAY_VIDEO_FORMAT_576I:
      case BLURAY_VIDEO_FORMAT_576P:
        videoInfo.height = 576;
        videoInfo.width = 720; // Guess but never displayed
        break;
      case BLURAY_VIDEO_FORMAT_720P:
        videoInfo.height = 720;
        videoInfo.width = 1280; // Guess but never displayed
        break;
      case BLURAY_VIDEO_FORMAT_1080I:
      case BLURAY_VIDEO_FORMAT_1080P:
        videoInfo.height = 1080;
        videoInfo.width = 1920; // Guess but never displayed
        break;
      case BLURAY_VIDEO_FORMAT_2160P:
        videoInfo.height = 2160;
        videoInfo.width = 3840; // Guess but never displayed
        break;
      default:
        videoInfo.height = 0;
        videoInfo.width = 0;
        break;
    }
    switch (video.coding)
    {
      case BLURAY_STREAM_TYPE_VIDEO_MPEG1:
        videoInfo.codecName = "mpeg1";
        break;
      case BLURAY_STREAM_TYPE_VIDEO_MPEG2:
        videoInfo.codecName = "mpeg2";
        break;
      case BLURAY_STREAM_TYPE_VIDEO_VC1:
        videoInfo.codecName = "vc1";
        break;
      case BLURAY_STREAM_TYPE_VIDEO_H264:
        videoInfo.codecName = "h264";
        break;
      case BLURAY_STREAM_TYPE_VIDEO_HEVC:
        videoInfo.codecName = "hevc";
        break;
      default:
        videoInfo.codecName = "";
        break;
    }
    switch (video.aspect)
    {
      case BLURAY_ASPECT_RATIO_4_3:
        videoInfo.videoAspectRatio = 4.0f / 3.0f;
        break;
      case BLURAY_ASPECT_RATIO_16_9:
        videoInfo.videoAspectRatio = 16.0f / 9.0f;
        break;
      default:
        videoInfo.videoAspectRatio = 0.0f;
        break;
    }
    videoInfo.stereoMode = ""; // Not stored in BLURAY_TITLE_INFO
    videoInfo.flags = FLAG_NONE;
    videoInfo.hdrType = StreamHdrType::HDR_TYPE_NONE; // Not stored in BLURAY_TITLE_INFO
    videoInfo.fpsRate = 0; // Not in streamdetails
    videoInfo.fpsScale = 0; // Not in streamdetails

    CVideoInfoTag* info = item->GetVideoInfoTag();
    info->m_streamDetails.SetStreams(videoInfo, static_cast<int>(title.duration / 90000),
                                     AudioStreamInfo{}, SubtitleStreamInfo{});

    for (const auto& audio : title.audioStreams)
    {
      AudioStreamInfo audioInfo;
      audioInfo.valid = true;
      audioInfo.bitrate = 0;
      audioInfo.channels = 0; // Only basic mono/stereo/multichannel is stored in BLURAY_TITLE_INFO

      switch (audio.coding)
      {
        case BLURAY_STREAM_TYPE_AUDIO_AC3:
          audioInfo.codecName = "ac3";
          break;
        case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS:
        case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS_SECONDARY:
          audioInfo.codecName = "eac3";
          break;
        case BLURAY_STREAM_TYPE_AUDIO_LPCM:
          audioInfo.codecName = "pcm";
          break;
        case BLURAY_STREAM_TYPE_AUDIO_DTS:
          audioInfo.codecName = "dts";
          break;
        case BLURAY_STREAM_TYPE_AUDIO_DTSHD:
        case BLURAY_STREAM_TYPE_AUDIO_DTSHD_SECONDARY:
          audioInfo.codecName = "dtshd";
          break;
        case BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER:
          audioInfo.codecName = "dtshd_ma";
          break;
        case BLURAY_STREAM_TYPE_AUDIO_TRUHD:
          audioInfo.codecName = "truehd";
          break;
        default:
          audioInfo.codecName = "";
          break;
      }
      audioInfo.flags = FLAG_NONE;
      audioInfo.language = audio.lang;
      info->m_streamDetails.AddStream(new CStreamDetailAudio(audioInfo));
    }

    // Subtitles
    for (const auto& subtitle : title.pgStreams)
    {
      SubtitleStreamInfo subtitleInfo;
      subtitleInfo.valid = true;
      subtitleInfo.bitrate = 0;
      subtitleInfo.codecDesc = "";
      subtitleInfo.codecName = "";
      subtitleInfo.isExternal = false;
      subtitleInfo.name = "";
      subtitleInfo.flags = FLAG_NONE;
      subtitleInfo.language = subtitle.lang;
      info->m_streamDetails.AddStream(new CStreamDetailSubtitle(subtitleInfo));
    }
  }

  return item;
}

bool CBlurayDirectory::GetPlaylists(GetTitlesJob job, CFileItemList& items, SortTitlesJob sort)
{
  std::vector<PlaylistInfo> playlists;
  int mainPlaylist{-1};

  // See if disc.inf for main playlist
  if (job != GetTitlesJob::GET_TITLES_ALL)
  {
    mainPlaylist = GetMainPlaylistFromDisc();
    if (mainPlaylist != -1)
    {
      // Only main playlist is needed
      PlaylistInfo t{};
      if (!GetPlaylistInfoFromDisc(mainPlaylist, t))
        CLog::LogF(LOGDEBUG, "Unable to get playlist {}", mainPlaylist);
      else
        playlists.emplace_back(t);
    }
  }

  if (playlists.empty())
  {
    CURL url{CURL(URIUtils::AddFileToFolder(m_url.GetHostName(), "BDMV", "PLAYLIST", ""))};
    CDirectory::CHints hints;
    hints.flags = m_flags;
    CFileItemList allTitles;
    if (!CDirectory::GetDirectory(url, allTitles, hints))
      return false;

    // Get information on all playlists
    const std::regex playlistPath("(\\d{5}.mpls)");
    std::smatch playlistMatch;
    for (const auto& title : allTitles)
    {
      const CURL url2{title->GetPath()};
      const std::string filename{URIUtils::GetFileName(url2.GetFileName())};
      if (std::regex_search(filename, playlistMatch, playlistPath))
      {
        const unsigned int playlist{static_cast<unsigned int>(std::stoi(playlistMatch[0]))};

        PlaylistInfo t{};
        if (!GetPlaylistInfoFromDisc(playlist, t))
          CLog::LogF(LOGDEBUG, "Unable to get playlist {}", playlist);
        else
          playlists.emplace_back(t);
      }
    }
  }

  // Remove playlists below minimum duration (default 5 minutes)
  const uint64_t minimumDuration{static_cast<uint64_t>(CServiceBroker::GetSettingsComponent()
                                                           ->GetAdvancedSettings()
                                                           ->m_minimumEpisodePlaylistDuration)};
  std::erase_if(playlists, [&minimumDuration](const PlaylistInfo& playlist)
                { return playlist.duration < minimumDuration; });

  // Remove playlists with no clips
  std::erase_if(playlists, [](const PlaylistInfo& playlist) { return playlist.clips.empty(); });

  // Remove playlists with duplicate clips
  std::erase_if(playlists,
                [](const PlaylistInfo& playlist)
                {
                  std::set<unsigned int> clips;
                  for (const auto& clip : playlist.clips)
                    clips.emplace(clip);
                  return clips.size() < playlist.clips.size();
                });

  // Remove duplicate playlists
  std::set<unsigned int> duplicatePlaylists;
  for (unsigned int i = 0; i < playlists.size() - 1; ++i)
  {
    for (unsigned int j = i + 1; j < playlists.size(); ++j)
    {
      if (playlists[i].audioStreams.size() == playlists[j].audioStreams.size() &&
          playlists[i].pgStreams.size() == playlists[j].pgStreams.size() &&
          playlists[i].chapters.size() == playlists[j].chapters.size() &&
          playlists[i].clips.size() == playlists[j].clips.size() &&
          std::memcmp(&playlists[i].audioStreams, &playlists[j].audioStreams,
                      playlists[i].audioStreams.size() * sizeof(DiscStreamInfo)) == 0 &&
          std::memcmp(&playlists[i].pgStreams, &playlists[j].pgStreams,
                      playlists[i].pgStreams.size() * sizeof(DiscStreamInfo)) == 0 &&
          playlists[i].chapters == playlists[j].chapters &&
          playlists[i].clips == playlists[j].clips)
      {
        duplicatePlaylists.emplace(playlists[j].playlist);
      }
    }
  }
  std::erase_if(playlists,
                [&duplicatePlaylists](const PlaylistInfo& p)
                {
                  return std::ranges::any_of(duplicatePlaylists,
                                             [&p](const unsigned int duplicatePlaylist)
                                             { return p.playlist == duplicatePlaylist; });
                });

  // Now we have curated playlists, find longest (for main title derivation)
  const auto& it{std::ranges::max_element(playlists, {}, &PlaylistInfo::duration)};

  const uint64_t maxDuration{it->duration};
  const unsigned int maxPlaylist{it->playlist};

  // Sort
  // Movies - placing main title - if present - first, then by duration
  // Episodes - by playlist number
  if (sort != SortTitlesJob::SORT_TITLES_NONE)
  {
    std::ranges::sort(playlists,
                      [&sort](const PlaylistInfo& i, const PlaylistInfo& j)
                      {
                        if (sort == SortTitlesJob::SORT_TITLES_MOVIE)
                        {
                          if (i.duration == j.duration)
                            return i.playlist < j.playlist;
                          return i.duration > j.duration;
                        }
                        return i.playlist < j.playlist;
                      });

    const auto& pivot{
        std::ranges::find_if(playlists, [&mainPlaylist](const PlaylistInfo& title)
                             { return title.playlist == static_cast<uint32_t>(mainPlaylist); })};
    if (pivot != playlists.end())
      std::rotate(playlists.begin(), pivot, pivot + 1);
  }

  const uint64_t minDuration{maxDuration * MAIN_TITLE_LENGTH_PERCENT / 100};
  for (const auto& title : playlists)
  {
    if (job == GetTitlesJob::GET_TITLES_ALL ||
        (job == GetTitlesJob::GET_TITLES_MAIN && title.duration >= minDuration) ||
        (job == GetTitlesJob::GET_TITLES_ONE &&
         (title.playlist == static_cast<uint32_t>(mainPlaylist) ||
          (mainPlaylist == -1 && title.playlist == static_cast<uint32_t>(maxPlaylist)))))
    {
      items.Add(GetFileItem(title, title.playlist == static_cast<uint32_t>(mainPlaylist)
                                       ? g_localizeStrings.Get(25004) /* Main Title */
                                       : g_localizeStrings.Get(25005) /* Title */));
    }
  }

  return !items.IsEmpty();
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
  bool success{true};
  if (file == "root")
  {
    success = GetPlaylists(GetTitlesJob::GET_TITLES_MAIN, items, SortTitlesJob::SORT_TITLES_MOVIE);

    // Add all titles and menu options
    CDiscDirectoryHelper::AddRootOptions(m_url, items, m_blurayMenuSupport);
  }
  else if (file == "root/titles")
    success = GetPlaylists(GetTitlesJob::GET_TITLES_ALL, items, SortTitlesJob::SORT_TITLES_MOVIE);
  else if (StringUtils::StartsWith(file, "root/episode"))
  {
    // Get episodes on disc
    const std::vector<CVideoInfoTag> episodesOnDisc{CDiscDirectoryHelper::GetEpisodesOnDisc(url)};

    int season{-1};
    int episode{-1};
    int episodeIndex{-1};
    if (file != "root/episode/all")
    {
      // Get desired episode from path
      CRegExp regex{true, CRegExp::autoUtf8, R"((root\/episode\/)(\d+)\/(\d+))"};
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
    GetPlaylistsInformation(clips, playlists);

    // Get episode playlists
    success = CDiscDirectoryHelper::GetEpisodePlaylists(m_url, items, episodeIndex, episodesOnDisc,
                                                        clips, playlists);

    // Heuristics failed so return all playlists
    if (items.IsEmpty())
      success =
          GetPlaylists(GetTitlesJob::GET_TITLES_ALL, items, SortTitlesJob::SORT_TITLES_EPISODE);

    // Add all titles and menu options
    CDiscDirectoryHelper::AddRootOptions(m_url, items, m_blurayMenuSupport);
  }
  else
  {
    const CURL url2{CURL(URIUtils::GetDiscUnderlyingFile(url))};
    CDirectory::CHints hints;
    hints.flags = m_flags;
    if (!CDirectory::GetDirectory(url2, items, hints))
      return false;

    // Found items will have underlying protocol (eg. udf:// or smb://)
    // in path so add back bluray://
    // (so properly recognised in cache as bluray:// files for CFile:Exists() etc..)
    CURL url3{url};
    for (const auto& item : items)
    {
      const CURL url4{item->GetPath()};
      url3.SetFileName(url4.GetFileName());
      item->SetPath(url3.Get());
    }

    url3.SetFileName("menu");
    const std::shared_ptr<CFileItem> item{std::make_shared<CFileItem>()};
    item->SetPath(url3.Get());
    items.Add(item);
  }

  items.AddSortMethod(SortByTrackNumber, 554,
                      LABEL_MASKS("%L", "%D", "%L", "")); // FileName, Duration | Foldername, empty
  items.AddSortMethod(SortBySize, 553,
                      LABEL_MASKS("%L", "%I", "%L", "%I")); // FileName, Size | Foldername, Size

  return success;
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

  auto fileHandler = CDirectoryFactory::Create(CURL{root});
  if (fileHandler)
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

int CBlurayDirectory::GetMainPlaylistFromDisc() const
{
  const std::string root{m_url.GetHostName()};
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

std::string CBlurayDirectory::GetCachePath() const
{
  if (m_url.Get().empty())
    return m_realPath;
  std::string path{m_url.GetHostName()};
  if (path.empty())
    path = m_url.Get(); // Could be drive letter
  return path;
}

const BLURAY_DISC_INFO* CBlurayDirectory::GetDiscInfo() const
{
  return bd_get_disc_info(m_bd);
}

bool CBlurayDirectory::GetPlaylistInfoFromDisc(unsigned int playlist, PlaylistInfo& playlistInfo)
{
  const std::string path{GetCachePath()};

  // Check cache
  PlaylistInformation p;
  if (!CServiceBroker::GetBlurayDiscCache()->GetPlaylistInfo(path, playlist, p))
  {
    // Retrieve from disc
    if (!ReadMPLS(playlist, p))
      return false;

    // Cache and return
    CServiceBroker::GetBlurayDiscCache()->SetPlaylistInfo(path, playlist, p);
  }

  // Parse PlaylistInformation into PlayInfo
  playlistInfo.playlist = p.playlist;
  playlistInfo.duration = p.duration / 1000;
  playlistInfo.chapters.reserve(p.chapters.size());
  for (const ChapterInformation& chapter : p.chapters)
    playlistInfo.chapters.emplace_back(chapter.start);
  playlistInfo.clips.reserve(p.clips.size());
  for (const ClipInformation& clip : p.clips)
  {
    playlistInfo.clips.emplace_back(clip.clip);
    playlistInfo.clipDuration[clip.clip] = clip.duration / 1000;
  }
  if (!p.clips.empty() && !p.clips[0].programs.empty())
  {
    for (const StreamInformation& stream : p.clips[0].programs[0].streams)
    {
      switch (stream.coding)
      {
        case BLURAY_STREAM_TYPE_VIDEO_MPEG1:
        case BLURAY_STREAM_TYPE_VIDEO_MPEG2:
        case BLURAY_STREAM_TYPE_VIDEO_VC1:
        case BLURAY_STREAM_TYPE_VIDEO_H264:
        case BLURAY_STREAM_TYPE_VIDEO_HEVC:
          playlistInfo.videoStreams.emplace_back(DiscStreamInfo{.coding = stream.coding,
                                                                .format = stream.format,
                                                                .rate = stream.rate,
                                                                .aspect = stream.aspect,
                                                                .lang = ""});
          break;
        case BLURAY_STREAM_TYPE_AUDIO_MPEG1:
        case BLURAY_STREAM_TYPE_AUDIO_MPEG2:
        case BLURAY_STREAM_TYPE_AUDIO_LPCM:
        case BLURAY_STREAM_TYPE_AUDIO_AC3:
        case BLURAY_STREAM_TYPE_AUDIO_DTS:
        case BLURAY_STREAM_TYPE_AUDIO_TRUHD:
        case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS:
        case BLURAY_STREAM_TYPE_AUDIO_DTSHD:
        case BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER:
          playlistInfo.audioStreams.emplace_back(DiscStreamInfo{.coding = stream.coding,
                                                                .format = stream.format,
                                                                .rate = stream.rate,
                                                                .aspect = stream.aspect,
                                                                .lang = stream.language});
          break;
        case BLURAY_STREAM_TYPE_SUB_PG:
        case BLURAY_STREAM_TYPE_SUB_TEXT:
          playlistInfo.pgStreams.emplace_back(DiscStreamInfo{.coding = stream.coding,
                                                             .format = stream.format,
                                                             .rate = stream.rate,
                                                             .aspect = stream.aspect,
                                                             .lang = stream.language});
          break;
        default:
          break;
      }
    }
  }
  return true;
}

bool CBlurayDirectory::ReadMPLS(unsigned int playlist, PlaylistInformation& playlistInformation)
{
  const std::string path{m_url.GetHostName()};
  const std::string playlistFile{URIUtils::AddFileToFolder(
      path, "BDMV", "PLAYLIST", StringUtils::Format("{:05}.mpls", playlist))};
  CFile file;
  if (!file.Open(playlistFile))
    return false;

  const int64_t size{file.GetLength()};
  std::vector<char> buffer;
  buffer.resize(size);
  ssize_t read = file.Read(buffer.data(), size);
  file.Close();

  if (read == size)
    return ParseMPLS(buffer, playlistInformation, playlist);
  return false;
}

// Structure of an MPLS File:
//
// Header: Contains general information about the playlist.
//
// PlayItems: Each PlayItem corresponds to a specific .m2ts file and includes details like
// In Time, Out Time, and System Time Clock ID for synchronization.
//
// SubPath: Optional, used for features like Picture-in-Picture (PiP) or secondary audio.

bool CBlurayDirectory::ParseMPLS(const std::vector<char>& buffer,
                                 PlaylistInformation& playlistInformation,
                                 unsigned int playlist)
{
  // Check size
  if (buffer.size() < 40)
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - header too small");
    return false;
  }

  // Check header
  const std::string header{CDiscDirectoryHelper::GetString(buffer, 0, 4)};
  const std::string version{CDiscDirectoryHelper::GetString(buffer, 4, 4)};
  if (header != "MPLS")
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS header");
    return false;
  }
  CLog::LogF(LOGDEBUG, "*** Valid MPLS header for playlist {} version {}", playlist, version);

  playlistInformation.playlist = playlist;
  playlistInformation.version = version;

  const unsigned int playlistPosition{CDiscDirectoryHelper::GetDWord(buffer, 8)};
  const unsigned int playlistMarkPosition{CDiscDirectoryHelper::GetDWord(buffer, 12)};
  //const unsigned int extensionPosition{CDiscDirectoryHelper::GetDWord(buffer, 16)};

  // AppInfoPlaylist
  unsigned int offset{40};
  const unsigned int appInfoSize{CDiscDirectoryHelper::GetDWord(buffer, offset)};
  if (buffer.size() < appInfoSize + offset)
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - too small for AppInfoPlaylist");
    return false;
  }

  BLURAY_PLAYBACK_TYPE playbackType{CDiscDirectoryHelper::GetByte(buffer, offset + 5)};
  unsigned int playbackCount{0};
  if (playbackType == BLURAY_PLAYBACK_TYPE::RANDOM || playbackType == BLURAY_PLAYBACK_TYPE::SHUFFLE)
    playbackCount = CDiscDirectoryHelper::GetWord(buffer, offset + 6);
  playlistInformation.playbackType = playbackType;
  playlistInformation.playbackCount = playbackCount;

  // Playlist
  offset = playlistPosition;
  const unsigned int playlistSize{CDiscDirectoryHelper::GetDWord(buffer, offset)};
  if (buffer.size() < playlistSize + offset)
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - too small for Playlist");
    return false;
  }
  const unsigned int numPlayItems{CDiscDirectoryHelper::GetWord(buffer, offset + 6)};
  const unsigned int numSubPaths{CDiscDirectoryHelper::GetWord(buffer, offset + 8)};
  offset += 10;

  playlistInformation.playItems.reserve(numPlayItems);
  for (unsigned int i = 0; i < numPlayItems; ++i)
  {
    PlayItemInformation playItem;
    if (!ParsePlayItem(buffer, offset, playItem))
      return false;
    playlistInformation.playItems.emplace_back(playItem);
  }

  // Calculate duration
  unsigned int duration{0};
  for (const auto& playItem : playlistInformation.playItems)
    duration += playItem.outTime - playItem.inTime;
  playlistInformation.duration = duration;
  CLog::LogF(LOGDEBUG, "Playlist duration {}",
             fmt::format("{:%H:%M:%S}", std::chrono::milliseconds(duration)));

  // Process clips
  for (const auto& playItem : playlistInformation.playItems)
  {
    for (const auto& clip : playItem.angleClips)
    {
      const auto& it = m_clips.find(clip.clip);
      if (it == m_clips.end()) // not in local cache
      {
        ClipInformation clipInformation;
        if (!ReadCLPI(clip.clip, clipInformation))
        {
          CLog::LogF(LOGDEBUG, "Cannot read clip {} information", clip.clip);
          return false;
        }
        playlistInformation.clips.emplace_back(clipInformation);
        m_clips[clip.clip] = clipInformation;
      }
      else // is in local cache
        playlistInformation.clips.emplace_back(it->second);
    }
  }

  if (numSubPaths > 0)
  {
    for (unsigned int i = 0; i < numSubPaths; ++i)
    {
      SubPathInformation subPathInformation;

      const BLURAY_SUBPATH_TYPE type{CDiscDirectoryHelper::GetByte(buffer, offset + 5)};
      const unsigned int flags{CDiscDirectoryHelper::GetWord(buffer, offset + 6)};
      subPathInformation.type = type;
      subPathInformation.repeat = (flags & 1) == 1;

      const unsigned int numSubPlayItems{CDiscDirectoryHelper::GetByte(buffer, offset + 9)};
      offset += 10;
      if (numSubPlayItems > 0)
      {
        playlistInformation.subPlayItems.reserve(numSubPlayItems);
        for (unsigned int j = 0; j < numSubPlayItems; ++j)
        {
          SubPlayItemInformation subPlayItem;
          if (!ParseSubPlayItem(buffer, offset, subPlayItem))
            return false;
          playlistInformation.subPlayItems.emplace_back(subPlayItem);
        }
      }
    }
  }

  // Parse PlayListMark
  offset = playlistMarkPosition;
  const unsigned int playlistMarkSize{CDiscDirectoryHelper::GetDWord(buffer, offset)};
  if (buffer.size() < playlistMarkSize + offset)
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - too small for PlayListMark");
    return false;
  }

  const unsigned int numPlaylistMarks{CDiscDirectoryHelper::GetWord(buffer, offset + 4)};
  if (buffer.size() < (numPlaylistMarks * 14) + offset + 6)
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - too small for PlayListMark");
    return false;
  }

  offset += 6;
  playlistInformation.playlistMarks.reserve(numPlaylistMarks);
  for (unsigned int i = 0; i < numPlaylistMarks; ++i)
  {

    PlaylistMarkInformation playlistMark{
        .markType =
            static_cast<BLURAY_MARK_TYPE>(CDiscDirectoryHelper::GetByte(buffer, offset + 1)),
        .playItemReference = CDiscDirectoryHelper::GetWord(buffer, offset + 2),
        .time = CDiscDirectoryHelper::GetDWord(buffer, offset + 4) / 45,
        .elementaryStreamPacketIdentifier = CDiscDirectoryHelper::GetWord(buffer, offset + 8),
        .duration = CDiscDirectoryHelper::GetDWord(buffer, offset + 10)};
    playlistInformation.playlistMarks.emplace_back(playlistMark);
    offset += 14;
  }

  // Update clip timings from corresponding playItems
  for (unsigned int i = 0, startTime = 0; i < playlistInformation.playItems.size(); ++i)
  {
    const auto& playItem = playlistInformation.playItems[i];
    auto& clip = playlistInformation.clips[i];
    clip.duration = playItem.outTime - playItem.inTime;
    clip.time = startTime;
    startTime += clip.duration;
  }

  // Update playMark timings
  int prevChapter{-1};
  for (int i = 0; i < static_cast<int>(playlistInformation.playlistMarks.size()); ++i)
  {
    auto& playlistMark{playlistInformation.playlistMarks[i]};

    // Get referenced playItem and clip to calculate playMark time
    const auto& clip = playlistInformation.clips[playlistMark.playItemReference];
    const auto& playItem = playlistInformation.playItems[playlistMark.playItemReference];
    playlistMark.time = clip.time + playlistMark.time - playItem.inTime;

    if (playlistMark.markType == BLURAY_MARK_TYPE::ENTRY) // chapter
    {
      if (prevChapter >= 0)
      {
        auto& prevMark{playlistInformation.playlistMarks[prevChapter]};
        if (prevMark.duration == 0)
          prevMark.duration = playlistMark.time - prevMark.time;
      }
      prevChapter = i;
    }
  }
  if (prevChapter >= 0 && playlistInformation.playlistMarks[prevChapter].duration == 0)
  {
    auto& prevMark{playlistInformation.playlistMarks[prevChapter]};
    prevMark.duration = playlistInformation.duration - prevMark.time;
  }

  // Derive chapters from playMarks
  for (unsigned int chapter{1}, start{0};
       const auto& playlistMark : playlistInformation.playlistMarks)
  {
    if (playlistMark.markType == BLURAY_MARK_TYPE::ENTRY)
    {
      ChapterInformation chapterInformation{
          .chapter = chapter, .start = start, .duration = playlistMark.duration};
      playlistInformation.chapters.emplace_back(chapterInformation);
      CLog::LogF(LOGDEBUG, "Chapter {} start {} duration {} end {}", chapter,
                 fmt::format("{:%H:%M:%S}", std::chrono::milliseconds(start)),
                 fmt::format("{:%H:%M:%S}", std::chrono::milliseconds(chapterInformation.duration)),
                 fmt::format("{:%H:%M:%S}",
                             std::chrono::milliseconds(start + chapterInformation.duration)));
      start += chapterInformation.duration;
      ++chapter;
    }
  }

  return true;
}

bool CBlurayDirectory::ParsePlayItem(const std::vector<char>& buffer,
                                     unsigned int& offset,
                                     PlayItemInformation& playItem)
{
  const unsigned int saveOffset{offset};
  const unsigned int length{CDiscDirectoryHelper::GetWord(buffer, offset)};
  if (length < 18)
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - Playitem too small");
    return false;
  }
  if (buffer.size() < length + offset)
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - too small for Playitem");
    return false;
  }

  std::string clipId{CDiscDirectoryHelper::GetString(buffer, offset + 2, 5)};
  std::string codecId{CDiscDirectoryHelper::GetString(buffer, offset + 7, 4)};
  if (codecId != "M2TS" && codecId != "FMTS")
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - invalid PlayItem codec identifier - {}", codecId);
    return false;
  }

  unsigned int flags{CDiscDirectoryHelper::GetWord(buffer, offset + 11)};
  const bool isMultiAngle{((flags >> 4) & 1) == 1};
  const BLURAY_CONNECTION connectionCondition{flags & 0x0F};
  if (connectionCondition != BLURAY_CONNECTION::SEAMLESS &&
      connectionCondition != BLURAY_CONNECTION::NONSEAMLESS &&
      connectionCondition != BLURAY_CONNECTION::BRANCHING)
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - invalid PlayItem connection condition - {}",
               connectionCondition);
    return false;
  }
  playItem.isMultiAngle = isMultiAngle;
  playItem.connectionCondition = connectionCondition;

  const unsigned int inTime{CDiscDirectoryHelper::GetDWord(buffer, offset + 14) / 45};
  const unsigned int outTime{CDiscDirectoryHelper::GetDWord(buffer, offset + 18) / 45};
  playItem.inTime = inTime;
  playItem.outTime = outTime;

  flags = CDiscDirectoryHelper::GetByte(buffer, offset + 30);
  const bool randomAccessFlag{(flags & 0x80) == 0x80};
  const unsigned int stillMode{CDiscDirectoryHelper::GetByte(buffer, offset + 31)};
  unsigned int stillTime{0};
  if (stillMode == BLURAY_STILL_TIME)
    stillTime = CDiscDirectoryHelper::GetWord(buffer, offset + 32);
  playItem.randomAccessFlag = randomAccessFlag;
  playItem.stillMode = stillMode;
  playItem.stillTime = stillTime;

  unsigned int angleCount{1};
  if (isMultiAngle)
  {
    angleCount = CDiscDirectoryHelper::GetByte(buffer, offset + 34);
    offset += 36;
  }
  else
    offset += 34;

  CLog::LogF(LOGDEBUG,
             "PlayItem entry - clip id {}, codec id {}, in time {}, out time {}, still mode "
             "{} (still time {}), angles {}",
             clipId, codecId, fmt::format("{:%H:%M:%S}", std::chrono::milliseconds(inTime)),
             fmt::format("{:%H:%M:%S}", std::chrono::milliseconds(outTime)), stillMode, stillTime,
             angleCount);

  // First/only angle
  ClipInformation angleInformation;
  angleInformation.clip = std::stoi(clipId);
  angleInformation.codec = codecId;
  playItem.angleClips.emplace_back(angleInformation);

  // Get additional angles (if any)
  for (unsigned int j = 1; j < angleCount; ++j)
  {
    ClipInformation additionalAngleInformation;
    const std::string angleClipId{CDiscDirectoryHelper::GetString(buffer, offset, 5)};
    const std::string angleCodecId{CDiscDirectoryHelper::GetString(buffer, offset + 5, 4)};
    if (angleCodecId != "M2TS" && angleCodecId != "FMTS")
    {
      CLog::LogF(LOGDEBUG, "Invalid MPLS - invalid PlayItem angle {} codec identifier - {}", j,
                 angleCodecId);
      return false;
    }
    CLog::LogF(LOGDEBUG, "  Additional angle {} - clip id {}, codec id {}", j, angleClipId,
               angleCodecId);

    additionalAngleInformation.clip = std::stoi(angleClipId);
    additionalAngleInformation.codec = angleCodecId;
    playItem.angleClips.emplace_back(additionalAngleInformation);
    offset += 10;
  }

  // Parse stream number table
  const unsigned int stnLength{CDiscDirectoryHelper::GetWord(buffer, offset)};
  if (buffer.size() < stnLength + offset)
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - too small for Stream Number Table");
    return false;
  }

  const unsigned int numVideoStreams{CDiscDirectoryHelper::GetByte(buffer, offset + 4)};
  const unsigned int numAudioStreams{CDiscDirectoryHelper::GetByte(buffer, offset + 5)};
  const unsigned int numPresentationGraphicStreams{
      CDiscDirectoryHelper::GetByte(buffer, offset + 6)};
  const unsigned int numInteractiveGraphicStreams{
      CDiscDirectoryHelper::GetByte(buffer, offset + 7)};
  const unsigned int numSecondaryVideoStreams{CDiscDirectoryHelper::GetByte(buffer, offset + 8)};
  const unsigned int numSecondaryAudioStreams{CDiscDirectoryHelper::GetByte(buffer, offset + 9)};
  const unsigned int numPictureInPictureSubtitleStreams{
      CDiscDirectoryHelper::GetByte(buffer, offset + 10)};
  const unsigned int numDolbyVisionStreams{CDiscDirectoryHelper::GetByte(buffer, offset + 11)};

  CLog::LogF(LOGDEBUG,
             " Stream number table - video {}, audio {}, presentation graphic (subtitle) {}, "
             "interactive graphic {}, "
             "secondary video {}, secondary audio {}, PIP subtitle {}, dolby vision {}",
             numVideoStreams, numAudioStreams, numPresentationGraphicStreams,
             numInteractiveGraphicStreams, numSecondaryVideoStreams, numSecondaryAudioStreams,
             numPictureInPictureSubtitleStreams, numDolbyVisionStreams);

  offset += 16;

  playItem.videoStreams.reserve(numVideoStreams);
  for (unsigned int k = 0; k < numVideoStreams; ++k)
  {
    playItem.videoStreams.emplace_back(ParseStream(buffer, offset, STREAM_TYPE::VIDEO_STREAM));
  }

  playItem.audioStreams.reserve(numAudioStreams);
  for (unsigned int k = 0; k < numAudioStreams; ++k)
  {
    playItem.audioStreams.emplace_back(ParseStream(buffer, offset, STREAM_TYPE::AUDIO_STREAM));
  }

  playItem.presentationGraphicStreams.reserve(numPresentationGraphicStreams);
  for (unsigned int k = 0; k < numPresentationGraphicStreams; ++k)
  {
    playItem.presentationGraphicStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::PRESENTATION_GRAPHIC_STREAM));
  }

  playItem.interactiveGraphicStreams.reserve(numInteractiveGraphicStreams);
  for (unsigned int k = 0; k < numInteractiveGraphicStreams; ++k)
  {
    playItem.interactiveGraphicStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::INTERACTIVE_GRAPHIC_STREAM));
  }

  playItem.secondaryAudioStreams.reserve(numSecondaryAudioStreams);
  for (unsigned int k = 0; k < numSecondaryAudioStreams; ++k)
  {
    playItem.secondaryAudioStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::SECONDARY_AUDIO_STREAM));
  }

  playItem.secondaryVideoStreams.reserve(numSecondaryVideoStreams);
  for (unsigned int k = 0; k < numSecondaryVideoStreams; ++k)
  {
    playItem.secondaryVideoStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::SECONDARY_VIDEO_STREAM));
  }

  playItem.dolbyVisionStreams.reserve(numDolbyVisionStreams);
  for (unsigned int k = 0; k < numDolbyVisionStreams; ++k)
  {
    playItem.dolbyVisionStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::DOLBY_VISION_STREAM));
  }

  offset = saveOffset + length + 2;
  return true;
}

StreamInformation CBlurayDirectory::ParseStream(const std::vector<char>& buffer,
                                                unsigned int& offset,
                                                STREAM_TYPE streamType)
{
  StreamInformation streamInformation;
  unsigned int length{CDiscDirectoryHelper::GetByte(buffer, offset)};

  const BLURAY_STREAM_TYPE type{CDiscDirectoryHelper::GetByte(buffer, offset + 1)};
  streamInformation.type = type;
  switch (type)
  {
    case BLURAY_STREAM_TYPE::PLAYITEM: // a stream of the clip used by the PlayItem
    {
      streamInformation.packetIdentifier = CDiscDirectoryHelper::GetWord(buffer, offset + 2);
      break;
    }
    case BLURAY_STREAM_TYPE::
        SUBPATH: // a stream of the clip used by a SubPath (types 2, 3, 4, 5, 6, 8 or 9)
    {
      streamInformation.subpathId = CDiscDirectoryHelper::GetWord(buffer, offset + 2);
      streamInformation.subclipId = CDiscDirectoryHelper::GetWord(buffer, offset + 3);
      streamInformation.packetIdentifier = CDiscDirectoryHelper::GetWord(buffer, offset + 4);
      break;
    }
    case BLURAY_STREAM_TYPE::
        SUBPATH_INMUX_SYNCHRONOUS_PIP: // a stream of the clip used by a SubPath (type 7)
    case BLURAY_STREAM_TYPE::
        SUBPATH_DOLBY_VISION_LAYER: // a stream of the clip used by a SubPath (type 10)
    {
      streamInformation.subpathId = CDiscDirectoryHelper::GetWord(buffer, offset + 2);
      streamInformation.packetIdentifier = CDiscDirectoryHelper::GetWord(buffer, offset + 3);
      break;
    }
  }
  offset += length + 1;

  length = CDiscDirectoryHelper::GetByte(buffer, offset);
  const unsigned int coding{CDiscDirectoryHelper::GetByte(buffer, offset + 1)};
  streamInformation.coding = coding;
  switch (coding)
  {
    case BLURAY_STREAM_TYPE_VIDEO_MPEG1:
    case BLURAY_STREAM_TYPE_VIDEO_MPEG2:
    case BLURAY_STREAM_TYPE_VIDEO_H264:
    case BLURAY_STREAM_TYPE_VIDEO_HEVC:
    case BLURAY_STREAM_TYPE_VIDEO_VC1:
    {
      unsigned int flag{CDiscDirectoryHelper::GetByte(buffer, offset + 2)};
      streamInformation.format = (flag & 0xF0) >> 4;
      streamInformation.rate = flag & 0x0F;
      if (coding == BLURAY_STREAM_TYPE_VIDEO_HEVC)
      {
        flag = CDiscDirectoryHelper::GetByte(buffer, offset + 3);
        streamInformation.dynamicRangeType = (flag & 0xF0) >> 4;
        streamInformation.colorSpace = flag & 0x0F;
        flag = CDiscDirectoryHelper::GetByte(buffer, offset + 4);
        streamInformation.copyRestricted = (flag & 0x80) == 0x80;
        streamInformation.HDRPlus = (flag & 0x40) == 0x40;
      }
      break;
    }
    case BLURAY_STREAM_TYPE_AUDIO_MPEG1:
    case BLURAY_STREAM_TYPE_AUDIO_MPEG2:
    case BLURAY_STREAM_TYPE_AUDIO_LPCM:
    case BLURAY_STREAM_TYPE_AUDIO_AC3:
    case BLURAY_STREAM_TYPE_AUDIO_DTS:
    case BLURAY_STREAM_TYPE_AUDIO_TRUHD:
    case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS:
    case BLURAY_STREAM_TYPE_AUDIO_DTSHD:
    case BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER:
    case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS_SECONDARY:
    case BLURAY_STREAM_TYPE_AUDIO_DTSHD_SECONDARY:
    {
      const unsigned int flag{CDiscDirectoryHelper::GetByte(buffer, offset + 2)};
      streamInformation.format = (flag & 0xF0) >> 4;
      streamInformation.rate = flag & 0x0F;
      streamInformation.language = CDiscDirectoryHelper::GetString(buffer, offset + 3, 3);
      break;
    }
    case BLURAY_STREAM_TYPE_SUB_PG:
    case BLURAY_STREAM_TYPE_SUB_IG:
    {
      streamInformation.language = CDiscDirectoryHelper::GetString(buffer, offset + 2, 3);
      break;
    }
    case BLURAY_STREAM_TYPE_SUB_TEXT:
    {
      streamInformation.characterEncoding = CDiscDirectoryHelper::GetByte(buffer, offset + 2);
      streamInformation.language = CDiscDirectoryHelper::GetString(buffer, offset + 3, 3);
      break;
    }
    default:
      break;
  }
  offset += length + 1;

  switch (streamType)
  {
    case STREAM_TYPE::SECONDARY_AUDIO_STREAM:
    {
      const unsigned int numAudioReferences{CDiscDirectoryHelper::GetByte(buffer, offset)};
      streamInformation.secondaryAudio_audioReferences.reserve(numAudioReferences);
      for (unsigned int i = 0; i < numAudioReferences; ++i)
      {
        streamInformation.secondaryAudio_audioReferences.emplace_back(
            CDiscDirectoryHelper::GetByte(buffer, offset + 2 + i));
      }
      offset += numAudioReferences + 2 + (numAudioReferences % 2);
      break;
    }
    case STREAM_TYPE::SECONDARY_VIDEO_STREAM:
    {
      const unsigned int numAudioReferences{CDiscDirectoryHelper::GetByte(buffer, offset)};
      streamInformation.secondaryVideo_audioReferences.reserve(numAudioReferences);
      for (unsigned int i = 0; i < numAudioReferences; ++i)
      {
        streamInformation.secondaryVideo_audioReferences.emplace_back(
            CDiscDirectoryHelper::GetByte(buffer, offset + 2 + i));
      }
      offset += numAudioReferences + 2 + (numAudioReferences % 2);
      const unsigned int numPGReferences{CDiscDirectoryHelper::GetByte(buffer, offset)};
      streamInformation.secondaryVideo_audioReferences.reserve(numPGReferences);
      for (unsigned int i = 0; i < numPGReferences; ++i)
      {
        streamInformation.secondaryVideo_presentationGraphicReferences.emplace_back(
            CDiscDirectoryHelper::GetByte(buffer, offset + 2 + i));
      }
      offset += numPGReferences + 2 + (numPGReferences % 2);
      break;
    }
    case STREAM_TYPE::VIDEO_STREAM:
    case STREAM_TYPE::AUDIO_STREAM:
    case STREAM_TYPE::PRESENTATION_GRAPHIC_STREAM:
    case STREAM_TYPE::INTERACTIVE_GRAPHIC_STREAM:
    case STREAM_TYPE::PICTURE_IN_PICTURE_SUBTITLE_STREAM:
    case STREAM_TYPE::DOLBY_VISION_STREAM:
      break;
  }

  CLog::LogF(LOGDEBUG, "  Stream - type {}, coding 0x{}", streamInformation.type,
             CDiscDirectoryHelper::HexToString(streamInformation.coding, 2));

  return streamInformation;
}

bool CBlurayDirectory::ParseSubPlayItem(const std::vector<char>& buffer,
                                        unsigned int& offset,
                                        SubPlayItemInformation& subPlayItemInformation)
{
  const unsigned int saveOffset{offset};
  unsigned int length{CDiscDirectoryHelper::GetWord(buffer, offset)};
  if (length < 24)
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - SubPlayItem too small");
    return false;
  }
  if (buffer.size() < length + offset)
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - too small for SubPlayItem");
    return false;
  }

  std::string clipId{CDiscDirectoryHelper::GetString(buffer, offset + 2, 5)};
  std::string codecId{CDiscDirectoryHelper::GetString(buffer, offset + 7, 4)};
  if (codecId != "M2TS" && codecId != "FMTS")
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - invalid PlayItem codec identifier - {}", codecId);
    return false;
  }

  unsigned int flags{CDiscDirectoryHelper::GetDWord(buffer, offset + 11)};
  const bool isMultiClip{(flags & 1) == 1};
  const BLURAY_CONNECTION connectionCondition{(flags >> 1) & 0x0F};
  if (connectionCondition != BLURAY_CONNECTION::SEAMLESS &&
      connectionCondition != BLURAY_CONNECTION::NONSEAMLESS &&
      connectionCondition != BLURAY_CONNECTION::BRANCHING)
  {
    CLog::LogF(LOGDEBUG, "Invalid MPLS - invalid PlayItem connection condition - {}",
               connectionCondition);
    return false;
  }
  subPlayItemInformation.isMultiClip = isMultiClip;
  subPlayItemInformation.connectionCondition = connectionCondition;

  const unsigned int inTime{CDiscDirectoryHelper::GetDWord(buffer, offset + 16) / 45};
  const unsigned int outTime{CDiscDirectoryHelper::GetDWord(buffer, offset + 20) / 45};
  subPlayItemInformation.inTime = inTime;
  subPlayItemInformation.outTime = outTime;

  const unsigned int syncPlayItemId{CDiscDirectoryHelper::GetWord(buffer, offset + 24)};
  subPlayItemInformation.syncPlayItemId = syncPlayItemId;

  unsigned int numClips{1};
  if (isMultiClip)
  {
    numClips = CDiscDirectoryHelper::GetByte(buffer, offset + 26);
    offset += 27;
  }
  else
    offset += 26;

  CLog::LogF(LOGDEBUG,
             " SubPlayItem entry - clip id {}, codec id {}, in time {}, out time {}, clips {}",
             clipId, codecId, fmt::format("{:%H:%M:%S}", std::chrono::milliseconds(inTime)),
             fmt::format("{:%H:%M:%S}", std::chrono::milliseconds(outTime)), numClips);

  // First/only clip
  ClipInformation clipInformation;
  clipInformation.clip = std::stoi(clipId);
  clipInformation.codec = codecId;
  subPlayItemInformation.clips.emplace_back(clipInformation);

  // Get additional clips (if any)
  for (unsigned int j = 1; j < numClips; ++j)
  {
    ClipInformation additionalClipInformation;
    const std::string additionalClipId{CDiscDirectoryHelper::GetString(buffer, offset, 5)};
    const std::string additionalCodecId{CDiscDirectoryHelper::GetString(buffer, offset + 5, 4)};
    if (additionalCodecId != "M2TS" && additionalCodecId != "FMTS")
    {
      CLog::LogF(LOGDEBUG, "Invalid MPLS - invalid SubPlayItem clip {} codec identifier - {}", j,
                 additionalCodecId);
      return false;
    }
    CLog::LogF(LOGDEBUG, "  Additional clip {} - clip id {}, codec id {}", j, additionalClipId,
               additionalCodecId);

    additionalClipInformation.clip = std::stoi(additionalClipId);
    additionalClipInformation.codec = additionalCodecId;
    subPlayItemInformation.clips.emplace_back(additionalClipInformation);
    offset += 10;
  }

  offset = saveOffset + length + 2;

  return true;
}

bool CBlurayDirectory::ReadCLPI(unsigned int clip, ClipInformation& clipInformation) const
{
  const std::string path{m_url.GetHostName()};
  const std::string clipFile{
      URIUtils::AddFileToFolder(path, "BDMV", "CLIPINF", StringUtils::Format("{:05}.clpi", clip))};
  CFile file;
  if (!file.Open(clipFile))
    return false;

  const int64_t size{file.GetLength()};
  std::vector<char> buffer;
  buffer.resize(size);
  ssize_t read = file.Read(buffer.data(), size);
  file.Close();

  if (read == size)
    return ParseCLPI(buffer, clipInformation, clip);
  return false;
}

bool CBlurayDirectory::ParseCLPI(const std::vector<char>& buffer,
                                 ClipInformation& clipInformation,
                                 unsigned int clip)
{
  // Check size
  if (buffer.size() < 28)
  {
    CLog::LogF(LOGDEBUG, "Invalid CLPI - header too small");
    return false;
  }

  // Check header
  const std::string header{CDiscDirectoryHelper::GetString(buffer, 0, 4)};
  const std::string version{CDiscDirectoryHelper::GetString(buffer, 4, 4)};
  if (header != "HDMV")
  {
    CLog::LogF(LOGDEBUG, "Invalid CLPI header");
    return false;
  }
  CLog::LogF(LOGDEBUG, "Valid CLPI header for clip {} header version {}", clip, version);

  clipInformation.version = version;
  clipInformation.clip = clip;

  //const unsigned int sequenceInformationStartAddress{CDiscDirectoryHelper::GetDWord(buffer, 8)};
  const unsigned int programInformationStartAddress{CDiscDirectoryHelper::GetDWord(buffer, 12)};
  //const unsigned int CPIStartAddress{CDiscDirectoryHelper::GetDWord(buffer, 16)};
  //const unsigned int clipMarkStartAddress{CDiscDirectoryHelper::GetDWord(buffer, 20)};
  //const unsigned int extensionDataStartAddress{CDiscDirectoryHelper::GetDWord(buffer, 24)};

  unsigned int offset{programInformationStartAddress};
  const unsigned int length{CDiscDirectoryHelper::GetDWord(buffer, offset)};
  if (buffer.size() < length + offset)
  {
    CLog::LogF(LOGDEBUG, "Invalid CLPI - too small for Program Information");
    return false;
  }

  const unsigned int numPrograms{CDiscDirectoryHelper::GetByte(buffer, offset + 5)};
  offset += 6;
  clipInformation.programs.reserve(numPrograms);
  for (unsigned int i = 0; i < numPrograms; ++i)
  {
    ProgramInformation programInformation;
    programInformation.spnProgramSequenceStart = CDiscDirectoryHelper::GetDWord(buffer, offset);
    programInformation.programId = CDiscDirectoryHelper::GetWord(buffer, offset + 4);
    const unsigned int numStreams{CDiscDirectoryHelper::GetByte(buffer, offset + 6)};
    programInformation.numGroups = CDiscDirectoryHelper::GetByte(buffer, offset + 7);
    offset += 8;

    CLog::LogF(LOGDEBUG, " Program {}", i);

    programInformation.streams.reserve(numStreams);
    for (unsigned int j = 0; j < numStreams; ++j)
    {
      StreamInformation streamInformation;
      streamInformation.packetIdentifier = CDiscDirectoryHelper::GetWord(buffer, offset);
      offset += 2;

      const unsigned int streamLength{CDiscDirectoryHelper::GetByte(buffer, offset)};
      const unsigned int coding{CDiscDirectoryHelper::GetByte(buffer, offset + 1)};
      streamInformation.coding = coding;
      switch (coding)
      {
        case BLURAY_STREAM_TYPE_VIDEO_MPEG1:
        case BLURAY_STREAM_TYPE_VIDEO_MPEG2:
        case BLURAY_STREAM_TYPE_VIDEO_H264:
        case BLURAY_STREAM_TYPE_VIDEO_HEVC:
        case BLURAY_STREAM_TYPE_VIDEO_VC1:
        {
          const unsigned int flag{CDiscDirectoryHelper::GetDWord(buffer, offset + 2)};
          streamInformation.format = (flag & 0xF0000000) >> 28;
          streamInformation.rate = (flag & 0x0F000000) >> 24;
          streamInformation.aspect = (flag & 0x00F00000) >> 20;
          streamInformation.outOfMux = (flag & 0x00020000) == 0x20000;
          if (coding == BLURAY_STREAM_TYPE_VIDEO_HEVC)
          {
            streamInformation.copyRestricted = (flag & 0x00010000) == 0x10000;
            streamInformation.dynamicRangeType = (flag & 0x0000F000) >> 12;
            streamInformation.colorSpace = (flag & 0x00000F00) >> 8;
            streamInformation.HDRPlus = (flag & 0x00000080) == 0x80;
          }
          break;
        }
        case BLURAY_STREAM_TYPE_AUDIO_MPEG1:
        case BLURAY_STREAM_TYPE_AUDIO_MPEG2:
        case BLURAY_STREAM_TYPE_AUDIO_LPCM:
        case BLURAY_STREAM_TYPE_AUDIO_AC3:
        case BLURAY_STREAM_TYPE_AUDIO_DTS:
        case BLURAY_STREAM_TYPE_AUDIO_TRUHD:
        case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS:
        case BLURAY_STREAM_TYPE_AUDIO_DTSHD:
        case BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER:
        case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS_SECONDARY:
        case BLURAY_STREAM_TYPE_AUDIO_DTSHD_SECONDARY:
        {
          const unsigned int flag{CDiscDirectoryHelper::GetByte(buffer, offset + 2)};
          streamInformation.format = (flag & 0xF0) >> 4;
          streamInformation.rate = flag & 0x0F;
          streamInformation.language = CDiscDirectoryHelper::GetString(buffer, offset + 3, 3);
          break;
        }
        case BLURAY_STREAM_TYPE_SUB_PG:
        case BLURAY_STREAM_TYPE_SUB_IG:
        {
          streamInformation.language = CDiscDirectoryHelper::GetString(buffer, offset + 2, 3);
          break;
        }
        case BLURAY_STREAM_TYPE_SUB_TEXT:
        {
          streamInformation.characterEncoding = CDiscDirectoryHelper::GetByte(buffer, offset + 2);
          streamInformation.language = CDiscDirectoryHelper::GetString(buffer, offset + 3, 3);
          break;
        }
        default:
          break;
      }
      programInformation.streams.emplace_back(streamInformation);

      CLog::LogF(LOGDEBUG, "  Stream - coding 0x{}",
                 CDiscDirectoryHelper::HexToString(streamInformation.coding, 2));

      offset += streamLength + 1;
    }
    clipInformation.programs.emplace_back(programInformation);
  }

  return true;
}