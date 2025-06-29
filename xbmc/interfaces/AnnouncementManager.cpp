/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AnnouncementManager.h"

#include "FileItem.h"
#include "music/MusicDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayListTypes.h"
#include "pvr/channels/PVRChannel.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"

#include <memory>
#include <mutex>

#define LOOKUP_PROPERTY "database-lookup"

using namespace ANNOUNCEMENT;
using namespace KODI;

const std::string CAnnouncementManager::ANNOUNCEMENT_SENDER = "xbmc";

namespace
{

void CopyPVRTagInfoToObject(const PVR::CPVRChannel& channel, bool copyPlayerId, CVariant& object)
{
  auto& objItem = object["item"];

  objItem["type"] = "channel";
  objItem["title"] = channel.ChannelName();
  objItem["channeltype"] = channel.IsRadio() ? "radio" : "tv";

  if (copyPlayerId)
  {
    object["player"]["playerid"] =
        static_cast<int>(channel.IsRadio() ? PLAYLIST::Id::TYPE_MUSIC : PLAYLIST::Id::TYPE_VIDEO);
  }

  objItem["id"] = channel.ChannelID();
}

void CopyVideoTagInfoToObject(CFileItem& item, CVariant& object)
{
  CVideoInfoTag& tag = *item.GetVideoInfoTag();

  auto& objItem = object["item"];
  int id = tag.GetDatabaseId();

  //! @todo Can be removed once this is properly handled when starting playback of a file
  if (id <= 0 && !item.GetPath().empty() && item.GetProperty(LOOKUP_PROPERTY).asBoolean(true))
  {
    CVideoDatabase videodatabase;
    if (videodatabase.Open())
    {
      std::string videoInfoTagPath = tag.m_strFileNameAndPath;
      std::string path;
      if (StringUtils::StartsWith(videoInfoTagPath, "removable://"))
        path = videoInfoTagPath;
      else
        path = item.GetPath();
      if (videodatabase.LoadVideoInfo(path, tag))
        id = tag.GetDatabaseId();

      videodatabase.Close();
    }
    else
    {
      CLog::LogFC(LOGWARNING, LOGANNOUNCE,
                  "Unable to open video database. Can not load video tag for announcement!");
    }
  }

  if (!tag.m_type.empty())
    objItem["type"] = tag.m_type;
  else
    objItem["type"] = CVideoDatabase::VideoContentTypeToString(item.GetVideoContentType());

  if (id <= 0)
  {
    //! @todo Can be removed once this is properly handled when starting playback of a file
    item.SetProperty(LOOKUP_PROPERTY, false);

    std::string title = tag.m_strTitle;
    if (title.empty())
      title = item.GetLabel();
    objItem["title"] = title;

    using enum VideoDbContentType;
    switch (item.GetVideoContentType())
    {
      case MOVIES:
        if (tag.HasYear())
          objItem["year"] = tag.GetYear();
        break;
      case EPISODES:
        if (tag.m_iEpisode >= 0)
          objItem["episode"] = tag.m_iEpisode;
        if (tag.m_iSeason >= 0)
          objItem["season"] = tag.m_iSeason;
        if (!tag.m_strShowTitle.empty())
          objItem["showtitle"] = tag.m_strShowTitle;
        break;
      case MUSICVIDEOS:
        if (!tag.m_strAlbum.empty())
          objItem["album"] = tag.m_strAlbum;
        if (!tag.m_artist.empty())
          objItem["artist"] = StringUtils::Join(tag.m_artist, " / ");
        break;
      default:
        break;
    }
  }
  else
  {
    objItem["id"] = id;
  }
}

void CopyMusicTagInfoToObject(CFileItem& item, CVariant& object)
{
  MUSIC_INFO::CMusicInfoTag& tag = *item.GetMusicInfoTag();

  auto& objItem = object["item"];
  int id = tag.GetDatabaseId();
  objItem["type"] = MediaTypeSong;

  //! @todo Can be removed once this is properly handled when starting playback of a file
  if (id <= 0 && !item.GetPath().empty() && item.GetProperty(LOOKUP_PROPERTY).asBoolean(true))
  {
    CMusicDatabase musicdatabase;
    if (musicdatabase.Open())
    {
      CSong song;
      if (musicdatabase.GetSongByFileName(item.GetPath(), song, item.GetStartOffset()))
      {
        tag.SetSong(song);
        id = tag.GetDatabaseId();
      }

      musicdatabase.Close();
    }
    else
    {
      CLog::LogFC(LOGWARNING, LOGANNOUNCE,
                  "Unable to open music database. Can not load song tag for announcement!");
    }
  }

  if (id <= 0)
  {
    //! @todo Can be removed once this is properly handled when starting playback of a file
    item.SetProperty(LOOKUP_PROPERTY, false);

    objItem["title"] = tag.GetTitle();
    if (objItem["title"].empty())
      objItem["title"] = item.GetLabel();

    if (tag.GetTrackNumber() > 0)
      objItem["track"] = tag.GetTrackNumber();
    if (!tag.GetAlbum().empty())
      objItem["album"] = tag.GetAlbum();
    if (!tag.GetArtist().empty())
      objItem["artist"] = tag.GetArtist();
  }
  else
  {
    objItem["id"] = id;
  }
}

CVariant CreateDataObjectFromItem(CFileItem& item, const CVariant& data)
{
  CVariant object;
  if (data.isNull() || data.isObject())
    object = data;
  else
    object = CVariant::VariantTypeObject;

  if (item.HasPVRChannelInfoTag())
  {
    const bool copyPlayerId = data.isMember("player") && data["player"].isMember("playerid");
    CopyPVRTagInfoToObject(*item.GetPVRChannelInfoTag(), copyPlayerId, object);
  }
  else if (item.HasVideoInfoTag() && !item.HasPVRRecordingInfoTag())
  {
    CopyVideoTagInfoToObject(item, object);
  }
  else if (item.HasMusicInfoTag())
  {
    CopyMusicTagInfoToObject(item, object);
  }
  else if (VIDEO::IsVideo(item))
  {
    // video item but has no video info tag.
    object["item"]["type"] = "movie";
    object["item"]["title"] = item.GetLabel();
  }
  else if (item.HasPictureInfoTag())
  {
    object["item"]["type"] = "picture";
    object["item"]["file"] = item.GetPath();
  }
  else
    object["item"]["type"] = "unknown";

  return object;
}

} // unnamed namespace

CAnnouncementManager::CAnnouncementManager() : CThread("Announce")
{
}

CAnnouncementManager::~CAnnouncementManager()
{
  Deinitialize();
}

void CAnnouncementManager::Start()
{
  Create();
}

void CAnnouncementManager::Deinitialize()
{
  m_bStop = true;
  m_queueEvent.Set();
  StopThread();
  std::unique_lock lock(m_announcersCritSection);
  m_announcers.clear();
}

void CAnnouncementManager::AddAnnouncer(IAnnouncer *listener)
{
  return AddAnnouncer(listener, ANNOUNCE_ALL);
}

void CAnnouncementManager::AddAnnouncer(IAnnouncer* listener, int flagMask)
{
  if (!listener)
    return;

  std::unique_lock lock(m_announcersCritSection);
  m_announcers.emplace(listener, flagMask);
}

void CAnnouncementManager::RemoveAnnouncer(IAnnouncer *listener)
{
  if (!listener)
    return;

  std::unique_lock lock(m_announcersCritSection);
  m_announcers.erase(listener);
}

void CAnnouncementManager::Announce(AnnouncementFlag flag, const std::string& message)
{
  CVariant data;
  Announce(flag, ANNOUNCEMENT_SENDER, message, CFileItemPtr(), data);
}

void CAnnouncementManager::Announce(AnnouncementFlag flag,
                                    const std::string& message,
                                    const CVariant& data)
{
  Announce(flag, ANNOUNCEMENT_SENDER, message, CFileItemPtr(), data);
}

void CAnnouncementManager::Announce(AnnouncementFlag flag,
                                    const std::string& message,
                                    const std::shared_ptr<const CFileItem>& item)
{
  CVariant data;
  Announce(flag, ANNOUNCEMENT_SENDER, message, item, data);
}

void CAnnouncementManager::Announce(AnnouncementFlag flag,
                                    const std::string& message,
                                    const std::shared_ptr<const CFileItem>& item,
                                    const CVariant& data)
{
  Announce(flag, ANNOUNCEMENT_SENDER, message, item, data);
}


void CAnnouncementManager::Announce(AnnouncementFlag flag,
                                    const std::string& sender,
                                    const std::string& message)
{
  CVariant data;
  Announce(flag, sender, message, CFileItemPtr(), data);
}

void CAnnouncementManager::Announce(AnnouncementFlag flag,
                                    const std::string& sender,
                                    const std::string& message,
                                    const CVariant& data)
{
  Announce(flag, sender, message, CFileItemPtr(), data);
}

void CAnnouncementManager::Announce(AnnouncementFlag flag,
                                    const std::string& sender,
                                    const std::string& message,
                                    const std::shared_ptr<const CFileItem>& item,
                                    const CVariant& data)
{
  CAnnounceData announcement;
  announcement.flag = flag;
  announcement.sender = sender;
  announcement.message = message;
  announcement.data = data;

  if (item != nullptr)
    announcement.item = std::make_shared<CFileItem>(*item);

  {
    std::unique_lock lock(m_queueCritSection);
    m_announcementQueue.push_back(announcement);
  }
  m_queueEvent.Set();
}

void CAnnouncementManager::DoAnnounce(AnnouncementFlag flag,
                                      const std::string& sender,
                                      const std::string& message,
                                      const CVariant& data)
{
  CLog::LogFC(LOGWARNING, LOGANNOUNCE, "CAnnouncementManager - Announcement: {} from {}", message,
              sender);

  std::unique_lock lock(m_announcersCritSection);

  // Make a copy of announcers. They may be removed or even remove themselves during execution of IAnnouncer::Announce()!
  std::unordered_map<IAnnouncer*, int> announcers{m_announcers};
  for (const auto& [announcer, flagMask] : announcers)
  {
    if (flag & flagMask)
    {
      announcer->Announce(flag, sender, message, data);
    }
  }
}

void CAnnouncementManager::DoAnnounce(AnnouncementFlag flag,
                                      const std::string& sender,
                                      const std::string& message,
                                      const std::shared_ptr<CFileItem>& item,
                                      const CVariant& data)
{
  if (item == nullptr)
    DoAnnounce(flag, sender, message, data);
  else
    DoAnnounce(flag, sender, message, CreateDataObjectFromItem(*item, data));
}

void CAnnouncementManager::Process()
{
  SetPriority(ThreadPriority::LOWEST);

  while (!m_bStop)
  {
    std::unique_lock lock(m_queueCritSection);
    if (!m_announcementQueue.empty())
    {
      auto announcement = m_announcementQueue.front();
      m_announcementQueue.pop_front();
      {
        CSingleExit ex(m_queueCritSection);
        DoAnnounce(announcement.flag, announcement.sender, announcement.message, announcement.item,
                   announcement.data);
      }
    }
    else
    {
      CSingleExit ex(m_queueCritSection);
      m_queueEvent.Wait();
    }
  }
}
