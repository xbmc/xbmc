/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InfoTagMusic.h"

#include "AddonUtils.h"
#include "ServiceBroker.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    InfoTagMusic::InfoTagMusic(bool offscreen /* = false */)
      : infoTag(new MUSIC_INFO::CMusicInfoTag()), offscreen(offscreen), owned(true)
    {
    }

    InfoTagMusic::InfoTagMusic(const MUSIC_INFO::CMusicInfoTag* tag) : InfoTagMusic(true)
    {
      *infoTag = *tag;
    }

    InfoTagMusic::InfoTagMusic(MUSIC_INFO::CMusicInfoTag* tag, bool offscreen /* = false */)
      : infoTag(tag), offscreen(offscreen), owned(false)
    {
    }

    InfoTagMusic::~InfoTagMusic()
    {
      if (owned)
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

    std::vector<String> InfoTagMusic::getGenres()
    {
      return infoTag->GetGenre();
    }

    int InfoTagMusic::getDuration()
    {
      return infoTag->GetDuration();
    }

    int InfoTagMusic::getYear()
    {
      return infoTag->GetYear();
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
      CLog::Log(LOGWARNING, "InfoTagMusic.getLastPlayed() is deprecated and might be removed in "
                            "future Kodi versions. Please use InfoTagMusic.getLastPlayedAsW3C().");

      return infoTag->GetLastPlayed().GetAsLocalizedDate();
    }

    String InfoTagMusic::getLastPlayedAsW3C()
    {
      return infoTag->GetLastPlayed().GetAsW3CDateTime();
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

    String InfoTagMusic::getSongVideoURL()
    {
      return infoTag->GetSongVideoURL();
    }

    void InfoTagMusic::setDbId(int dbId, const String& type)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setDbIdRaw(infoTag, dbId, type);
    }

    void InfoTagMusic::setURL(const String& url)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setURLRaw(infoTag, url);
    }

    void InfoTagMusic::setMediaType(const String& mediaType)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setMediaTypeRaw(infoTag, mediaType);
    }

    void InfoTagMusic::setTrack(int track)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setTrackRaw(infoTag, track);
    }

    void InfoTagMusic::setDisc(int disc)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setDiscRaw(infoTag, disc);
    }

    void InfoTagMusic::setDuration(int duration)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setDurationRaw(infoTag, duration);
    }

    void InfoTagMusic::setYear(int year)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setYearRaw(infoTag, year);
    }

    void InfoTagMusic::setReleaseDate(const String& releaseDate)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setReleaseDateRaw(infoTag, releaseDate);
    }

    void InfoTagMusic::setListeners(int listeners)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setListenersRaw(infoTag, listeners);
    }

    void InfoTagMusic::setPlayCount(int playcount)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setPlayCountRaw(infoTag, playcount);
    }

    void InfoTagMusic::setGenres(const std::vector<String>& genres)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setGenresRaw(infoTag, genres);
    }

    void InfoTagMusic::setAlbum(const String& album)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setAlbumRaw(infoTag, album);
    }

    void InfoTagMusic::setArtist(const String& artist)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setArtistRaw(infoTag, artist);
    }

    void InfoTagMusic::setAlbumArtist(const String& albumArtist)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setAlbumArtistRaw(infoTag, albumArtist);
    }

    void InfoTagMusic::setTitle(const String& title)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setTitleRaw(infoTag, title);
    }

    void InfoTagMusic::setRating(float rating)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setRatingRaw(infoTag, rating);
    }

    void InfoTagMusic::setUserRating(int userrating)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setUserRatingRaw(infoTag, userrating);
    }

    void InfoTagMusic::setLyrics(const String& lyrics)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setLyricsRaw(infoTag, lyrics);
    }

    void InfoTagMusic::setLastPlayed(const String& lastPlayed)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setLastPlayedRaw(infoTag, lastPlayed);
    }

    void InfoTagMusic::setMusicBrainzTrackID(const String& musicBrainzTrackID)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setMusicBrainzTrackIDRaw(infoTag, musicBrainzTrackID);
    }

    void InfoTagMusic::setMusicBrainzArtistID(const std::vector<String>& musicBrainzArtistID)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setMusicBrainzArtistIDRaw(infoTag, musicBrainzArtistID);
    }

    void InfoTagMusic::setMusicBrainzAlbumID(const String& musicBrainzAlbumID)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setMusicBrainzAlbumIDRaw(infoTag, musicBrainzAlbumID);
    }

    void InfoTagMusic::setMusicBrainzReleaseGroupID(const String& musicBrainzReleaseGroupID)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setMusicBrainzReleaseGroupIDRaw(infoTag, musicBrainzReleaseGroupID);
    }

    void InfoTagMusic::setMusicBrainzAlbumArtistID(
        const std::vector<String>& musicBrainzAlbumArtistID)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setMusicBrainzAlbumArtistIDRaw(infoTag, musicBrainzAlbumArtistID);
    }

    void InfoTagMusic::setComment(const String& comment)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setCommentRaw(infoTag, comment);
    }

    void InfoTagMusic::setSongVideoURL(const String& songVideoURL)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setSongVideoURLRaw(infoTag, songVideoURL);
    }

    void InfoTagMusic::setDbIdRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int dbId, const String& type)
    {
      infoTag->SetDatabaseId(dbId, type);
    }

    void InfoTagMusic::setURLRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& url)
    {
      infoTag->SetURL(url);
    }

    void InfoTagMusic::setMediaTypeRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& mediaType)
    {
      infoTag->SetType(mediaType);
    }

    void InfoTagMusic::setTrackRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int track)
    {
      infoTag->SetTrackNumber(track);
    }

    void InfoTagMusic::setDiscRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int disc)
    {
      infoTag->SetDiscNumber(disc);
    }

    void InfoTagMusic::setDurationRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int duration)
    {
      infoTag->SetDuration(duration);
    }

    void InfoTagMusic::setYearRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int year)
    {
      infoTag->SetYear(year);
    }

    void InfoTagMusic::setReleaseDateRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                         const String& releaseDate)
    {
      infoTag->SetReleaseDate(releaseDate);
    }

    void InfoTagMusic::setListenersRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int listeners)
    {
      infoTag->SetListeners(listeners);
    }

    void InfoTagMusic::setPlayCountRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int playcount)
    {
      infoTag->SetPlayCount(playcount);
    }

    void InfoTagMusic::setGenresRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                    const std::vector<String>& genres)
    {
      infoTag->SetGenre(genres);
    }

    void InfoTagMusic::setAlbumRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& album)
    {
      infoTag->SetAlbum(album);
    }

    void InfoTagMusic::setArtistRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& artist)
    {
      infoTag->SetArtist(artist);
    }

    void InfoTagMusic::setAlbumArtistRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                         const String& albumArtist)
    {
      infoTag->SetAlbumArtist(albumArtist);
    }

    void InfoTagMusic::setTitleRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& title)
    {
      infoTag->SetTitle(title);
    }

    void InfoTagMusic::setRatingRaw(MUSIC_INFO::CMusicInfoTag* infoTag, float rating)
    {
      infoTag->SetRating(rating);
    }

    void InfoTagMusic::setUserRatingRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int userrating)
    {
      infoTag->SetUserrating(userrating);
    }

    void InfoTagMusic::setLyricsRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& lyrics)
    {
      infoTag->SetLyrics(lyrics);
    }

    void InfoTagMusic::setLastPlayedRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                        const String& lastPlayed)
    {
      infoTag->SetLastPlayed(lastPlayed);
    }

    void InfoTagMusic::setMusicBrainzTrackIDRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                                const String& musicBrainzTrackID)
    {
      infoTag->SetMusicBrainzTrackID(musicBrainzTrackID);
    }

    void InfoTagMusic::setMusicBrainzArtistIDRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                                 const std::vector<String>& musicBrainzArtistID)
    {
      infoTag->SetMusicBrainzArtistID(musicBrainzArtistID);
    }

    void InfoTagMusic::setMusicBrainzAlbumIDRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                                const String& musicBrainzAlbumID)
    {
      infoTag->SetMusicBrainzAlbumID(musicBrainzAlbumID);
    }

    void InfoTagMusic::setMusicBrainzReleaseGroupIDRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                                       const String& musicBrainzReleaseGroupID)
    {
      infoTag->SetMusicBrainzReleaseGroupID(musicBrainzReleaseGroupID);
    }

    void InfoTagMusic::setMusicBrainzAlbumArtistIDRaw(
        MUSIC_INFO::CMusicInfoTag* infoTag, const std::vector<String>& musicBrainzAlbumArtistID)
    {
      infoTag->SetMusicBrainzAlbumArtistID(musicBrainzAlbumArtistID);
    }

    void InfoTagMusic::setCommentRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& comment)
    {
      infoTag->SetComment(comment);
    }

    void InfoTagMusic::setSongVideoURLRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                          const String& songVideoURL)
    {
      infoTag->SetSongVideoURL(songVideoURL);
    }
  }
}

