#pragma once
/*
 *      Copyright (C) 2015 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../definitions.hpp"

API_NAMESPACE

namespace KodiAPI
{
namespace Player
{

  class CPlayer;

  ///
  /// \defgroup CPP_KodiAPI_Player_CInfoTagVideo Video Info Tag (class CInfoTagVideo)
  /// \ingroup CPP_KodiAPI_Player
  /// @{
  /// @brief <b>Used for present video information</b>
  ///
  /// Class contains after request the on Kodi present information of played
  /// video or as entry from a list item.
  ///
  /// It has the header \ref InfoTagVideo.hpp "#include <kodi/api3/player/InfoTagVideo.hpp>" be included
  /// to enjoy it.
  ///
  class CInfoTagVideo
  {
  public:
    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Class constructer with play as source.
    ///
    /// @param[in] player                Class from where the tag data becomes retrieved.
    ///
    CInfoTagVideo(CPlayer* player);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Class destructor.
    ///
    virtual ~CInfoTagVideo();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Get [film director](https://en.wikipedia.org/wiki/Film_director)
    /// who has made the film (if present).
    ///
    /// @return [string] Film director name.
    ///
    const std::string& GetDirector() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Get the writing credits if present from video info tag.
    ///
    /// @return [string] Writing credits
    ///
    const std::string& GetWritingCredits() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief To get the [Video Genre](https://en.wikipedia.org/wiki/Film_genre)
    /// if available.
    ///
    /// @return [string] Genre name
    ///
    const std::string& GetGenre() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Get country from where the video is (if rpresent)
    ///
    /// @return [string] Country from Video
    ///
    const std::string& GetCountry() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Get video tag line if available.
    ///
    /// @return [string] Video tag line
    ///
    const std::string& GetTagLine() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Get the outline plot of the video if present.
    ///
    /// @return [string] Outline plot
    ///
    const std::string& GetPlotOutline() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Get the plot of the video if present.
    ///
    /// @return [string] Plot
    ///
    const std::string& GetPlot() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Get the URL of the trailer from Video
    ///
    /// @return [string] Trailer URL (if present)
    ///
    const std::string& GetTrailer() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Get a picture URL of the video to show as screenshot.
    ///
    /// @return [string] Picture URL
    ///
    const std::string& GetPictureURL() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Get the video title.
    ///
    /// @return [string] Video title
    ///
    const std::string& GetTitle() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Get the video votes if available from video info tag.
    ///
    /// @return [string] Votes
    ///
    const std::string& GetVotes() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief To get the cast of the video when available.
    ///
    /// @return [string] Video casts
    ///
    const std::string& GetCast() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief To get the video file name.
    ///
    /// @return [string] File name
    ///
    const std::string& GetFile() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief To get the path where the video is stored.
    ///
    /// @return [string] Path
    ///
    const std::string& GetPath() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief To get the [IMDb](https://en.wikipedia.org/wiki/Internet_Movie_Database)
    /// number of the video (if present).
    ///
    /// @return [string] IMDb number
    ///
    const std::string& GetIMDBNumber() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// To get the [MPAA Rating](https://en.wikipedia.org/wiki/Motion_Picture_Association_of_America_film_rating_system)
    /// of the video (if present).
    ///
    /// @return [string] MPAA rating
    ///
    const std::string& GetMPAARating() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Get production year of video if present.
    ///
    /// @return [integer] Production Year
    ///
    int GetYear() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Get the video rating if present as float (double where supported).
    ///
    /// @return [float] The rating of the video
    ///
    double GetRating() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief To get the number of plays of the video.
    ///
    /// @return [integer] Play Count
    ///
    int GetPlayCount() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Get the last played date / time as string.
    ///
    /// @return [string] Last played date / time
    ///
    const std::string& GetLastPlayed() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief To get the original title of the video.
    ///
    /// @return [string] Original title
    ///
    const std::string& GetOriginalTitle() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief To get [premiered](https://en.wikipedia.org/wiki/Premiere) date
    /// of the video, if available.
    ///
    /// @return [string] Get premiered date string (if present)
    ///
    const std::string& GetPremiered() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Returns first aired date as string from info tag.
    ///
    /// @return [string] First aired date
    ///
    const std::string& GetFirstAired() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CInfoTagVideo
    /// @brief Returns the duration of video.
    ///
    /// @return Duration of video in seconds
    ///
    unsigned int GetDuration() const;
    //--------------------------------------------------------------------------

  #ifndef DOXYGEN_SHOULD_SKIP_THIS
    IMPL_ADDON_INFO_TAG_VIDEO;
  #endif
  };
  /// @}
} /* namespace Player */
} /* namespace KodiAPI */

END_NAMESPACE()
