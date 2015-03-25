/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AnnouncementManager.h"
#include "threads/SingleLock.h"
#include <stdio.h>
#include "utils/log.h"
#include "utils/Variant.h"
#include "utils/StringUtils.h"
#include "FileItem.h"
#include "music/tags/MusicInfoTag.h"
#include "music/MusicDatabase.h"
#include "video/VideoDatabase.h"
#include "pvr/channels/PVRChannel.h"
#include "PlayListPlayer.h"

#define LOOKUP_PROPERTY "database-lookup"

using namespace std;
using namespace ANNOUNCEMENT;

CAnnouncementManager::CAnnouncementManager()
{ }

CAnnouncementManager::~CAnnouncementManager()
{
  Deinitialize();
}

CAnnouncementManager& CAnnouncementManager::Get()
{
  static CAnnouncementManager s_instance;
  return s_instance;
}

void CAnnouncementManager::Deinitialize()
{
  CSingleLock lock (m_critSection);
  m_announcers.clear();
}

void CAnnouncementManager::AddAnnouncer(IAnnouncer *listener)
{
  if (!listener)
    return;

  CSingleLock lock (m_critSection);
  m_announcers.push_back(listener);
}

void CAnnouncementManager::RemoveAnnouncer(IAnnouncer *listener)
{
  if (!listener)
    return;

  CSingleLock lock (m_critSection);
  for (unsigned int i = 0; i < m_announcers.size(); i++)
  {
    if (m_announcers[i] == listener)
    {
      m_announcers.erase(m_announcers.begin() + i);
      return;
    }
  }
}

void CAnnouncementManager::Announce(AnnouncementFlag flag, const char *sender, const char *message)
{
  CVariant data;
  Announce(flag, sender, message, data);
}

void CAnnouncementManager::Announce(AnnouncementFlag flag, const char *sender, const char *message, CVariant &data)
{
  CLog::Log(LOGDEBUG, "CAnnouncementManager - Announcement: %s from %s", message, sender);

  CSingleLock lock (m_critSection);

  // Make a copy of announers. They may be removed or even remove themselves during execution of IAnnouncer::Announce()!
  std::vector<IAnnouncer *> announcers(m_announcers); 
  for (unsigned int i = 0; i < announcers.size(); i++)
    announcers[i]->Announce(flag, sender, message, data);
}

void CAnnouncementManager::Announce(AnnouncementFlag flag, const char *sender, const char *message, CFileItemPtr item)
{
  CVariant data;
  Announce(flag, sender, message, item, data);
}

void CAnnouncementManager::Announce(AnnouncementFlag flag, const char *sender, const char *message, CFileItemPtr item, CVariant &data)
{
  if (!item.get())
  {
    Announce(flag, sender, message, data);
    return;
  }

  // Extract db id of item
  CVariant object = data.isNull() || data.isObject() ? data : CVariant::VariantTypeObject;
  std::string type;
  int id = 0;
  
  if(item->HasPVRChannelInfoTag())
  {
    const PVR::CPVRChannelPtr channel(item->GetPVRChannelInfoTag());
    id = channel->ChannelID();
    type = "channel";

    object["item"]["title"] = channel->ChannelName();
    object["item"]["channeltype"] = channel->IsRadio() ? "radio" : "tv";

    if (data.isMember("player") && data["player"].isMember("playerid"))
      object["player"]["playerid"] = channel->IsRadio() ? PLAYLIST_MUSIC : PLAYLIST_VIDEO;
  }
  else if (item->HasVideoInfoTag())
  {
    id = item->GetVideoInfoTag()->m_iDbId;

    // TODO: Can be removed once this is properly handled when starting playback of a file
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
        if (videodatabase.LoadVideoInfo(path, *item->GetVideoInfoTag()))
          id = item->GetVideoInfoTag()->m_iDbId;

        videodatabase.Close();
      }
    }

    if (!item->GetVideoInfoTag()->m_type.empty())
      type = item->GetVideoInfoTag()->m_type;
    else
      CVideoDatabase::VideoContentTypeToString((VIDEODB_CONTENT_TYPE)item->GetVideoContentType(), type);

    if (id <= 0)
    {
      // TODO: Can be removed once this is properly handled when starting playback of a file
      item->SetProperty(LOOKUP_PROPERTY, false);

      std::string title = item->GetVideoInfoTag()->m_strTitle;
      if (title.empty())
        title = item->GetLabel();
      object["item"]["title"] = title;

      switch (item->GetVideoContentType())
      {
      case VIDEODB_CONTENT_MOVIES:
        if (item->GetVideoInfoTag()->m_iYear > 0)
          object["item"]["year"] = item->GetVideoInfoTag()->m_iYear;
        break;
      case VIDEODB_CONTENT_EPISODES:
        if (item->GetVideoInfoTag()->m_iEpisode >= 0)
          object["item"]["episode"] = item->GetVideoInfoTag()->m_iEpisode;
        if (item->GetVideoInfoTag()->m_iSeason >= 0)
          object["item"]["season"] = item->GetVideoInfoTag()->m_iSeason;
        if (!item->GetVideoInfoTag()->m_strShowTitle.empty())
          object["item"]["showtitle"] = item->GetVideoInfoTag()->m_strShowTitle;
        break;
      case VIDEODB_CONTENT_MUSICVIDEOS:
        if (!item->GetVideoInfoTag()->m_strAlbum.empty())
          object["item"]["album"] = item->GetVideoInfoTag()->m_strAlbum;
        if (!item->GetVideoInfoTag()->m_artist.empty())
          object["item"]["artist"] = StringUtils::Join(item->GetVideoInfoTag()->m_artist, " / ");
        break;
      }
    }
  }
  else if (item->HasMusicInfoTag())
  {
    id = item->GetMusicInfoTag()->GetDatabaseId();
    type = MediaTypeSong;

    // TODO: Can be removed once this is properly handled when starting playback of a file
    if (id <= 0 && !item->GetPath().empty() &&
       (!item->HasProperty(LOOKUP_PROPERTY) || item->GetProperty(LOOKUP_PROPERTY).asBoolean()))
    {
      CMusicDatabase musicdatabase;
      if (musicdatabase.Open())
      {
        CSong song;
        if (musicdatabase.GetSongByFileName(item->GetPath(), song, item->m_lStartOffset))
        {
          item->GetMusicInfoTag()->SetSong(song);
          id = item->GetMusicInfoTag()->GetDatabaseId();
        }

        musicdatabase.Close();
      }
    }

    if (id <= 0)
    {
      // TODO: Can be removed once this is properly handled when starting playback of a file
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
  else if (item->IsVideo())
  {
    // video item but has no video info tag.
    type = "movies";
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

  Announce(flag, sender, message, object);
}
