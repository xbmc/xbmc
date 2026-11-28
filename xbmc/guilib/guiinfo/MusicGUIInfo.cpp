/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/MusicGUIInfo.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayLists.h"
#include "application/ApplicationPlayer.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoHelper.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "guilib/guiinfo/GUIInfoUtils.h"
#include "music/MusicFileItemClassify.h"
#include "music/MusicInfoLoader.h"
#include "music/MusicThumbLoader.h"
#include "music/tags/MusicInfoTag.h"
#include "network/NetworkFileItemClassify.h"
#include "playlists/PlayList.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/ArtTypes.h"
#include "utils/DefaultArt.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <mutex>

using namespace KODI;
using namespace KODI::GUILIB;
using namespace KODI::GUILIB::GUIINFO;
using namespace MUSIC_INFO;

CMusicGUIInfo::CMusicGUIInfo() : m_playLists(CServiceBroker::GetPlayLists())
{
}

bool CMusicGUIInfo::InitCurrentItem(CFileItem* item)
{
  const auto appPlayer = CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>();
  if (item &&
      (MUSIC::IsAudio(*item) || (NETWORK::IsInternetStream(*item) && appPlayer->IsPlayingAudio())))
  {
    CLog::Log(LOGDEBUG, "CMusicGUIInfo::InitCurrentItem({})", item->GetPath());

    item->LoadMusicTag();

    CMusicInfoTag* tag =
        item->GetMusicInfoTag(); // creates item if not yet set, so no nullptr checks needed
    tag->SetLoaded(true);

    // find a thumb for this file.
    if (NETWORK::IsInternetStream(*item) && !MUSIC::IsMusicDb(*item))
    {
      if (const std::string sourcePath = m_playLists->GetPlayingSourcePath(); !sourcePath.empty())
      {
        CLog::Log(LOGDEBUG, "Streaming media detected... using {} to find a thumb", sourcePath);
        CFileItem streamingItem(sourcePath, false);

        CMusicThumbLoader loader;
        loader.FillThumb(streamingItem);
        if (streamingItem.HasArt(ART::TYPE::THUMB))
          item->SetArt(ART::TYPE::THUMB, streamingItem.GetArt(ART::TYPE::THUMB));
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

bool CMusicGUIInfo::GetLabel(std::string& value,
                             const CFileItem* item,
                             int contextWindow,
                             const CGUIInfo& info,
                             std::string* fallback) const
{
  if (GUIINFO::IsPlayListEntryInfo(info, MUSICPLAYER_OFFSET_POSITION_FIRST,
                                   MUSICPLAYER_OFFSET_POSITION_LAST))
    return GetPlaylistInfo(value, info);

  const CMusicInfoTag* tag = item->GetMusicInfoTag();
  if (tag)
  {
    switch (info.GetInfo())
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
        value = GUIINFO::GetFileInfoLabelValueFromPath(info.GetInfo(), value);
        return true;
      case PLAYER_TITLE:
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
      {
        const std::string sep{info.GetData3().empty() ? CServiceBroker::GetSettingsComponent()
                                                            ->GetAdvancedSettings()
                                                            ->m_musicItemSeparator
                                                      : info.GetData3()};
        value = StringUtils::Join(tag->GetGenre(), sep);
        return true;
      }
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
            value = StringUtils::Format(
                CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20350),
                StringUtils::FormatNumber(rating), StringUtils::FormatNumber(votes));
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
          value = StringUtils::SecondsToTimeString(
              iDuration,
              static_cast<TIME_FORMAT>(info.GetInfo() == LISTITEM_DURATION ? info.GetData4()
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
        const auto formatted{
            CGUIInfoUtils::FormatAudioChannels(info.GetData3(), tag->GetNoOfChannels())};

        if (formatted.has_value())
        {
          value = formatted.value();
          return true;
        }
        break;
      }
      case LISTITEM_ALBUMSTATUS:
        value = tag->GetAlbumReleaseStatus();
        return true;
      case LISTITEM_FILENAME:
      case LISTITEM_FILE_EXTENSION:
      case LISTITEM_FILENAME_NO_EXTENSION:
        if (MUSIC::IsMusicDb(*item))
          value = URIUtils::GetFileName(tag->GetURL());
        else if (
            item->HasVideoInfoTag()) // special handling for music videos, which have both a videotag and a musictag
          break;
        else
          value = URIUtils::GetFileName(item->GetPath());

        if (info.GetInfo() == LISTITEM_FILE_EXTENSION)
        {
          std::string strExtension = URIUtils::GetExtension(value);
          value = StringUtils::TrimLeft(strExtension, ".");
        }
        else if (info.GetInfo() == LISTITEM_FILENAME_NO_EXTENSION)
        {
          URIUtils::RemoveExtension(value);
        }
        return true;
      case LISTITEM_FOLDERNAME:
      case LISTITEM_PATH:
        if (MUSIC::IsMusicDb(*item))
          value = URIUtils::GetDirectory(tag->GetURL());
        else if (
            item->HasVideoInfoTag()) // special handling for music videos, which have both a videotag and a musictag
          break;
        else
          URIUtils::GetParentPath(item->GetPath(), value);

        value = CURL(value).GetWithoutUserDetails();

        if (info.GetInfo() == LISTITEM_FOLDERNAME)
        {
          URIUtils::RemoveSlashAtEnd(value);
          value = URIUtils::GetFileName(value);
        }
        return true;
      case LISTITEM_FILENAME_AND_PATH:
        if (MUSIC::IsMusicDb(*item))
          value = tag->GetURL();
        else if (
            item->HasVideoInfoTag()) // special handling for music videos, which have both a videotag and a musictag
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
      default:
        break;
    }
  }

  switch (info.GetInfo())
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
      if (m_playLists->GetPlayingType() == PLAYLIST::Audio)
      {
        value = GUIINFO::GetPlayListLengthLabel(*m_playLists, PLAYLIST::Audio);
        return true;
      }
      break;
    case MUSICPLAYER_PLAYLISTPOS:
      value = GUIINFO::GetPlayListPositionLabel(*m_playLists, PLAYLIST::Audio);
      return true;
    case MUSICPLAYER_COVER:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (appPlayer->IsPlayingAudio())
      {
        if (fallback)
          *fallback = ART::DEFAULT::ALBUM_COVER;
        value = item->HasArt(ART::TYPE::THUMB) ? item->GetArt(ART::TYPE::THUMB)
                                               : ART::DEFAULT::ALBUM_COVER;
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
      const auto formatted{
          CGUIInfoUtils::FormatAudioChannels(info.GetData3(), m_audioInfo.channels)};

      if (formatted.has_value())
      {
        value = formatted.value();
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
    default:
      break;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////
  // MUSICPM_*
  ///////////////////////////////////////////////////////////////////////////////////////////////
  if (GetPartyModeLabel(value, info))
    return true;

  return false;
}

bool CMusicGUIInfo::GetPartyModeLabel(std::string& value, const CGUIInfo& info) const
{
  int entries = -1;

  switch (info.GetInfo())
  {
    case MUSICPM_MATCHINGENTRIES:
      entries = m_playLists->GetFeedTotal();
      break;
    case MUSICPM_MATCHINGENTRIESLEFT:
      entries = m_playLists->GetFeedLeft();
      break;
    default:
      break;
  }

  if (entries >= 0)
  {
    value = std::to_string(entries);
    return true;
  }

  return false;
}

bool CMusicGUIInfo::GetPlaylistInfo(std::string& value, const CGUIInfo& info) const
{
  const auto found = GUIINFO::GetPlayListEntry(*m_playLists, PLAYLIST::Audio, info);
  if (!found)
    return false;

  if (info.GetInfo() == MUSICPLAYER_PLAYLISTPOS)
  {
    value = std::to_string(
        m_playLists->GetPlayList(PLAYLIST::Audio).GetPlayOrderPosition(found->entry) + 1);
    return true;
  }

  // The path labels are answered from the item itself. Loading the tag for them opens a database
  // connection, which is expensive on a remote database.
  if (info.GetInfo() == PLAYER_PATH || info.GetInfo() == PLAYER_FILEPATH ||
      info.GetInfo() == PLAYER_FILENAME)
  {
    if (GetLabel(value, found->item.get(), 0, CGUIInfo(info.GetInfo()), nullptr))
      return true;
    return GUIINFO::GetFileFallbackLabel(value, *found->item, info.GetInfo());
  }

  const CFileItemPtr playlistItem =
      GUIINFO::LookUpOnce(*m_playLists, PLAYLIST::Audio, *found, m_lookedUp,
                          [](CFileItem& item)
                          {
                            if (item.HasMusicInfoTag() && !item.GetMusicInfoTag()->Loaded())
                            {
                              item.LoadMusicTag();
                              item.GetMusicInfoTag()->SetLoaded();
                            }
                            if (!item.HasArt(ART::TYPE::THUMB))
    {
      CMusicThumbLoader loader;
      loader.LoadItem(&item);
                            }
                          });

  if (info.GetInfo() == MUSICPLAYER_COVER)
  {
    value = playlistItem->HasArt(ART::TYPE::THUMB) ? playlistItem->GetArt(ART::TYPE::THUMB)
                                                   : ART::DEFAULT::ALBUM_COVER;
    return true;
  }

  if (GetLabel(value, playlistItem.get(), 0, CGUIInfo(info.GetInfo()), nullptr))
    return true;
  return GUIINFO::GetFileFallbackLabel(value, *playlistItem, info.GetInfo());
}

bool CMusicGUIInfo::GetFallbackLabel(std::string& value,
                                     const CFileItem* item,
                                     int contextWindow,
                                     const CGUIInfo& info,
                                     std::string* fallback)
{
  if (GUIINFO::IsPlayListEntryInfo(info, MUSICPLAYER_OFFSET_POSITION_FIRST,
                                   MUSICPLAYER_OFFSET_POSITION_LAST))
    return false;

  const CMusicInfoTag* tag = item->GetMusicInfoTag();
  if (tag)
  {
    switch (info.GetInfo())
    {
      /////////////////////////////////////////////////////////////////////////////////////////////
      // MUSICPLAYER_*
      /////////////////////////////////////////////////////////////////////////////////////////////
      case MUSICPLAYER_TITLE:
        return GUIINFO::GetFileFallbackLabel(value, *item, info.GetInfo());
      default:
        break;
    }
  }
  return false;
}

bool CMusicGUIInfo::GetInt(int& value,
                           const CGUIListItem* gitem,
                           int contextWindow,
                           const CGUIInfo& info) const
{
  return false;
}

bool CMusicGUIInfo::GetBool(bool& value,
                            const CGUIListItem* gitem,
                            int contextWindow,
                            const CGUIInfo& info) const
{
  const auto* item{static_cast<const CFileItem*>(gitem)};
  const CMusicInfoTag* tag = item->GetMusicInfoTag();

  switch (info.GetInfo())
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // MUSICPLAYER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case MUSICPLAYER_CONTENT:
      value = StringUtils::EqualsNoCase(info.GetData3(), "files");
      return value; // if no match for this provider, other providers shall be asked.
    case MUSICPLAYER_HASPREVIOUS:
      value = m_playLists->HasPrevious(PLAYLIST::Audio);
      return true;
    case MUSICPLAYER_HASNEXT:
      value = m_playLists->HasNext(PLAYLIST::Audio);
      return true;
    case MUSICPLAYER_PLAYLISTPLAYING:
    {
      if (m_playLists->GetPlayingType() == PLAYLIST::Audio)
      {
        value = true;
        return true;
      }
      break;
    }
    case MUSICPLAYER_EXISTS:
    {
      const int index = info.GetData2();
      if (info.GetData1() == 1)
      { // relative index
        value = m_playLists->HasEntry(PLAYLIST::Audio, index);
        return true;
      }
      value = (index >= 0 && index < m_playLists->GetPlayList(PLAYLIST::Audio).Size());
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
      value = m_playLists->GetFedType().has_value();
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
    default:
      break;
  }

  return false;
}
