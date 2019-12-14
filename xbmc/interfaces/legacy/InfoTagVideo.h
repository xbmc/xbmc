/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"
#include "video/VideoInfoTag.h"

namespace XBMCAddon
{
  namespace xbmc
  {

    ///
    /// \defgroup python_InfoTagVideo InfoTagVideo
    /// \ingroup python_xbmc
    /// @{
    /// @brief **Kodi's video info tag class.**
    ///
    /// \python_class{ xbmc.InfoTagVideo() }
    ///
    /// Access and / or modify the video metadata of a ListItem.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// tag = xbmc.Player().getVideoInfoTag()
    ///
    /// title = tag.getTitle()
    /// file  = tag.getFile()
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class InfoTagVideo : public AddonClass
    {
    private:
      CVideoInfoTag* infoTag;
      bool owned;

    public:
#ifndef SWIG
      explicit InfoTagVideo(const CVideoInfoTag* tag);
      explicit InfoTagVideo(CVideoInfoTag* tag);
#endif
      InfoTagVideo();
      ~InfoTagVideo() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getDbId() }
      /// Get identification number of tag in database
      ///
      /// @return [integer] database id
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      getDbId();
#else
      int getDbId();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getDirector() }
      /// Get [film director](https://en.wikipedia.org/wiki/Film_director)
      /// who has made the film (if present).
      ///
      /// @return [string] Film director name.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getDirector();
#else
      String getDirector();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getWritingCredits() }
      /// Get the writing credits if present from video info tag.
      ///
      /// @return [string] Writing credits
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getWritingCredits();
#else
      String getWritingCredits();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getGenre() }
      /// To get the [Video Genre](https://en.wikipedia.org/wiki/Film_genre)
      /// if available.
      ///
      /// @return [string] Genre name
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getGenre();
#else
      String getGenre();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getTagLine() }
      /// Get video tag line if available.
      ///
      /// @return [string] Video tag line
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getTagLine();
#else
      String getTagLine();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPlotOutline() }
      /// Get the outline plot of the video if present.
      ///
      /// @return [string] Outline plot
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getPlotOutline();
#else
      String getPlotOutline();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPlot() }
      /// Get the plot of the video if present.
      ///
      /// @return [string] Plot
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getPlot();
#else
      String getPlot();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPictureURL() }
      /// Get a picture URL of the video to show as screenshot.
      ///
      /// @return [string] Picture URL
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getPictureURL();
#else
      String getPictureURL();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getTitle() }
      /// Get the video title.
      ///
      /// @return [string] Video title
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
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getTVShowTitle() }
      /// Get the video TV show title.
      ///
      /// @return [string] TV show title
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      getTVShowTitle();
#else
      String getTVShowTitle();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getMediaType() }
      /// Get the media type of the video.
      ///
      /// @return [string] media type
      ///
      /// Available strings about media type for video:
      /// | String         | Description                                       |
      /// |---------------:|:--------------------------------------------------|
      /// | video          | For normal video
      /// | set            | For a selection of video
      /// | musicvideo     | To define it as music video
      /// | movie          | To define it as normal movie
      /// | tvshow         | If this is it defined as tvshow
      /// | season         | The type is used as a series season
      /// | episode        | The type is used as a series episode
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      getMediaType();
#else
      String getMediaType();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getVotes() }
      /// Get the video votes if available from video info tag.
      ///
      /// @return [string] Votes
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Deprecated. Use **getVotesAsInt()** instead.
      ///
      getVotes();
#else
      String getVotes();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getVotesAsInt([type]) }
      /// Get the votes of the rating (if available) as an integer.
      ///
      /// @param type           [opt] string - the type of the rating.
      /// - Some rating type values (any string possible):
      ///  | Label         | Type                                             |
      ///  |---------------|--------------------------------------------------|
      ///  | imdb          | string - type name
      ///  | tvdb          | string - type name
      ///  | tmdb          | string - type name
      ///  | anidb         | string - type name
      ///
      /// @return [integer] Votes
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getVotesAsInt(type);
#else
      int getVotesAsInt(const String& type = "");
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getCast() }
      /// To get the cast of the video when available.
      ///
      /// @return [string] Video casts
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getCast();
#else
      String getCast();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getFile() }
      /// To get the video file name.
      ///
      /// @return [string] File name
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getFile();
#else
      String getFile();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPath() }
      /// To get the path where the video is stored.
      ///
      /// @return [string] Path
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getPath();
#else
      String getPath();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getFilenameAndPath() }
      /// To get the full path with filename where the video is stored.
      ///
      /// @return [string] File name and Path
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 New function added.
      ///
      getFilenameAndPath();
#else
      String getFilenameAndPath();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getIMDBNumber() }
      /// To get the [IMDb](https://en.wikipedia.org/wiki/Internet_Movie_Database)
      /// number of the video (if present).
      ///
      /// @return [string] IMDb number
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getIMDBNumber();
#else
      String getIMDBNumber();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getSeason() }
      /// To get season number of a series
      ///
      /// @return [integer] season number
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      getSeason();
#else
      int getSeason();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getEpisode() }
      /// To get episode number of a series
      ///
      /// @return [integer] episode number
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      getEpisode();
#else
      int getEpisode();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getYear() }
      /// Get production year of video if present.
      ///
      /// @return [integer] Production Year
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getYear();
#else
      int getYear();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getRating([type]) }
      /// Get the video rating if present as float (double where supported).
      ///
      /// @param type           [opt] string - the type of the rating.
      /// - Some rating type values (any string possible):
      ///  | Label         | Type                                             |
      ///  |---------------|--------------------------------------------------|
      ///  | imdb          | string - type name
      ///  | tvdb          | string - type name
      ///  | tmdb          | string - type name
      ///  | anidb         | string - type name
      ///
      /// @return [float] The rating of the video
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Optional `type` parameter added.
      ///
      getRating(type);
#else
      double getRating(const String& type = "");
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getUserRating() }
      /// Get the user rating if present as integer.
      ///
      /// @return [integer] The user rating of the video
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
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPlayCount() }
      /// To get the number of plays of the video.
      ///
      /// @return [integer] Play Count
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
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getLastPlayed() }
      /// Get the last played date / time as string.
      ///
      /// @return [string] Last played date / time
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
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getLastPlayedAsW3C() }
      /// Get last played datetime as string in W3C format (YYYY-MM-DDThh:mm:ssTZD).
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
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getOriginalTitle() }
      /// To get the original title of the video.
      ///
      /// @return [string] Original title
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getOriginalTitle();
#else
      String getOriginalTitle();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPremiered() }
      /// To get [premiered](https://en.wikipedia.org/wiki/Premiere) date
      /// of the video, if available.
      ///
      /// @return [string]
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Deprecated. Use **getPremieredAsW3C()** instead.
      ///
      getPremiered();
#else
      String getPremiered();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPremieredAsW3C() }
      /// Get [premiered](https://en.wikipedia.org/wiki/Premiere) date as string in W3C format (YYYY-MM-DD).
      ///
      /// @return [string] Premiered date (W3C)
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getPremieredAsW3C();
#else
      String getPremieredAsW3C();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getFirstAired() }
      /// Returns first aired date as string from info tag.
      ///
      /// @return [string] First aired date
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Deprecated. Use **getFirstAiredAsW3C()** instead.
      ///
      getFirstAired();
#else
      String getFirstAired();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getFirstAiredAsW3C() }
      /// Get first aired date as string in W3C format (YYYY-MM-DD).
      ///
      /// @return [string] First aired date (W3C)
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getFirstAiredAsW3C();
#else
      String getFirstAiredAsW3C();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getTrailer() }
      /// To get the path where the trailer is stored.
      ///
      /// @return [string] Trailer path
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      getTrailer();
#else
      String getTrailer();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getArtist() }
      /// To get the artist name (for musicvideos)
      ///
      /// @return [std::vector<std::string>] Artist name
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      getArtist();
#else
      std::vector<std::string> getArtist();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getAlbum() }
      /// To get the album name (for musicvideos)
      ///
      /// @return [string] Album name
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      getAlbum();
#else
      String getAlbum();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getTrack() }
      /// To get the track number (for musicvideos)
      ///
      /// @return [int] Track number
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      getTrack();
#else
      int getTrack();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getDuration() }
      /// To get the duration
      ///
      /// @return [unsigned int] Duration
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      getDuration();
#else
      unsigned int getDuration();
#endif

      // TODO(Montellese)
      double getResumeTime();
      double getResumeTimeTotal();

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getUniqueID(key) }
      ///-----------------------------------------------------------------------
      /// Get the unique ID of the given key.
      /// A unique ID is an identifier used by a (online) video database used to
      /// identify a video in its database.
      ///
      /// @param key            string - uniqueID name.
      /// - Some default uniqueID values (any string possible):
      ///  | Label         | Type                                             |
      ///  |---------------|--------------------------------------------------|
      ///  | imdb          | string - uniqueid name
      ///  | tvdb          | string - uniqueid name
      ///  | tmdb          | string - uniqueid name
      ///  | anidb         | string - uniqueid name
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getUniqueID(key);
#else
      String getUniqueID(const char* key);
#endif
    };
  }
}
