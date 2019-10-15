/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/MusicGUIInfo.h"

#include "Application.h"
#include "FileItem.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "URL.h"
#include "Util.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicInfoLoader.h"
#include "music/MusicThumbLoader.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayList.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoHelper.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

using namespace KODI::GUILIB;
using namespace KODI::GUILIB::GUIINFO;
using namespace MUSIC_INFO;

bool CMusicGUIInfo::InitCurrentItem(CFileItem *item)
{
  if (item && (item->IsAudio() || (item->IsInternetStream() && g_application.GetAppPlayer().IsPlayingAudio())))
  {
    CLog::Log(LOGDEBUG,"CMusicGUIInfo::InitCurrentItem(%s)", item->GetPath().c_str());

    item->LoadMusicTag();

    CMusicInfoTag* tag = item->GetMusicInfoTag(); // creates item if not yet set, so no nullptr checks needed
    if (tag->GetTitle().empty())
    {
      // No title in tag, show filename only
      tag->SetTitle(CUtil::GetTitleFromPath(item->GetPath()));
    }
    tag->SetLoaded(true);

    // find a thumb for this file.
    if (item->IsInternetStream() && !item->IsMusicDb())
    {
      if (!g_application.m_strPlayListFile.empty())
      {
        CLog::Log(LOGDEBUG,"Streaming media detected... using %s to find a thumb", g_application.m_strPlayListFile.c_str());
        CFileItem streamingItem(g_application.m_strPlayListFile,false);

        CMusicThumbLoader loader;
        loader.FillThumb(streamingItem);
        if (streamingItem.HasArt("thumb"))
          item->SetArt("thumb", streamingItem.GetArt("thumb"));
      }
    }
    else
    {
      CMusicThumbLoader loader;
      loader.LoadItem(item);
    }

    CMusicInfoLoader::LoadAdditionalTagInfo(item);
    return true;
  }
  return false;
}

bool CMusicGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  // For musicplayer "offset" and "position" info labels check playlist
  if (info.GetData1() && info.m_info >= MUSICPLAYER_OFFSET_POSITION_FIRST &&
      info.m_info <= MUSICPLAYER_OFFSET_POSITION_LAST)
    return GetPlaylistInfo(value, info);

  const CMusicInfoTag* tag = item->GetMusicInfoTag();
  if (tag)
  {
    switch (info.m_info)
    {
      /////////////////////////////////////////////////////////////////////////////////////////////
      // PLAYER_* / MUSICPLAYER_* / LISTITEM_*
      /////////////////////////////////////////////////////////////////////////////////////////////
      case PLAYER_PATH:
      case PLAYER_FILENAME:
      case PLAYER_FILEPATH:
        value = tag->GetURL();
        if (value.empty())
          value = item->GetPath();
        value = GUIINFO::GetFileInfoLabelValueFromPath(info.m_info, value);
        return true;
      case PLAYER_TITLE:
        value = tag->GetTitle();
        return !value.empty();
      case MUSICPLAYER_TITLE:
        value = tag->GetTitle();
        if (value.empty())
          value = item->GetLabel();
        if (value.empty())
          value = CUtil::GetTitleFromPath(item->GetPath());
        return true;
      case LISTITEM_TITLE:
        value = tag->GetTitle();
        return true;
      case MUSICPLAYER_PLAYCOUNT:
      case LISTITEM_PLAYCOUNT:
        if (tag->GetPlayCount() > 0)
        {
          value = StringUtils::Format("%i", tag->GetPlayCount());
          return true;
        }
        break;
      case MUSICPLAYER_LASTPLAYED:
      case LISTITEM_LASTPLAYED:
      {
        const CDateTime dateTime = tag->GetLastPlayed();
        if (dateTime.IsValid())
        {
          value = dateTime.GetAsLocalizedDate();
          return true;
        }
        break;
      }
      case MUSICPLAYER_TRACK_NUMBER:
      case LISTITEM_TRACKNUMBER:
        if (tag->Loaded() && tag->GetTrackNumber() > 0)
        {
          value = StringUtils::Format("%02i", tag->GetTrackNumber());
          return true;
        }
        break;
      case MUSICPLAYER_DISC_NUMBER:
      case LISTITEM_DISC_NUMBER:
        if (tag->GetDiscNumber() > 0)
        {
          value = StringUtils::Format("%i", tag->GetDiscNumber());
          return true;
        }
        break;
      case MUSICPLAYER_ARTIST:
      case LISTITEM_ARTIST:
        value = tag->GetArtistString();
        return true;
      case MUSICPLAYER_ALBUM_ARTIST:
      case LISTITEM_ALBUM_ARTIST:
        value = tag->GetAlbumArtistString();
        return true;
      case MUSICPLAYER_CONTRIBUTORS:
      case LISTITEM_CONTRIBUTORS:
        if (tag->HasContributors())
        {
          value = tag->GetContributorsText();
          return true;
        }
        break;
      case MUSICPLAYER_CONTRIBUTOR_AND_ROLE:
      case LISTITEM_CONTRIBUTOR_AND_ROLE:
        if (tag->HasContributors())
        {
          value = tag->GetContributorsAndRolesText();
          return true;
        }
        break;
      case MUSICPLAYER_ALBUM:
      case LISTITEM_ALBUM:
        value = tag->GetAlbum();
        return true;
      case MUSICPLAYER_YEAR:
      case LISTITEM_YEAR:
        value = tag->GetYearString();
        return true;
      case MUSICPLAYER_GENRE:
      case LISTITEM_GENRE:
        value =  StringUtils::Join(tag->GetGenre(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
        return true;
      case MUSICPLAYER_LYRICS:
        value = tag->GetLyrics();
        return true;
      case MUSICPLAYER_RATING:
      case LISTITEM_RATING:
      {
        float rating = tag->GetRating();
        if (rating > 0.f)
        {
          value = StringUtils::FormatNumber(rating);
          return true;
        }
        break;
      }
      case MUSICPLAYER_RATING_AND_VOTES:
      case LISTITEM_RATING_AND_VOTES:
      {
        float rating = tag->GetRating();
        if (rating > 0.f)
        {
          int votes = tag->GetVotes();
          if (votes <= 0)
            value = StringUtils::FormatNumber(rating);
          else
            value = StringUtils::Format(g_localizeStrings.Get(20350).c_str(),
                                        StringUtils::FormatNumber(rating).c_str(),
                                        StringUtils::FormatNumber(votes).c_str());
          return true;
        }
        break;
      }
      case MUSICPLAYER_USER_RATING:
      case LISTITEM_USER_RATING:
        if (tag->GetUserrating() > 0)
        {
          value = StringUtils::Format("%i", tag->GetUserrating());
          return true;
        }
        break;
      case MUSICPLAYER_COMMENT:
      case LISTITEM_COMMENT:
        value = tag->GetComment();
        return true;
      case MUSICPLAYER_MOOD:
      case LISTITEM_MOOD:
        value = tag->GetMood();
        return true;
      case LISTITEM_DBTYPE:
        value = tag->GetType();
        return true;
      case MUSICPLAYER_DBID:
      case LISTITEM_DBID:
      {
        int dbId = tag->GetDatabaseId();
        if (dbId > -1)
        {
          value = StringUtils::Format("%i", dbId);
          return true;
        }
        break;
      }
      case PLAYER_DURATION:
        if (!g_application.GetAppPlayer().IsPlayingAudio())
          break;
        // fall-thru is intended.
      case MUSICPLAYER_DURATION:
      case LISTITEM_DURATION:
      {
        int iDuration = tag->GetDuration();
        if (iDuration > 0)
        {
          value = StringUtils::SecondsToTimeString(iDuration,
                                                   static_cast<TIME_FORMAT>(info.m_info == LISTITEM_DURATION
                                                                            ? info.GetData4()
                                                                            : info.GetData1()));
          return true;
        }
        break;
      }

      /////////////////////////////////////////////////////////////////////////////////////////////
      // LISTITEM_*
      /////////////////////////////////////////////////////////////////////////////////////////////
      case LISTITEM_PROPERTY:
        if (StringUtils::StartsWithNoCase(info.GetData3(), "Role."))
        {
          // "Role.xxxx" properties are held in music tag
          std::string property = info.GetData3();
          property.erase(0, 5); //Remove Role.
          value = tag->GetArtistStringForRole(property);
          return true;
        }
        break;
      case LISTITEM_VOTES:
        value = StringUtils::FormatNumber(tag->GetVotes());
        return true;
      case LISTITEM_FILENAME:
      case LISTITEM_FILE_EXTENSION:
        if (item->IsMusicDb())
          value = URIUtils::GetFileName(tag->GetURL());
        else if (item->HasVideoInfoTag()) // special handling for music videos, which have both a videotag and a musictag
          break;
        else
          value = URIUtils::GetFileName(item->GetPath());

        if (info.m_info == LISTITEM_FILE_EXTENSION)
        {
          std::string strExtension = URIUtils::GetExtension(value);
          value = StringUtils::TrimLeft(strExtension, ".");
        }
        return true;
      case LISTITEM_FOLDERNAME:
      case LISTITEM_PATH:
        if (item->IsMusicDb())
          value = URIUtils::GetDirectory(tag->GetURL());
        else if (item->HasVideoInfoTag()) // special handling for music videos, which have both a videotag and a musictag
          break;
        else
          URIUtils::GetParentPath(item->GetPath(), value);

        value = CURL(value).GetWithoutUserDetails();

        if (info.m_info == LISTITEM_FOLDERNAME)
        {
          URIUtils::RemoveSlashAtEnd(value);
          value = URIUtils::GetFileName(value);
        }
        return true;
      case LISTITEM_FILENAME_AND_PATH:
        if (item->IsMusicDb())
          value = tag->GetURL();
        else if (item->HasVideoInfoTag()) // special handling for music videos, which have both a videotag and a musictag
          break;
        else
          value = item->GetPath();

        value = CURL(value).GetWithoutUserDetails();
        return true;
    }
  }

  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // MUSICPLAYER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case MUSICPLAYER_PROPERTY:
      if (StringUtils::StartsWithNoCase(info.GetData3(), "Role.") && item->HasMusicInfoTag())
      {
        // "Role.xxxx" properties are held in music tag
        std::string property = info.GetData3();
        property.erase(0, 5); //Remove Role.
        value = item->GetMusicInfoTag()->GetArtistStringForRole(property);
        return true;
      }
      value = item->GetProperty(info.GetData3()).asString();
      return true;
    case MUSICPLAYER_PLAYLISTLEN:
      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST_MUSIC)
      {
        value = GUIINFO::GetPlaylistLabel(PLAYLIST_LENGTH);
        return true;
      }
      break;
    case MUSICPLAYER_PLAYLISTPOS:
      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST_MUSIC)
      {
        value = GUIINFO::GetPlaylistLabel(PLAYLIST_POSITION);
        return true;
      }
      break;
    case MUSICPLAYER_COVER:
      if (g_application.GetAppPlayer().IsPlayingAudio())
      {
        if (fallback)
          *fallback = "DefaultAlbumCover.png";
        value = item->HasArt("thumb") ? item->GetArt("thumb") : "DefaultAlbumCover.png";
        return true;
      }
      break;
    case MUSICPLAYER_BITRATE:
    {
      int iBitrate = m_audioInfo.bitrate;
      if (iBitrate > 0)
      {
        value = StringUtils::Format("%li", std::lrint(static_cast<double>(iBitrate) / 1000.0));
        return true;
      }
      break;
    }
    case MUSICPLAYER_CHANNELS:
    {
      int iChannels = m_audioInfo.channels;
      if (iChannels > 0)
      {
        value = StringUtils::Format("%i", iChannels);
        return true;
      }
      break;
    }
    case MUSICPLAYER_BITSPERSAMPLE:
    {
      int iBPS = m_audioInfo.bitspersample;
      if (iBPS > 0)
      {
        value = StringUtils::Format("%i", iBPS);
        return true;
      }
      break;
    }
    case MUSICPLAYER_SAMPLERATE:
    {
      int iSamplerate = m_audioInfo.samplerate;
      if (iSamplerate > 0)
      {
        value = StringUtils::Format("%.5g", static_cast<double>(iSamplerate) / 1000.0);
        return true;
      }
      break;
    }
    case MUSICPLAYER_CODEC:
      value = StringUtils::Format("%s", m_audioInfo.codecName.c_str());
      return true;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////
  // MUSICPM_*
  ///////////////////////////////////////////////////////////////////////////////////////////////
  if (GetPartyModeLabel(value, info))
    return true;

  return false;
}

bool CMusicGUIInfo::GetPartyModeLabel(std::string& value, const CGUIInfo &info) const
{
  int iSongs = -1;

  switch (info.m_info)
  {
    case MUSICPM_SONGSPLAYED:
      iSongs = g_partyModeManager.GetSongsPlayed();
      break;
    case MUSICPM_MATCHINGSONGS:
      iSongs = g_partyModeManager.GetMatchingSongs();
      break;
    case MUSICPM_MATCHINGSONGSPICKED:
      iSongs = g_partyModeManager.GetMatchingSongsPicked();
      break;
    case MUSICPM_MATCHINGSONGSLEFT:
      iSongs = g_partyModeManager.GetMatchingSongsLeft();
      break;
    case MUSICPM_RELAXEDSONGSPICKED:
      iSongs = g_partyModeManager.GetRelaxedSongs();
      break;
    case MUSICPM_RANDOMSONGSPICKED:
      iSongs = g_partyModeManager.GetRandomSongs();
      break;
  }

  if (iSongs >= 0)
  {
    value = StringUtils::Format("%i", iSongs);
    return true;
  }

  return false;
}

bool CMusicGUIInfo::GetPlaylistInfo(std::string& value, const CGUIInfo &info) const
{
  PLAYLIST::CPlayList& playlist = CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST_MUSIC);
  if (playlist.size() < 1)
    return false;

  int index = info.GetData2();
  if (info.GetData1() == 1)
  { // relative index (requires current playlist is PLAYLIST_MUSIC)
    if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() != PLAYLIST_MUSIC)
      return false;

    index = CServiceBroker::GetPlaylistPlayer().GetNextSong(index);
  }

  if (index < 0 || index >= playlist.size())
    return false;

  const CFileItemPtr playlistItem = playlist[index];
  if (!playlistItem->GetMusicInfoTag()->Loaded())
  {
    playlistItem->LoadMusicTag();
    playlistItem->GetMusicInfoTag()->SetLoaded();
  }
  // try to set a thumbnail
  if (!playlistItem->HasArt("thumb"))
  {
    CMusicThumbLoader loader;
    loader.LoadItem(playlistItem.get());
    // still no thumb? then just the set the default cover
    if (!playlistItem->HasArt("thumb"))
      playlistItem->SetArt("thumb", "DefaultAlbumCover.png");
  }
  if (info.m_info == MUSICPLAYER_PLAYLISTPOS)
  {
    value = StringUtils::Format("%i", index + 1);
    return true;
  }
  else if (info.m_info == MUSICPLAYER_COVER)
  {
    value = playlistItem->GetArt("thumb");
    return true;
  }

  return GetLabel(value, playlistItem.get(), 0, CGUIInfo(info.m_info), nullptr);
}

bool CMusicGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  return false;
}

bool CMusicGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // MUSICPLAYER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case MUSICPLAYER_CONTENT:
      value = StringUtils::EqualsNoCase(info.GetData3(), "files");
      return value; // if no match for this provider, other providers shall be asked.
    case MUSICPLAYER_HASPREVIOUS:
      // requires current playlist be PLAYLIST_MUSIC
      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST_MUSIC)
      {
        value = (CServiceBroker::GetPlaylistPlayer().GetCurrentSong() > 0); // not first song
        return true;
      }
      break;
    case MUSICPLAYER_HASNEXT:
      // requires current playlist be PLAYLIST_MUSIC
      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST_MUSIC)
      {
        value = (CServiceBroker::GetPlaylistPlayer().GetCurrentSong() < (CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST_MUSIC).size() - 1)); // not last song
        return true;
      }
      break;
    case MUSICPLAYER_PLAYLISTPLAYING:
      if (g_application.GetAppPlayer().IsPlayingAudio() && CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST_MUSIC)
      {
        value = true;
        return true;
      }
      break;
    case MUSICPLAYER_EXISTS:
    {
      int index = info.GetData2();
      if (info.GetData1() == 1)
      { // relative index
        if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() != PLAYLIST_MUSIC)
        {
          value = false;
          return true;
        }
        index += CServiceBroker::GetPlaylistPlayer().GetCurrentSong();
      }
      value = (index >= 0 && index < CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST_MUSIC).size());
      return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // MUSICPM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case MUSICPM_ENABLED:
      value = g_partyModeManager.IsEnabled();
      return true;
  }

  return false;
}
