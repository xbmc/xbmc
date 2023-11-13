/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"

#include <vector>

namespace MUSIC_INFO
{
class CMusicInfoTag;
}

namespace XBMCAddon
{
  namespace xbmc
  {
    //
    /// \defgroup python_InfoTagMusic InfoTagMusic
    /// \ingroup python_xbmc
    /// @{
    /// @brief **Kodi's music info tag class.**
    ///
    /// \python_class{ xbmc.InfoTagMusic([offscreen]) }
    ///
    /// Access and / or modify the music metadata of a ListItem.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// tag = xbmc.Player().getMusicInfoTag()
    ///
    /// title = tag.getTitle()
    /// url   = tag.getURL()
    /// ...
    /// ~~~~~~~~~~~~~
    //
    class InfoTagMusic : public AddonClass
    {
    private:
      MUSIC_INFO::CMusicInfoTag* infoTag;
      bool offscreen;
      bool owned;

    public:
#ifndef SWIG
      explicit InfoTagMusic(const MUSIC_INFO::CMusicInfoTag* tag);
      explicit InfoTagMusic(MUSIC_INFO::CMusicInfoTag* tag, bool offscreen = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ xbmc.InfoTagMusic([offscreen]) }
      /// Create a music info tag.
      ///
      /// @param offscreen            [opt] bool (default `False`) - if GUI based locks should be
      ///                                          avoided. Most of the times listitems are created
      ///                                          offscreen and added later to a container
      ///                                          for display (e.g. plugins) or they are not
      ///                                          even displayed (e.g. python scrapers).
      ///                                          In such cases, there is no need to lock the
      ///                                          GUI when creating the items (increasing your addon
      ///                                          performance).
      ///                                          Note however, that if you are creating listitems
      ///                                          and managing the container itself (e.g using
      ///                                          WindowXML or WindowXMLDialog classes) subsquent
      ///                                          modifications to the item will require locking.
      ///                                          Thus, in such cases, use the default value (`False`).
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Added **offscreen** argument.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// musicinfo = xbmc.InfoTagMusic(offscreen=False)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      InfoTagMusic(...);
#else
      explicit InfoTagMusic(bool offscreen = false);
#endif
      ~InfoTagMusic() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getDbId() }
      /// Get identification number of tag in database.
      ///
      /// @return [integer] database id.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      getDbId();
#else
      int getDbId();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getURL() }
      /// Returns url of source as string from music info tag.
      ///
      /// @return [string] Url of source
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getURL();
#else
      String getURL();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getTitle() }
      /// Returns the title from music as string on info tag.
      ///
      /// @return [string] Music title
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getTitle();
#else
      String getTitle();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getMediaType() }
      /// Get the media type of the music item.
      ///
      /// @return [string] media type
      ///
      /// Available strings about media type for music:
      /// | String         | Description                                       |
      /// |---------------:|:--------------------------------------------------|
      /// | artist         | If it is defined as an artist
      /// | album          | If it is defined as an album
      /// | song           | If it is defined as a song
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      getMediaType();
#else
      String getMediaType();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getArtist() }
      /// Returns the artist from music as string if present.
      ///
      /// @return [string] Music artist
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getArtist();
#else
      String getArtist();
#endif


#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getAlbum() }
      /// Returns the album from music tag as string if present.
      ///
      /// @return [string] Music album name
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getAlbum();
#else
      String getAlbum();
#endif


#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getAlbumArtist() }
      /// Returns the album artist from music tag as string if present.
      ///
      /// @return [string] Music album artist name
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getAlbumArtist();
#else
      String getAlbumArtist();
#endif


#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getGenre() }
      /// Returns the genre name from music tag as string if present.
      ///
      /// @return [string] Genre name
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Deprecated. Use **getGenres()** instead.
      ///
      getGenre();
#else
      String getGenre();
#endif


#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getGenres() }
      /// Returns the list of genres  from music tag if present.
      ///
      /// @return [list] List of genres
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getGenres();
#else
      std::vector<String> getGenres();
#endif


#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getDuration() }
      /// Returns the duration of music as integer from info tag.
      ///
      /// @return [integer] Duration
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getDuration();
#else
      int getDuration();
#endif


#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getYear() }
      /// Returns the year of music as integer from info tag.
      ///
      /// @return [integer] Year
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      ///
      getYear();
#else
      int getYear();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getRating() }
      /// Returns the scraped rating as integer.
      ///
      /// @return [integer] Rating
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getRating();
#else
      int getRating();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getUserRating() }
      /// Returns the user rating as integer (-1 if not existing)
      ///
      /// @return [integer] User rating
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getUserRating();
#else
      int getUserRating();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getTrack() }
      /// Returns the track number (if present) from music info tag as integer.
      ///
      /// @return [integer] Track number
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getTrack();
#else
      int getTrack();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getDisc() }
      /// Returns the disk number (if present) from music info tag as integer.
      ///
      /// @return [integer] Disc number
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getDisc();
#else
      /**
       * getDisc() -- returns an integer.\n
       */
      int getDisc();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getReleaseDate() }
      /// Returns the release date as string from music info tag (if present).
      ///
      /// @return [string] Release date
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getReleaseDate();
#else
      String getReleaseDate();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getListeners() }
      /// Returns the listeners as integer from music info tag.
      ///
      /// @return [integer] Listeners
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getListeners();
#else
      int getListeners();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getPlayCount() }
      /// Returns the number of carried out playbacks.
      ///
      /// @return [integer] Playback count
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getPlayCount();
#else
      int getPlayCount();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getLastPlayed() }
      /// Returns last played time as string from music info tag.
      ///
      /// @return [string] Last played date / time on tag
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Deprecated. Use **getLastPlayedAsW3C()** instead.
      ///
      getLastPlayed();
#else
      String getLastPlayed();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getLastPlayedAsW3C() }
      /// Returns last played time as string in W3C format (YYYY-MM-DDThh:mm:ssTZD).
      ///
      /// @return [string] Last played datetime (W3C)
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getLastPlayedAsW3C();
#else
      String getLastPlayedAsW3C();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getComment() }
      /// Returns comment as string from music info tag.
      ///
      /// @return [string] Comment on tag
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getComment();
#else
      String getComment();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getLyrics() }
      /// Returns a string from lyrics.
      ///
      /// @return [string] Lyrics on tag
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getLyrics();
#else
      String getLyrics();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getMusicBrainzTrackID() }
      /// Returns the MusicBrainz Recording ID from music info tag (if present).
      ///
      /// @return [string] MusicBrainz Recording ID
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 New function added.
      ///
      getMusicBrainzTrackID();
#else
      String getMusicBrainzTrackID();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getMusicBrainzArtistID() }
      /// Returns the MusicBrainz Artist IDs from music info tag (if present).
      ///
      /// @return [list] MusicBrainz Artist IDs
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 New function added.
      ///
      getMusicBrainzArtistID();
#else
      std::vector<String> getMusicBrainzArtistID();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getMusicBrainzAlbumID() }
      /// Returns the MusicBrainz Release ID from music info tag (if present).
      ///
      /// @return [string] MusicBrainz Release ID
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 New function added.
      ///
      getMusicBrainzAlbumID();
#else
      String getMusicBrainzAlbumID();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getMusicBrainzReleaseGroupID() }
      /// Returns the MusicBrainz Release Group ID from music info tag (if present).
      ///
      /// @return [string] MusicBrainz Release Group ID
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 New function added.
      ///
      getMusicBrainzReleaseGroupID();
#else
      String getMusicBrainzReleaseGroupID();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getMusicBrainzAlbumArtistID() }
      /// Returns the MusicBrainz Release Artist IDs from music info tag (if present).
      ///
      /// @return [list] MusicBrainz Release Artist IDs
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 New function added.
      ///
      getMusicBrainzAlbumArtistID();
#else
      std::vector<String> getMusicBrainzAlbumArtistID();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getSongVideoURL() }
      /// Returns the URL to a video of the song from the music tag as a string (if present).
      ///
      /// @return [string] URL to a video of the song.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v21 New function added.
      ///
      getSongVideoURL();
#else
      String getSongVideoURL();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setDbId(dbId, type) }
      /// Set the database identifier of the music item.
      ///
      /// @param dbId               integer - Database identifier.
      /// @param type               string - Media type of the item.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setDbId(...);
#else
      void setDbId(int dbId, const String& type);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setURL(url) }
      /// Set the URL of the music item.
      ///
      /// @param url                string - URL.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setURL(...);
#else
      void setURL(const String& url);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setMediaType(mediaType) }
      /// Set the media type of the music item.
      ///
      /// @param mediaType          string - Media type.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setMediaType(...);
#else
      void setMediaType(const String& mediaType);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setTrack(track) }
      /// Set the track number of the song.
      ///
      /// @param track              integer - Track number.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setTrack(...);
#else
      void setTrack(int track);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setDisc(disc) }
      /// Set the disc number of the song.
      ///
      /// @param disc               integer - Disc number.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setDisc(...);
#else
      void setDisc(int disc);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setDuration(duration) }
      /// Set the duration of the song.
      ///
      /// @param duration           integer - Duration in seconds.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setDuration(...);
#else
      void setDuration(int duration);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setYear(year) }
      /// Set the year of the music item.
      ///
      /// @param year               integer - Year.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setYear(...);
#else
      void setYear(int year);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setReleaseDate(releaseDate) }
      /// Set the release date of the music item.
      ///
      /// @param releaseDate        string - Release date in ISO8601 format (YYYY, YYYY-MM or YYYY-MM-DD).
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setReleaseDate(...);
#else
      void setReleaseDate(const String& releaseDate);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setListeners(listeners) }
      /// Set the number of listeners of the music item.
      ///
      /// @param listeners          integer - Number of listeners.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setListeners(...);
#else
      void setListeners(int listeners);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setPlayCount(playcount) }
      /// Set the playcount of the music item.
      ///
      /// @param playcount          integer - Playcount.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setPlayCount(...);
#else
      void setPlayCount(int playcount);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setGenres(genres) }
      /// Set the genres of the music item.
      ///
      /// @param genres             list - Genres.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setGenres(...);
#else
      void setGenres(const std::vector<String>& genres);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setAlbum(album) }
      /// Set the album of the music item.
      ///
      /// @param album              string - Album.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setAlbum(...);
#else
      void setAlbum(const String& album);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setArtist(artist) }
      /// Set the artist(s) of the music item.
      ///
      /// @param artist             string - Artist(s).
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setArtist(...);
#else
      void setArtist(const String& artist);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setAlbumArtist(albumArtist) }
      /// Set the album artist(s) of the music item.
      ///
      /// @param albumArtist        string - Album artist(s).
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setAlbumArtist(...);
#else
      void setAlbumArtist(const String& albumArtist);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setTitle(title) }
      /// Set the title of the music item.
      ///
      /// @param title              string - Title.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setTitle(...);
#else
      void setTitle(const String& title);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setRating(rating) }
      /// Set the rating of the music item.
      ///
      /// @param rating             float - Rating.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setRating(...);
#else
      void setRating(float rating);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setUserRating(userrating) }
      /// Set the user rating of the music item.
      ///
      /// @param userrating         integer - User rating.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setUserRating(...);
#else
      void setUserRating(int userrating);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setLyrics(lyrics) }
      /// Set the lyrics of the song.
      ///
      /// @param lyrics             string - Lyrics.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setLyrics(...);
#else
      void setLyrics(const String& lyrics);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setLastPlayed(lastPlayed) }
      /// Set the last played date of the music item.
      ///
      /// @param lastPlayed         string - Last played date (YYYY-MM-DD HH:MM:SS).
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setLastPlayed(...);
#else
      void setLastPlayed(const String& lastPlayed);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setMusicBrainzTrackID(musicBrainzTrackID) }
      /// Set the MusicBrainz track ID of the song.
      ///
      /// @param musicBrainzTrackID  string - MusicBrainz track ID.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setMusicBrainzTrackID(...);
#else
      void setMusicBrainzTrackID(const String& musicBrainzTrackID);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setMusicBrainzArtistID(musicBrainzArtistID) }
      /// Set the MusicBrainz artist IDs of the music item.
      ///
      /// @param musicBrainzArtistID  list - MusicBrainz artist IDs.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setMusicBrainzArtistID(...);
#else
      void setMusicBrainzArtistID(const std::vector<String>& musicBrainzArtistID);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setMusicBrainzAlbumID(musicBrainzAlbumID) }
      /// Set the MusicBrainz album ID of the music item.
      ///
      /// @param musicBrainzAlbumID  string - MusicBrainz album ID.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setMusicBrainzAlbumID(...);
#else
      void setMusicBrainzAlbumID(const String& musicBrainzAlbumID);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setMusicBrainzReleaseGroupID(musicBrainzReleaseGroupID) }
      /// Set the MusicBrainz release group ID of the music item.
      ///
      /// @param musicBrainzReleaseGroupID  string - MusicBrainz release group ID.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setMusicBrainzReleaseGroupID(...);
#else
      void setMusicBrainzReleaseGroupID(const String& musicBrainzReleaseGroupID);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setMusicBrainzAlbumArtistID(musicBrainzAlbumArtistID) }
      /// Set the MusicBrainz album artist IDs of the music item.
      ///
      /// @param musicBrainzAlbumArtistID  list - MusicBrainz album artist IDs.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setMusicBrainzAlbumArtistID(...);
#else
      void setMusicBrainzAlbumArtistID(const std::vector<String>& musicBrainzAlbumArtistID);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setComment(comment) }
      /// Set the comment of the music item.
      ///
      /// @param comment            string - Comment.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setComment(...);
#else
      void setComment(const String& comment);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ setSongVideoURL(songVideoURL) }
      /// Set the URL of the song to point to a video.
      ///
      /// @param songVideoURL            string - URL to a video of the song.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v21 New function added.
      ///
      setSongVideoURL(...);
#else
      void setSongVideoURL(const String& songVideoURL);
#endif

#ifndef SWIG
      static void setDbIdRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int dbId, const String& type);
      static void setURLRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& url);
      static void setMediaTypeRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& mediaType);
      static void setTrackRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int track);
      static void setDiscRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int disc);
      static void setDurationRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int duration);
      static void setYearRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int year);
      static void setReleaseDateRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& releaseDate);
      static void setListenersRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int listeners);
      static void setPlayCountRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int playcount);
      static void setGenresRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                               const std::vector<String>& genres);
      static void setAlbumRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& album);
      static void setArtistRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& artist);
      static void setAlbumArtistRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& albumArtist);
      static void setTitleRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& title);
      static void setRatingRaw(MUSIC_INFO::CMusicInfoTag* infoTag, float rating);
      static void setUserRatingRaw(MUSIC_INFO::CMusicInfoTag* infoTag, int userrating);
      static void setLyricsRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& lyrics);
      static void setLastPlayedRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& lastPlayed);
      static void setMusicBrainzTrackIDRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                           const String& musicBrainzTrackID);
      static void setMusicBrainzArtistIDRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                            const std::vector<String>& musicBrainzArtistID);
      static void setMusicBrainzAlbumIDRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                           const String& musicBrainzAlbumID);
      static void setMusicBrainzReleaseGroupIDRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                                  const String& musicBrainzReleaseGroupID);
      static void setMusicBrainzAlbumArtistIDRaw(
          MUSIC_INFO::CMusicInfoTag* infoTag, const std::vector<String>& musicBrainzAlbumArtistID);
      static void setCommentRaw(MUSIC_INFO::CMusicInfoTag* infoTag, const String& comment);
      static void setSongVideoURLRaw(MUSIC_INFO::CMusicInfoTag* infoTag,
                                     const String& songVideoURL);
#endif
    };
    //@}
  }
}
