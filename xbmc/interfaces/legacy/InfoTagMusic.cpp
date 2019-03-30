/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InfoTagMusic.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    InfoTagMusic::InfoTagMusic()
    {
      infoTag = new MUSIC_INFO::CMusicInfoTag();
    }

    InfoTagMusic::InfoTagMusic(const MUSIC_INFO::CMusicInfoTag& tag)
    {
      infoTag = new MUSIC_INFO::CMusicInfoTag();
      *infoTag = tag;
    }

    InfoTagMusic::~InfoTagMusic()
    {
      delete infoTag;
    }

    int InfoTagMusic::getDbId()
    {
      return infoTag->GetDatabaseId();
    }

    String InfoTagMusic::getURL()
    {
      return infoTag->GetURL();
    }

    String InfoTagMusic::getTitle()
    {
      return infoTag->GetTitle();
    }

    String InfoTagMusic::getMediaType()
    {
      return infoTag->GetType();
    }

    String InfoTagMusic::getArtist()
    {
      return infoTag->GetArtistString();
    }

    String InfoTagMusic::getAlbumArtist()
    {
      return infoTag->GetAlbumArtistString();
    }

    String InfoTagMusic::getAlbum()
    {
      return infoTag->GetAlbum();
    }

    String InfoTagMusic::getGenre()
    {
      return StringUtils::Join(infoTag->GetGenre(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
    }

    int InfoTagMusic::getDuration()
    {
      return infoTag->GetDuration();
    }

    int InfoTagMusic::getRating()
    {
      return infoTag->GetRating();
    }

    int InfoTagMusic::getUserRating()
    {
      return infoTag->GetUserrating();
    }

    int InfoTagMusic::getTrack()
    {
      return infoTag->GetTrackNumber();
    }

    int InfoTagMusic::getDisc()
    {
      return infoTag->GetDiscNumber();
    }

    String InfoTagMusic::getReleaseDate()
    {
      return infoTag->GetReleaseDate();
    }

    int InfoTagMusic::getListeners()
    {
      return infoTag->GetListeners();
    }

    int InfoTagMusic::getPlayCount()
    {
      return infoTag->GetPlayCount();
    }

    String InfoTagMusic::getLastPlayed()
    {
      return infoTag->GetLastPlayed().GetAsLocalizedDate();
    }

    String InfoTagMusic::getComment()
    {
      return infoTag->GetComment();
    }

    String InfoTagMusic::getLyrics()
    {
      return infoTag->GetLyrics();
    }

    String InfoTagMusic::getMusicBrainzTrackID()
    {
      return infoTag->GetMusicBrainzTrackID();
    }

    std::vector<String> InfoTagMusic::getMusicBrainzArtistID()
    {
      return infoTag->GetMusicBrainzArtistID();
    }

    String InfoTagMusic::getMusicBrainzAlbumID()
    {
      return infoTag->GetMusicBrainzAlbumID();
    }

    String InfoTagMusic::getMusicBrainzReleaseGroupID()
    {
      return infoTag->GetMusicBrainzReleaseGroupID();
    }

    std::vector<String> InfoTagMusic::getMusicBrainzAlbumArtistID()
    {
      return infoTag->GetMusicBrainzAlbumArtistID();
    }
  }
}

