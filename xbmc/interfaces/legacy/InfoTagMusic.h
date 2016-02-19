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

#include "music/tags/MusicInfoTag.h"
#include "AddonClass.h"

#pragma once

namespace XBMCAddon
{
  namespace xbmc
  {
    //
    /// \defgroup python_InfoTagMusic InfoTagMusic
    /// \ingroup python_xbmc
    /// @{
    /// @brief <b>Kodi's music info tag class.</b>
    ///
    /// To get music info tag data of currently played source.
    ///
    /// @note Info tag load is only be possible from present player class.
    ///
    ///
    ///--------------------------------------------------------------------------
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

    public:
#ifndef SWIG
      InfoTagMusic(const MUSIC_INFO::CMusicInfoTag& tag);
#endif
      InfoTagMusic();
      virtual ~InfoTagMusic();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns url of source as string from music info tag.
      ///
      /// @return [string] Url of source
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      String getURL();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns the title from music as string on info tag.
      ///
      /// @return [string] Music title
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      String getTitle();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns the artist from music as string if present.
      ///
      /// @return [string] Music artist
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      String getArtist();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns the album from music tag as string if present.
      ///
      /// @return [string] Music album name
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      String getAlbum();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns the album artist from music tag as string if present.
      ///
      /// @return [string] Music album artist name
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      String getAlbumArtist();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns the genre name from music tag as string if present.
      ///
      /// @return [string] Genre name
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      String getGenre();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns the duration of music as integer from info tag.
      ///
      /// @return [integer] Duration
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      int getDuration();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns the track number (if present) from music info tag as integer.
      ///
      /// @return [integer] Track number
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      int getTrack();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns the disk number (if present) from music info tag as
      /// integer.
      ///
      /// @return [integer] Disc number
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      int getDisc();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns the release date as string from music info tag (if present).
      ///
      /// @return [string] Release date
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      String getReleaseDate();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns the listeners as integer from music info tag.
      ///
      /// @return [integer] Listeners
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      int getListeners();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns the number of carried out playbacks.
      ///
      /// @return [integer] Playback count
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      int getPlayCount();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns last played time as string from music info tag.
      ///
      /// @return [string] Last played date / time on tag
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      String getLastPlayed();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns comment as string from music info tag.
      ///
      /// @return [string] Comment on tag
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      String getComment();

      ///
      /// \ingroup python_InfoTagMusic
      /// @brief Returns a string from lyrics.
      ///
      /// @return [string] Lyrics on tag
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      ///
      String getLyrics();
    };
    //@}
  }
}
