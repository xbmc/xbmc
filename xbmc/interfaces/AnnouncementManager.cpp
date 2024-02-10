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
#include <stdio.h>

#define LOOKUP_PROPERTY "database-lookup"

using namespace ANNOUNCEMENT;
using namespace KODI::VIDEO;

const std::string CAnnouncementManager::ANNOUNCEMENT_SENDER = "xbmc";

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
  std::unique_lock<CCriticalSection> lock(m_announcersCritSection);
  m_announcers.clear();
}

void CAnnouncementManager::AddAnnouncer(IAnnouncer *listener)
{
  if (!listener)
    return;

  std::unique_lock<CCriticalSection> lock(m_announcersCritSection);
  m_announcers.push_back(listener);
}

void CAnnouncementManager::RemoveAnnouncer(IAnnouncer *listener)
{
  if (!listener)
    return;

  std::unique_lock<CCriticalSection> lock(m_announcersCritSection);
  for (unsigned int i = 0; i < m_announcers.size(); i++)
  {
    if (m_announcers[i] == listener)
    {
      m_announcers.erase(m_announcers.begin() + i);
      return;
    }
  }
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
    std::unique_lock<CCriticalSection> lock(m_queueCritSection);
    m_announcementQueue.push_back(announcement);
  }
  m_queueEvent.Set();
}

void CAnnouncementManager::DoAnnounce(AnnouncementFlag flag,
                                      const std::string& sender,
                                      const std::string& message,
                                      const CVariant& data)
{
  CLog::Log(LOGDEBUG, LOGANNOUNCE, "CAnnouncementManager - Announcement: {} from {}", message, sender);

  std::unique_lock<CCriticalSection> lock(m_announcersCritSection);

  // Make a copy of announcers. They may be removed or even remove themselves during execution of IAnnouncer::Announce()!

  std::vector<IAnnouncer *> announcers(m_announcers);
  for (unsigned int i = 0; i < announcers.size(); i++)
    announcers[i]->Announce(flag, sender, message, data);
}

void CAnnouncementManager::DoAnnounce(AnnouncementFlag flag,
                                      const std::string& sender,
                                      const std::string& message,
                                      const std::shared_ptr<CFileItem>& item,
                                      const CVariant& data)
{
  if (item == nullptr)
  {
    DoAnnounce(flag, sender, message, data);
    return;
  }

  // Extract db id of item
  CVariant object = data.isNull() || data.isObject() ? data : CVariant::VariantTypeObject;
  std::string type;
  int id = 0;

  if(item->HasPVRChannelInfoTag())
  {
    const std::shared_ptr<PVR::CPVRChannel> channel(item->GetPVRChannelInfoTag());
    id = channel->ChannelID();
    type = "channel";

    object["item"]["title"] = channel->ChannelName();
    object["item"]["channeltype"] = channel->IsRadio() ? "radio" : "tv";

    if (data.isMember("player") && data["player"].isMember("playerid"))
    {
      object["player"]["playerid"] =
          channel->IsRadio() ? PLAYLIST::TYPE_MUSIC : PLAYLIST::TYPE_VIDEO;
    }
  }
  else if (item->HasVideoInfoTag() && !item->HasPVRRecordingInfoTag())
  {
    id = item->GetVideoInfoTag()->m_iDbId;

    //! @todo Can be removed once this is properly handled when starting playback of a file
    if (id <= 0 && !item->GetPath().empty() &&
       (!item->HasProperty(LOOKUP_PROPERTY) || item->GetProperty(LOOKUP_PROPERTY).asBoolean()))
    {
      CVideoDatabase videodatabase;
      if (videodatabase.Open())
      {
        std::string path = item->GetPath();
        std::string videoInfoTagPath(item->GetVideoInfoTag()->m_strFileNameAndPath);
        if (StringUtils::StartsWith(videoInfoTagPath, "removable://"))
          path = videoInfoTagPath;
        if (videodatabase.LoadVideoInfo(path, *item->GetVideoInfoTag(), VideoDbDetailsNone))
          id = item->GetVideoInfoTag()->m_iDbId;

        videodatabase.Close();
      }
    }

    if (!item->GetVideoInfoTag()->m_type.empty())
      type = item->GetVideoInfoTag()->m_type;
    else
      CVideoDatabase::VideoContentTypeToString(item->GetVideoContentType(), type);

    if (id <= 0)
    {
      //! @todo Can be removed once this is properly handled when starting playback of a file
      item->SetProperty(LOOKUP_PROPERTY, false);

      std::string title = item->GetVideoInfoTag()->m_strTitle;
      if (title.empty())
        title = item->GetLabel();
      object["item"]["title"] = title;

      switch (item->GetVideoContentType())
      {
        case VideoDbContentType::MOVIES:
          if (item->GetVideoInfoTag()->HasYear())
            object["item"]["year"] = item->GetVideoInfoTag()->GetYear();
          break;
        case VideoDbContentType::EPISODES:
          if (item->GetVideoInfoTag()->m_iEpisode >= 0)
            object["item"]["episode"] = item->GetVideoInfoTag()->m_iEpisode;
          if (item->GetVideoInfoTag()->m_iSeason >= 0)
            object["item"]["season"] = item->GetVideoInfoTag()->m_iSeason;
          if (!item->GetVideoInfoTag()->m_strShowTitle.empty())
            object["item"]["showtitle"] = item->GetVideoInfoTag()->m_strShowTitle;
          break;
        case VideoDbContentType::MUSICVIDEOS:
          if (!item->GetVideoInfoTag()->m_strAlbum.empty())
            object["item"]["album"] = item->GetVideoInfoTag()->m_strAlbum;
          if (!item->GetVideoInfoTag()->m_artist.empty())
            object["item"]["artist"] = StringUtils::Join(item->GetVideoInfoTag()->m_artist, " / ");
          break;
        default:
          break;
      }
    }
  }
  else if (item->HasMusicInfoTag())
  {
    id = item->GetMusicInfoTag()->GetDatabaseId();
    type = MediaTypeSong;

    //! @todo Can be removed once this is properly handled when starting playback of a file
    if (id <= 0 && !item->GetPath().empty() &&
       (!item->HasProperty(LOOKUP_PROPERTY) || item->GetProperty(LOOKUP_PROPERTY).asBoolean()))
    {
      CMusicDatabase musicdatabase;
      if (musicdatabase.Open())
      {
        CSong song;
        if (musicdatabase.GetSongByFileName(item->GetPath(), song, item->GetStartOffset()))
        {
          item->GetMusicInfoTag()->SetSong(song);
          id = item->GetMusicInfoTag()->GetDatabaseId();
        }

        musicdatabase.Close();
      }
    }

    if (id <= 0)
    {
      //! @todo Can be removed once this is properly handled when starting playback of a file
      item->SetProperty(LOOKUP_PROPERTY, false);

      std::string title = item->GetMusicInfoTag()->GetTitle();
      if (title.empty())
        title = item->GetLabel();
      object["item"]["title"] = title;

      if (item->GetMusicInfoTag()->GetTrackNumber() > 0)
        object["item"]["track"] = item->GetMusicInfoTag()->GetTrackNumber();
      if (!item->GetMusicInfoTag()->GetAlbum().empty())
        object["item"]["album"] = item->GetMusicInfoTag()->GetAlbum();
      if (!item->GetMusicInfoTag()->GetArtist().empty())
        object["item"]["artist"] = item->GetMusicInfoTag()->GetArtist();
    }
  }
  else if (IsVideo(*item))
  {
    // video item but has no video info tag.
    type = "movie";
    object["item"]["title"] = item->GetLabel();
  }
  else if (item->HasPictureInfoTag())
  {
    type = "picture";
    object["item"]["file"] = item->GetPath();
  }
  else
    type = "unknown";

  object["item"]["type"] = type;
  if (id > 0)
    object["item"]["id"] = id;

  DoAnnounce(flag, sender, message, object);
}

void CAnnouncementManager::Process()
{
  SetPriority(ThreadPriority::LOWEST);

  while (!m_bStop)
  {
    std::unique_lock<CCriticalSection> lock(m_queueCritSection);
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
