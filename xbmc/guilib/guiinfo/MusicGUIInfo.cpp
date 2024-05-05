/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/MusicGUIInfo.h"

#include "FileItem.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoHelper.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "music/MusicFileItemClassify.h"
#include "music/MusicInfoLoader.h"
#include "music/MusicThumbLoader.h"
#include "music/tags/MusicInfoTag.h"
#include "network/NetworkFileItemClassify.h"
#include "playlists/PlayList.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace KODI::GUILIB;
using namespace KODI::GUILIB::GUIINFO;
using namespace MUSIC_INFO;

bool CMusicGUIInfo::InitCurrentItem(CFileItem *item)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (item &&
      (MUSIC::IsAudio(*item) || (NETWORK::IsInternetStream(*item) && appPlayer->IsPlayingAudio())))
  {
    CLog::Log(LOGDEBUG, "CMusicGUIInfo::InitCurrentItem({})", item->GetPath());

    item->LoadMusicTag();

    CMusicInfoTag* tag = item->GetMusicInfoTag(); // creates item if not yet set, so no nullptr checks needed
    tag->SetLoaded(true);

    // find a thumb for this file.
    if (NETWORK::IsInternetStream(*item) && !MUSIC::IsMusicDb(*item))
    {
      if (!g_application.m_strPlayListFile.empty())
      {
        CLog::Log(LOGDEBUG, "Streaming media detected... using {} to find a thumb",
                  g_application.m_strPlayListFile);
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
  if (info.GetData1() && ((info.m_info >= MUSICPLAYER_OFFSET_POSITION_FIRST &&
      info.m_info <= MUSICPLAYER_OFFSET_POSITION_LAST) ||
      (info.m_info >= PLAYER_OFFSET_POSITION_FIRST && info.m_info <= PLAYER_OFFSET_POSITION_LAST)))
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
        return !value.empty();
      case LISTITEM_TITLE:
        value = tag->GetTitle();
        return true;
      case MUSICPLAYER_PLAYCOUNT:
      case LISTITEM_PLAYCOUNT:
        if (tag->GetPlayCount() > 0)
        {
          value = std::to_string(tag->GetPlayCount());
          return true;
        }
        break;
      case MUSICPLAYER_LASTPLAYED:
      case LISTITEM_LASTPLAYED:
      {
        const CDateTime& dateTime = tag->GetLastPlayed();
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
          value = StringUtils::Format("{:02}", tag->GetTrackNumber());
          return true;
        }
        break;
      case MUSICPLAYER_DISC_NUMBER:
      case LISTITEM_DISC_NUMBER:
        if (tag->GetDiscNumber() > 0)
        {
          value = std::to_string(tag->GetDiscNumber());
          return true;
        }
        break;
      case MUSICPLAYER_TOTALDISCS:
      case LISTITEM_TOTALDISCS:
        value = std::to_string(tag->GetTotalDiscs());
        return true;
      case MUSICPLAYER_DISC_TITLE:
      case LISTITEM_DISC_TITLE:
        value = tag->GetDiscSubtitle();
        return true;
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
            value =
                StringUtils::Format(g_localizeStrings.Get(20350), StringUtils::FormatNumber(rating),
                                    StringUtils::FormatNumber(votes));
          return true;
        }
        break;
      }
      case MUSICPLAYER_USER_RATING:
      case LISTITEM_USER_RATING:
        if (tag->GetUserrating() > 0)
        {
          value = std::to_string(tag->GetUserrating());
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
          value = std::to_string(dbId);
          return true;
        }
        break;
      }
      case PLAYER_DURATION:
      {
        const auto& components = CServiceBroker::GetAppComponents();
        const auto appPlayer = components.GetComponent<CApplicationPlayer>();
        if (!appPlayer->IsPlayingAudio())
          break;
      }
        [[fallthrough]];
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
      case MUSICPLAYER_BPM:
      case LISTITEM_BPM:
        if (tag->GetBPM() > 0)
        {
          value = std::to_string(tag->GetBPM());
          return true;
        }
        break;
      case MUSICPLAYER_STATIONNAME:
        // This property can be used for example by addons to enforce/override the station name.
        value = item->GetProperty("StationName").asString();
        if (value.empty())
          value = tag->GetStationName();
        return true;

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
      case MUSICPLAYER_ORIGINALDATE:
      case LISTITEM_ORIGINALDATE:
      {
        value = tag->GetOriginalDate();
        if (!CServiceBroker::GetSettingsComponent()
                ->GetAdvancedSettings()
                ->m_bMusicLibraryUseISODates)
          value = StringUtils::ISODateToLocalizedDate(value);
        return true;
      }
      case MUSICPLAYER_RELEASEDATE:
      case LISTITEM_RELEASEDATE:
      {
        value = tag->GetReleaseDate();
        if (!CServiceBroker::GetSettingsComponent()
                ->GetAdvancedSettings()
                ->m_bMusicLibraryUseISODates)
          value = StringUtils::ISODateToLocalizedDate(value);
        return true;
      }
      break;
      case LISTITEM_BITRATE:
      {
        int BitRate = tag->GetBitRate();
        if (BitRate > 0)
        {
          value = std::to_string(BitRate);
          return true;
        }
        break;
      }
      case LISTITEM_SAMPLERATE:
      {
        int sampleRate = tag->GetSampleRate();
        if (sampleRate > 0)
        {
          value = StringUtils::Format("{:.5}", static_cast<double>(sampleRate) / 1000.0);
          return true;
        }
        break;
      }
      case LISTITEM_MUSICCHANNELS:
      {
        int channels = tag->GetNoOfChannels();
        if (channels > 0)
        {
          value = std::to_string(channels);
          return true;
        }
        break;
      }
      case LISTITEM_ALBUMSTATUS:
        value = tag->GetAlbumReleaseStatus();
        return true;
      case LISTITEM_FILENAME:
      case LISTITEM_FILE_EXTENSION:
        if (MUSIC::IsMusicDb(*item))
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
        if (MUSIC::IsMusicDb(*item))
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
        if (MUSIC::IsMusicDb(*item))
          value = tag->GetURL();
        else if (item->HasVideoInfoTag()) // special handling for music videos, which have both a videotag and a musictag
          break;
        else
          value = item->GetPath();

        value = CURL(value).GetWithoutUserDetails();
        return true;
      case LISTITEM_DATE_ADDED:
        if (tag->GetDateAdded().IsValid())
        {
          value = tag->GetDateAdded().GetAsLocalizedDate();
          return true;
        }
        break;
      case LISTITEM_SONG_VIDEO_URL:
        value = tag->GetSongVideoURL();
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
      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_MUSIC)
      {
        value = GUIINFO::GetPlaylistLabel(PLAYLIST_LENGTH);
        return true;
      }
      break;
    case MUSICPLAYER_PLAYLISTPOS:
      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_MUSIC)
      {
        value = GUIINFO::GetPlaylistLabel(PLAYLIST_POSITION);
        return true;
      }
      break;
    case MUSICPLAYER_COVER:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (appPlayer->IsPlayingAudio())
      {
        if (fallback)
          *fallback = "DefaultAlbumCover.png";
        value = item->HasArt("thumb") ? item->GetArt("thumb") : "DefaultAlbumCover.png";
        return true;
      }
      break;
    }
    case MUSICPLAYER_BITRATE:
    {
      int iBitrate = m_audioInfo.bitrate;
      if (iBitrate > 0)
      {
        value = std::to_string(std::lrint(static_cast<double>(iBitrate) / 1000.0));
        return true;
      }
      break;
    }
    case MUSICPLAYER_CHANNELS:
    {
      int iChannels = m_audioInfo.channels;
      if (iChannels > 0)
      {
        value = std::to_string(iChannels);
        return true;
      }
      break;
    }
    case MUSICPLAYER_BITSPERSAMPLE:
    {
      int iBPS = m_audioInfo.bitspersample;
      if (iBPS > 0)
      {
        value = std::to_string(iBPS);
        return true;
      }
      break;
    }
    case MUSICPLAYER_SAMPLERATE:
    {
      int iSamplerate = m_audioInfo.samplerate;
      if (iSamplerate > 0)
      {
        value = StringUtils::Format("{:.5}", static_cast<double>(iSamplerate) / 1000.0);
        return true;
      }
      break;
    }
    case MUSICPLAYER_CODEC:
      value = m_audioInfo.codecName;
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
    value = std::to_string(iSongs);
    return true;
  }

  return false;
}

bool CMusicGUIInfo::GetPlaylistInfo(std::string& value, const CGUIInfo &info) const
{
  const PLAYLIST::CPlayList& playlist =
      CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST::TYPE_MUSIC);
  if (playlist.size() < 1)
    return false;

  int index = info.GetData2();
  if (info.GetData1() == 1)
  { // relative index (requires current playlist is TYPE_MUSIC)
    if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() != PLAYLIST::TYPE_MUSIC)
      return false;

    index = CServiceBroker::GetPlaylistPlayer().GetNextItemIdx(index);
  }

  if (index < 0 || index >= playlist.size())
    return false;

  const CFileItemPtr playlistItem = playlist[index];
  if (playlistItem->HasMusicInfoTag() && !playlistItem->GetMusicInfoTag()->Loaded())
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
    value = std::to_string(index + 1);
    return true;
  }
  else if (info.m_info == MUSICPLAYER_COVER)
  {
    value = playlistItem->GetArt("thumb");
    return true;
  }

  return GetLabel(value, playlistItem.get(), 0, CGUIInfo(info.m_info), nullptr);
}

bool CMusicGUIInfo::GetFallbackLabel(std::string& value,
                                     const CFileItem* item,
                                     int contextWindow,
                                     const CGUIInfo& info,
                                     std::string* fallback)
{
  // No fallback for musicplayer "offset" and "position" info labels
  if (info.GetData1() && ((info.m_info >= MUSICPLAYER_OFFSET_POSITION_FIRST &&
      info.m_info <= MUSICPLAYER_OFFSET_POSITION_LAST) ||
      (info.m_info >= PLAYER_OFFSET_POSITION_FIRST && info.m_info <= PLAYER_OFFSET_POSITION_LAST)))
    return false;

  const CMusicInfoTag* tag = item->GetMusicInfoTag();
  if (tag)
  {
    switch (info.m_info)
    {
      /////////////////////////////////////////////////////////////////////////////////////////////
      // MUSICPLAYER_*
      /////////////////////////////////////////////////////////////////////////////////////////////
      case MUSICPLAYER_TITLE:
        value = item->GetLabel();
        if (value.empty())
          value = CUtil::GetTitleFromPath(item->GetPath());
        return true;
      default:
        break;
    }
  }
  return false;
}

bool CMusicGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  return false;
}

bool CMusicGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  const CFileItem* item = static_cast<const CFileItem*>(gitem);
  const CMusicInfoTag* tag = item->GetMusicInfoTag();

  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // MUSICPLAYER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case MUSICPLAYER_CONTENT:
      value = StringUtils::EqualsNoCase(info.GetData3(), "files");
      return value; // if no match for this provider, other providers shall be asked.
    case MUSICPLAYER_HASPREVIOUS:
      // requires current playlist be TYPE_MUSIC
      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_MUSIC)
      {
        value = (CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx() > 0); // not first song
        return true;
      }
      break;
    case MUSICPLAYER_HASNEXT:
      // requires current playlist be TYPE_MUSIC
      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_MUSIC)
      {
        value = (CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx() <
                 (CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST::TYPE_MUSIC).size() -
                  1)); // not last song
        return true;
      }
      break;
    case MUSICPLAYER_PLAYLISTPLAYING:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (appPlayer->IsPlayingAudio() &&
          CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_MUSIC)
      {
        value = true;
        return true;
      }
      break;
    }
    case MUSICPLAYER_EXISTS:
    {
      int index = info.GetData2();
      if (info.GetData1() == 1)
      { // relative index
        if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() != PLAYLIST::TYPE_MUSIC)
        {
          value = false;
          return true;
        }
        index += CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx();
      }
      value =
          (index >= 0 &&
           index < CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST::TYPE_MUSIC).size());
      return true;
    }
    case MUSICPLAYER_ISMULTIDISC:
      if (tag)
      {
        value = (item->GetMusicInfoTag()->GetTotalDiscs() > 1);
        return true;
      }
      break;
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // MUSICPM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case MUSICPM_ENABLED:
      value = g_partyModeManager.IsEnabled();
      return true;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // LISTITEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case LISTITEM_IS_BOXSET:
      if (tag)
      {
        value = tag->GetBoxset() == true;
        return true;
      }
      break;
  }

  return false;
}
