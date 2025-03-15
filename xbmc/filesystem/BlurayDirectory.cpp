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
#include "URL.h"
#include "filesystem/BlurayCallback.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "utils/LangCodeExpander.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

#include <array>
#include <cassert>
#include <climits>
#include <memory>
#include <stdlib.h>
#include <string>

#include <libbluray/bluray-version.h>
#include <libbluray/bluray.h>
#include <libbluray/filesystem.h>
#include <libbluray/log_control.h>

namespace XFILE
{

#define MAIN_TITLE_LENGTH_PERCENT 70 /** Minimum length of main titles, based on longest title */

CBlurayDirectory::~CBlurayDirectory()
{
  Dispose();
}

void CBlurayDirectory::Dispose()
{
  if(m_bd)
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

std::string CBlurayDirectory::GetDiscInfoString(DiscInfo info)
{
  switch (info)
  {
  case XFILE::CBlurayDirectory::DiscInfo::TITLE:
  {
    if (!m_blurayInitialized)
      return "";
    const BLURAY_DISC_INFO* disc_info = bd_get_disc_info(m_bd);
    if (!disc_info || !disc_info->bluray_detected)
      return "";

    std::string title = "";

#if (BLURAY_VERSION > BLURAY_VERSION_CODE(1,0,0))
    title = disc_info->disc_name ? disc_info->disc_name : "";
#endif

    return title;
  }
  case XFILE::CBlurayDirectory::DiscInfo::ID:
  {
    if (!m_blurayInitialized)
      return "";

    const BLURAY_DISC_INFO* disc_info = bd_get_disc_info(m_bd);
    if (!disc_info || !disc_info->bluray_detected)
      return "";

    std::string id = "";

#if (BLURAY_VERSION > BLURAY_VERSION_CODE(1,0,0))
    id = disc_info->udf_volume_id ? disc_info->udf_volume_id : "";

    if (id.empty())
    {
      id = HexToString(disc_info->disc_id, 20);
    }
#endif

    return id;
  }
  default:
    break;
  }

  return "";
}

std::shared_ptr<CFileItem> CBlurayDirectory::GetTitle(const BLURAY_TITLE_INFO* title,
                                                      const std::string& label)
{
  std::string buf;
  std::string chap;
  CFileItemPtr item(new CFileItem("", false));
  CURL path(m_url);
  buf = StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", title->playlist);
  path.SetFileName(buf);
  item->SetPath(path.Get());
  int duration = (int)(title->duration / 90000);
  item->GetVideoInfoTag()->SetDuration(duration);
  item->GetVideoInfoTag()->m_iTrack = title->playlist;
  buf = StringUtils::Format(label, title->playlist);
  item->m_strTitle = buf;
  item->SetLabel(buf);
  chap = StringUtils::Format(g_localizeStrings.Get(25007), title->chapter_count,
                             StringUtils::SecondsToTimeString(duration));
  item->SetLabel2(chap);
  item->m_dwSize = 0;
  item->SetArt("icon", "DefaultVideo.png");
  for(unsigned int i = 0; i < title->clip_count; ++i)
    item->m_dwSize += title->clips[i].pkt_count * 192;

  // Generate streamdetails
  VideoStreamInfo videoInfo;
  AudioStreamInfo audioInfo;
  SubtitleStreamInfo subtitleInfo;
  CVideoInfoTag* info = item->GetVideoInfoTag();

  // Populate videoInfo
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

void CBlurayDirectory::GetTitles(bool main, CFileItemList &items)
{
  std::vector<BLURAY_TITLE_INFO*> titleList;
  uint64_t minDuration = 0;

  // Searching for a user provided list of playlists.
  if (main)
    titleList = GetUserPlaylists();

  if (!main || titleList.empty())
  {
    uint32_t numTitles = bd_get_titles(m_bd, TITLES_RELEVANT, 0);

    for (uint32_t i = 0; i < numTitles; i++)
    {
      BLURAY_TITLE_INFO* t = bd_get_title_info(m_bd, i, 0);

      if (!t)
      {
        CLog::Log(LOGDEBUG, "CBlurayDirectory - unable to get title {}", i);
        continue;
      }

      if (main && t->duration > minDuration)
          minDuration = t->duration;

      titleList.emplace_back(t);
    }
  }

  minDuration = minDuration * MAIN_TITLE_LENGTH_PERCENT / 100;

  for (auto& title : titleList)
  {
    if (title->duration < minDuration)
      continue;

    items.Add(GetTitle(title, main ? g_localizeStrings.Get(25004) /* Main Title */ : g_localizeStrings.Get(25005) /* Title */));
    bd_free_title_info(title);
  }
}

void CBlurayDirectory::GetRoot(CFileItemList &items)
{
    GetTitles(true, items);

    CURL path(m_url);
    CFileItemPtr item;

    path.SetFileName(URIUtils::AddFileToFolder(m_url.GetFileName(), "titles"));
    item = std::make_shared<CFileItem>();
    item->SetPath(path.Get());
    item->m_bIsFolder = true;
    item->SetLabel(g_localizeStrings.Get(25002) /* All titles */);
    item->SetArt("icon", "DefaultVideoPlaylists.png");
    items.Add(item);

    const BLURAY_DISC_INFO* disc_info = bd_get_disc_info(m_bd);
    if (disc_info && disc_info->no_menu_support)
    {
      CLog::Log(LOGDEBUG, "CBlurayDirectory::GetRoot - no menu support, skipping menu entry");
      return;
    }

    path.SetFileName("menu");
    item = std::make_shared<CFileItem>();
    item->SetPath(path.Get());
    item->m_bIsFolder = false;
    item->SetLabel(g_localizeStrings.Get(25003) /* Menus */);
    item->SetArt("icon", "DefaultProgram.png");
    items.Add(item);
}

bool CBlurayDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  Dispose();
  m_url = url;
  std::string root = m_url.GetHostName();
  std::string file = m_url.GetFileName();
  URIUtils::RemoveSlashAtEnd(file);
  URIUtils::RemoveSlashAtEnd(root);

  if (!InitializeBluray(root))
    return false;

  if(file == "root")
    GetRoot(items);
  else if(file == "root/titles")
    GetTitles(false, items);
  else
  {
    CURL url2 = GetUnderlyingCURL(url);
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

  items.AddSortMethod(SortByTrackNumber,  554, LABEL_MASKS("%L", "%D", "%L", ""));    // FileName, Duration | Foldername, empty
  items.AddSortMethod(SortBySize,         553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // FileName, Size | Foldername, Size

  return true;
}

CURL CBlurayDirectory::GetUnderlyingCURL(const CURL& url)
{
  assert(url.IsProtocol("bluray"));
  std::string host = url.GetHostName();
  const std::string& filename = url.GetFileName();
  return CURL(host.append(filename));
}

bool CBlurayDirectory::InitializeBluray(const std::string &root)
{
  bd_set_debug_handler(CBlurayCallback::bluray_logger);
  bd_set_debug_mask(DBG_CRIT | DBG_BLURAY | DBG_NAV);

  m_bd = bd_init();

  if (!m_bd)
  {
    CLog::Log(LOGERROR, "CBlurayDirectory::InitializeBluray - failed to initialize libbluray");
    return false;
  }

  std::string langCode;
  g_LangCodeExpander.ConvertToISO6392T(g_langInfo.GetDVDMenuLanguage(), langCode);
  bd_set_player_setting_str(m_bd, BLURAY_PLAYER_SETTING_MENU_LANG, langCode.c_str());

  if (!bd_open_files(m_bd, const_cast<std::string*>(&root), CBlurayCallback::dir_open, CBlurayCallback::file_open))
  {
    CLog::Log(LOGERROR, "CBlurayDirectory::InitializeBluray - failed to open {}",
              CURL::GetRedacted(root));
    return false;
  }
  m_blurayInitialized = true;

  return true;
}

std::string CBlurayDirectory::HexToString(const uint8_t *buf, int count)
{
  std::array<char, 42> tmp;

  for (int i = 0; i < count; i++)
  {
    sprintf(tmp.data() + (i * 2), "%02x", buf[i]);
  }

  return std::string(std::begin(tmp), std::end(tmp));
}

std::vector<BLURAY_TITLE_INFO*> CBlurayDirectory::GetUserPlaylists()
{
  std::string root = m_url.GetHostName();
  std::string discInfPath = URIUtils::AddFileToFolder(root, "disc.inf");
  std::vector<BLURAY_TITLE_INFO*> userTitles;
  CFile file;
  std::string line;
  line.reserve(1024);

  if (file.Open(discInfPath))
  {
    CLog::Log(LOGDEBUG, "CBlurayDirectory::GetTitles - disc.inf found");

    CRegExp pl(true);
    if (!pl.RegComp("(\\d+)"))
    {
      file.Close();
      return userTitles;
    }

    uint8_t maxLines = 100;
    while ((maxLines > 0) && file.ReadLine(line))
    {
      maxLines--;
      if (StringUtils::StartsWithNoCase(line, "playlists"))
      {
        int pos = 0;
        while ((pos = pl.RegFind(line, static_cast<unsigned int>(pos))) >= 0)
        {
          std::string playlist = pl.GetMatch(0);
          uint32_t len = static_cast<uint32_t>(playlist.length());

          if (len <= 5)
          {
            unsigned long int plNum = strtoul(playlist.c_str(), nullptr, 10);

            BLURAY_TITLE_INFO* t = bd_get_playlist_info(m_bd, static_cast<uint32_t>(plNum), 0);
            if (t)
              userTitles.emplace_back(t);
          }

          if (static_cast<int64_t>(pos) + static_cast<int64_t>(len) > INT_MAX)
            break;
          else
            pos += len;
        }
      }
    }
    file.Close();
  }
  return userTitles;
}

} /* namespace XFILE */
