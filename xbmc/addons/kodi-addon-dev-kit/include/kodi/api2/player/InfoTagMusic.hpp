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

namespace V2
{
namespace KodiAPI
{

  namespace Player
  {

  class CPlayer;

  //============================================================================
  ///
  /// \defgroup CPP_V2_KodiAPI_Player_CInfoTagMusic
  /// \ingroup CPP_V2_KodiAPI_Player
  /// @{
  /// @brief <b>Used for present music information</b>
  ///
  /// Class contains after request the on Kodi present information of played
  /// music or as entry from a list item.
  ///
  /// It has the header \ref InfoTagMusic.h "#include <kodi/api2/player/InfoTagMusic.h>" be included
  /// to enjoy it.
  ///
  class CInfoTagMusic
  {
  public:
    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Class constructer with play as source.
    ///
    /// @param[in] player                Class from where the tag data becomes retrieved.
    ///
    CInfoTagMusic(CPlayer* player);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Class destructor.
    ///
    virtual ~CInfoTagMusic();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns url of source as string from music info tag.
    ///
    /// @return [string] Url of source
    ///
    const std::string& GetURL() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns the title from music as string on info tag.
    ///
    /// @return [string] Music title
    ///
    const std::string& GetTitle() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns the artist from music as string if present.
    ///
    /// @return [string] Music artist
    ///
    const std::string& GetArtist() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns the album from music tag as string if present.
    ///
    /// @return [string] Music album name
    ///
    const std::string& GetAlbum() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns the album artist from music tag as string if present.
    ///
    /// @return [string] Music album artist name
    ///
    const std::string& GetAlbumArtist() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns the genre name from music tag as string if present.
    ///
    /// @return [string] Genre name
    ///
    const std::string& GetGenre() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns the duration of music as integer from info tag.
    ///
    /// @return [integer] Duration
    ///
    int GetDuration() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns the track number (if present) from music info tag as integer.
    ///
    /// @return [integer] Track number
    ///
    int GetTrack() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns the disk number (if present) from music info tag as
    /// integer.
    ///
    /// @return [integer] Disc number
    ///
    int GetDisc() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns the release date as string from music info tag (if present).
    ///
    /// @return [string] Release date
    ///
    const std::string& GetReleaseDate() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns the listeners as integer from music info tag.
    ///
    /// @return [integer] Listeners
    ///
    int GetListeners() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns the number of carried out playbacks.
    ///
    /// @return [integer] Playback count
    ///
    int GetPlayCount() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns last played time as string from music info tag.
    ///
    /// @return [string] Last played date / time on tag
    ///
    const std::string& GetLastPlayed() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns comment as string from music info tag.
    ///
    /// @return [string] Comment on tag
    ///
    const std::string& GetComment() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CInfoTagMusic
    /// @brief Returns a string from lyrics.
    ///
    /// @return [string] Lyrics on tag
    ///
    const std::string& GetLyrics() const;
    //--------------------------------------------------------------------------

    IMPL_ADDON_INFO_TAG_MUSIC;
  };
  /// @}
  }; /* namespace Player */

}; /* namespace KodiAPI */
}; /* namespace V2 */
