/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "BlurayDirectory.h"

#include "File.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "LangInfo.h"
#include "filesystem/BlurayCallback.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "utils/LangCodeExpander.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

#include <cassert>
#include <memory>
#include <regex>
#include <stdlib.h>
#include <string>

#include <libbluray/bluray-version.h>
#include <libbluray/bluray.h>
#include <libbluray/log_control.h>

namespace XFILE
{

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

std::string CBlurayDirectory::GetBlurayTitle()
{
  return GetDiscInfoString(DiscInfo::TITLE);
}

std::string CBlurayDirectory::GetBlurayID()
{
  return GetDiscInfoString(DiscInfo::ID);
}

std::string CBlurayDirectory::GetDiscInfoString(DiscInfo info) const
{
  switch (info)
  {
    case DiscInfo::TITLE:
    {
      if (!m_blurayInitialized)
        return "";
      const BLURAY_DISC_INFO* disc_info = bd_get_disc_info(m_bd);
      if (!disc_info || !disc_info->bluray_detected)
        return "";

      std::string title;

#if (BLURAY_VERSION > BLURAY_VERSION_CODE(1, 0, 0))
      title = disc_info->disc_name ? disc_info->disc_name : "";
#endif

      return title;
    }
    case DiscInfo::ID:
    {
      if (!m_blurayInitialized)
        return "";

      const BLURAY_DISC_INFO* disc_info = bd_get_disc_info(m_bd);
      if (!disc_info || !disc_info->bluray_detected)
        return "";

      std::string id;

#if (BLURAY_VERSION > BLURAY_VERSION_CODE(1, 0, 0))
      id = disc_info->udf_volume_id ? disc_info->udf_volume_id : "";

      if (id.empty())
      {
        id = CDiscDirectoryHelper::HexToString(disc_info->disc_id, 10);
      }
#endif

      return id;
    }
  }

  return "";
}

void CBlurayDirectory::GetPlaylistInfo(ClipMap& clips, PlaylistMap& playlists) const
{
  // Get all titles on disc
  // Sort by playlist for grouping later

  CFileItemList allTitles;
  GetTitles(GetTitlesJob::GET_TITLES_ALL, allTitles, SortTitlesJob::SORT_TITLES_NONE);
  SortDescription sorting;
  sorting.sortBy = SortByFile;
  allTitles.Sort(sorting);

  // Get information on all titles
  // Including relationship between clips and playlists
  // List all playlists

  CLog::LogF(LOGDEBUG, "*** Playlist information ***");
  const std::regex playlistPath("(\\d{5}.mpls)");
  std::smatch playlistMatch;
  for (const auto& title : allTitles)
  {
    const CURL url{title->GetDynPath()};
    const std::string filename{URIUtils::GetFileName(url.GetFileName())};
    if (std::regex_search(filename, playlistMatch, playlistPath))
    {
      const unsigned int playlist{static_cast<unsigned int>(std::stoi(playlistMatch[0]))};
      BLURAY_TITLE_INFO* titleInfo{bd_get_playlist_info(m_bd, playlist, 0)};
      if (!titleInfo)
      {
        CLog::LogF(LOGDEBUG, "Unable to get playlist {}", playlist);
      }
      else
      {
        // Save playlist
        PlaylistInformation info;

        // Save playlist duration
        info.duration = static_cast<unsigned int>(titleInfo->duration / 90000);

        // Get clips
        std::string clipsStr;
        for (unsigned int i = 0; i < titleInfo->clip_count; ++i)
        {
          // Add clip to playlist
          const unsigned int clip{
              static_cast<unsigned int>(strtoul(titleInfo->clips[i].clip_id, nullptr, 10))};
          info.clips.emplace_back(clip);

          // Add/extend clip information
          const auto& it = clips.find(clip);
          if (it == clips.end())
          {
            // First reference to clip
            ClipInformation clipInfo;
            clipInfo.duration = static_cast<unsigned int>(
                (titleInfo->clips[i].out_time - titleInfo->clips[i].in_time) / 90000);
            clipInfo.playlists.emplace_back(playlist);
            clips[clip] = clipInfo;
          }
          else
          {
            // Additional reference to clip, add this playlist
            it->second.playlists.emplace_back(playlist);
          }

          const std::string c{titleInfo->clips[i].clip_id};
          clipsStr += c + ",";
        }
        if (!clipsStr.empty())
          clipsStr.pop_back(); // Remove last ','

        playlists[playlist] = info;

        // Get languages
        std::string langs;
        for (int i = 0; i < titleInfo->clips[0].audio_stream_count; ++i)
        {
          const std::string l{
              reinterpret_cast<char const*>(titleInfo->clips[0].audio_streams[i].lang)};
          langs += l + ",";
        }
        if (!langs.empty())
          langs.pop_back(); // Remove last ','

        playlists[playlist].languages = langs;

        CLog::LogF(LOGDEBUG, "Playlist {}, Duration {}, Langs {}, Clips {} ", playlist,
                   title->GetVideoInfoTag()->GetDuration(), langs, clipsStr);

        bd_free_title_info(titleInfo);
      }
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
}

std::shared_ptr<CFileItem> CBlurayDirectory::GetTitle(const BLURAY_TITLE_INFO* title,
                                                      const std::string& label) const
{
  const auto item{std::make_shared<CFileItem>("", false)};
  CURL path{m_url};
  std::string buf{StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", title->playlist)};
  path.SetFileName(buf);
  item->SetPath(path.Get());
  const int duration = static_cast<int>(title->duration / 90000);
  item->GetVideoInfoTag()->SetDuration(duration);
  item->GetVideoInfoTag()->m_iTrack = static_cast<int>(title->playlist);
  buf = StringUtils::Format(label, title->playlist);
  item->m_strTitle = buf;
  item->SetLabel(buf);
  const std::string chap{StringUtils::Format(g_localizeStrings.Get(25007), title->chapter_count,
                                             StringUtils::SecondsToTimeString(duration))};
  item->SetLabel2(chap);
  item->m_dwSize = 0;
  item->SetArt("icon", "DefaultVideo.png");
  for (unsigned int i = 0; i < title->clip_count; ++i)
    item->m_dwSize += static_cast<int64_t>(title->clips[i].pkt_count * 192);

  // Generate streamdetails

  // Populate videoInfo
  VideoStreamInfo videoInfo;
  if (title->clip_count > 0 && title->clips[0].video_stream_count > 0)
  {
    const auto& video{title->clips[0].video_streams[0]};
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
    switch (video.coding_type)
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

    AudioStreamInfo audioInfo;
    SubtitleStreamInfo subtitleInfo;
    CVideoInfoTag* info = item->GetVideoInfoTag();
    info->m_streamDetails.SetStreams(videoInfo, static_cast<int>(title->duration / 90000),
                                     audioInfo, subtitleInfo);

    for (int i = 0; i < title->clips[0].audio_stream_count; ++i)
    {
      AudioStreamInfo audioInfo;
      auto& audio{title->clips[0].audio_streams[i]};

      audioInfo.valid = true;
      audioInfo.bitrate = 0;
      audioInfo.channels = 0; // Only basic mono/stereo/multichannel is stored in BLURAY_TITLE_INFO

      switch (audio.coding_type)
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
      audioInfo.language = reinterpret_cast<char*>(audio.lang);
      info->m_streamDetails.AddStream(new CStreamDetailAudio(audioInfo));
    }

    // Subtitles
    for (int i = 0; i < title->clips[0].pg_stream_count; ++i)
    {
      SubtitleStreamInfo subtitleInfo;
      auto& subtitle{title->clips[0].pg_streams[i]};

      subtitleInfo.valid = true;
      subtitleInfo.bitrate = 0;
      subtitleInfo.codecDesc = "";
      subtitleInfo.codecName = "";
      subtitleInfo.isExternal = false;
      subtitleInfo.name = "";
      subtitleInfo.flags = FLAG_NONE;

      subtitleInfo.language = reinterpret_cast<char*>(subtitle.lang);
      info->m_streamDetails.AddStream(new CStreamDetailSubtitle(subtitleInfo));
    }
  }

  return item;
}

void CBlurayDirectory::GetTitles(GetTitlesJob job, CFileItemList& items, SortTitlesJob sort) const
{
  std::vector<BLURAY_TITLE_INFO*> titleList;
  uint64_t maxDuration{0};
  int maxPlaylist{-1};
  int mainPlaylist{-1};

  // See if disc.inf for main playlist
  if (job != GetTitlesJob::GET_TITLES_ALL)
  {
    mainPlaylist = GetUserPlaylists();
    if (mainPlaylist != -1)
    {
      // Only main playlist is needed
      if (BLURAY_TITLE_INFO * t{bd_get_playlist_info(m_bd, mainPlaylist, 0)}; !t)
        CLog::LogF(LOGDEBUG, "Unable to get title {}", mainPlaylist);
      else
        titleList.emplace_back(t);
    }
  }

  if (titleList.empty())
  {
    const uint32_t numTitles{bd_get_titles(m_bd, TITLES_RELEVANT, 0)};
    for (uint32_t i = 0; i < numTitles; i++)
    {
      if (BLURAY_TITLE_INFO * t{bd_get_title_info(m_bd, i, 0)}; !t)
        CLog::LogF(LOGDEBUG, "Unable to get title {}", i);
      else
      {
        if (t->duration > maxDuration)
        {
          maxDuration = t->duration;
          maxPlaylist = static_cast<int>(t->playlist);
        }
        titleList.emplace_back(t);
      }
    }
  }

  // Sort
  // Movies - placing main title - if present - first, then by duration
  // Episodes - by playlist number
  if (sort != SortTitlesJob::SORT_TITLES_NONE)
  {
    std::ranges::sort(titleList,
                      [&sort](const BLURAY_TITLE_INFO* i, const BLURAY_TITLE_INFO* j)
                      {
                        if (sort == SortTitlesJob::SORT_TITLES_MOVIE)
                        {
                          if (i->duration == j->duration)
                            return i->playlist < j->playlist;
                          return i->duration > j->duration;
                        }
                        return i->playlist < j->playlist;
                      });

    const auto& pivot{
        std::ranges::find_if(titleList, [&mainPlaylist](const BLURAY_TITLE_INFO* title)
                             { return title->playlist == static_cast<uint32_t>(mainPlaylist); })};
    if (pivot != titleList.end())
      std::rotate(titleList.begin(), pivot, pivot + 1);
  }

  const uint64_t minDuration{maxDuration * MAIN_TITLE_LENGTH_PERCENT / 100};
  for (const auto& title : titleList)
  {
    if (job == GetTitlesJob::GET_TITLES_ALL ||
        (job == GetTitlesJob::GET_TITLES_MAIN && title->duration >= minDuration) ||
        (job == GetTitlesJob::GET_TITLES_ONE &&
         (title->playlist == static_cast<uint32_t>(mainPlaylist) ||
          (mainPlaylist == -1 && title->playlist == static_cast<uint32_t>(maxPlaylist)))))
    {
      items.Add(GetTitle(title, title->playlist == static_cast<uint32_t>(mainPlaylist)
                                    ? g_localizeStrings.Get(25004) /* Main Title */
                                    : g_localizeStrings.Get(25005) /* Title */));
    }
    bd_free_title_info(title);
  }
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
  // /root/episode/all                  - get all episodes
  if (file == "root")
  {
    GetTitles(GetTitlesJob::GET_TITLES_MAIN, items, SortTitlesJob::SORT_TITLES_MOVIE);

    // Add all titles and menu options
    CDiscDirectoryHelper::AddRootOptions(m_url, items, m_blurayMenuSupport);
  }
  else if (file == "root/titles")
    GetTitles(GetTitlesJob::GET_TITLES_ALL, items, SortTitlesJob::SORT_TITLES_MOVIE);
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
      episodeIndex = std::distance(episodesOnDisc.begin(), it);
    }

    // Get playlist, clip and language information
    ClipMap clips;
    PlaylistMap playlists;
    GetPlaylistInfo(clips, playlists);

    // Get episode playlists
    CDiscDirectoryHelper::GetEpisodeTitles(m_url, items, episodeIndex, episodesOnDisc, clips,
                                           playlists);

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

  if (!bd_open_files(m_bd, const_cast<std::string*>(&root), CBlurayCallback::dir_open,
                     CBlurayCallback::file_open))
  {
    CLog::LogF(LOGERROR, "Failed to open {}", CURL::GetRedacted(root));
    return false;
  }
  m_blurayInitialized = true;

  const BLURAY_DISC_INFO* disc_info{bd_get_disc_info(m_bd)};
  m_blurayMenuSupport = disc_info && !disc_info->no_menu_support;

  return true;
}

int CBlurayDirectory::GetUserPlaylists() const
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
} // namespace XFILE