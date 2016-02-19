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

#include "video/VideoInfoTag.h"
#include "AddonClass.h"

#pragma once

namespace XBMCAddon
{
  namespace xbmc
  {

    /**
     * \defgroup python_InfoTagVideo InfoTagVideo
     * \ingroup python_xbmc
     * @{
     * @brief <b>Kodi's video info tag class.</b>
     *
     * To get video info tag data of currently played source.
     *
     * @note Info tag load is only be possible from present player class.
     *
     *
     *--------------------------------------------------------------------------
     *
     * <b>Example:</b>
     * @code{.py}
     * ...
     * tag = xbmc.Player().getVideoInfoTag()
     *
     * title = tag.getTitle()
     * file  = tag.getFile()
     * ...
     * @endcode
     */
    class InfoTagVideo : public AddonClass
    {
    private:
      CVideoInfoTag* infoTag;

    public:
#ifndef SWIG
      InfoTagVideo(const CVideoInfoTag& tag);
#endif
      InfoTagVideo();
      virtual ~InfoTagVideo();

      /**
       * \ingroup python_InfoTagVideo
       * @brief Get [film director](https://en.wikipedia.org/wiki/Film_director)
       * who has made the film (if present).
       *
       * @return [string] Film director name.
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getDirector();

      /**
       * \ingroup python_InfoTagVideo
       * @brief Get the writing credits if present from video info tag.
       *
       * @return [string] Writing credits
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getWritingCredits();

      /**
       * \ingroup python_InfoTagVideo
       * @brief To get the [Video Genre](https://en.wikipedia.org/wiki/Film_genre)
       * if available.
       *
       * @return [string] Genre name
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getGenre();

      /**
       * \ingroup python_InfoTagVideo
       * @brief Get video tag line if available.
       *
       * @return [string] Video tag line
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getTagLine();

      /**
       * \ingroup python_InfoTagVideo
       * @brief Get the outline plot of the video if present.
       *
       * @return [string] Outline plot
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getPlotOutline();

      /**
       * \ingroup python_InfoTagVideo
       * @brief Get the plot of the video if present.
       *
       * @return [string] Plot
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getPlot();

      /**
       * \ingroup python_InfoTagVideo
       * @brief Get a picture URL of the video to show as screenshot.
       *
       * @return [string] Picture URL
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getPictureURL();

      /**
       * \ingroup python_InfoTagVideo
       * @brief Get the video title.
       *
       * @return [string] Video title
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getTitle();

      /**
       * \ingroup python_InfoTagVideo
       * @brief Get the video votes if available from video info tag.
       *
       * @return [string] Votes
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getVotes();

      /**
       * \ingroup python_InfoTagVideo
       * @brief To get the cast of the video when available.
       *
       * @return [string] Video casts
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getCast();

      /**
       * \ingroup python_InfoTagVideo
       * @brief To get the video file name.
       *
       * @return [string] File name
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getFile();

      /**
       * \ingroup python_InfoTagVideo
       * @brief To get the path where the video is stored.
       *
       * @return [string] Path
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getPath();

      /**
       * \ingroup python_InfoTagVideo
       * @brief To get the [IMDb](https://en.wikipedia.org/wiki/Internet_Movie_Database)
       * number of the video (if present).
       *
       * @return [string] IMDb number
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getIMDBNumber();

      /**
       * \ingroup python_InfoTagVideo
       * @brief Get production year of video if present.
       *
       * @return [integer] Production Year
       *
       *
       *------------------------------------------------------------------------
       *
       */
      int getYear();

      /**
       * \ingroup python_InfoTagVideo
       * @brief Get the video rating if present as float (double where supported).
       *
       * @return [float] The rating of the video
       *
       *
       *------------------------------------------------------------------------
       *
       */
      double getRating();

      /**
       * \ingroup python_InfoTagVideo
       * @brief To get the number of plays of the video.
       *
       * @return [integer] Play Count
       *
       *
       *------------------------------------------------------------------------
       *
       */
      int getPlayCount();

      /**
       * \ingroup python_InfoTagVideo
       * @brief Get the last played date / time as string.
       *
       * @return [string] Last played date / time
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getLastPlayed();

      /**
       * \ingroup python_InfoTagVideo
       * @brief To get the original title of the video.
       *
       * @return [string] Original title
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getOriginalTitle();

      /**
       * \ingroup python_InfoTagVideo
       * @brief To get [premiered](https://en.wikipedia.org/wiki/Premiere) date
       * of the video, if available.
       *
       * @return [string]
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getPremiered();

      /**
       * \ingroup python_InfoTagVideo
       * @brief Returns first aired date as string from info tag.
       *
       * @return [string] First aired date
       *
       *
       *------------------------------------------------------------------------
       *
       */
      String getFirstAired();
    };
    //@}
  }
}
