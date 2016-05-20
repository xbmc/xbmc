#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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

#include "../definitions-all.hpp"

API_NAMESPACE

namespace KodiAPI
{
extern "C"
{

  /*!
  \defgroup CPP_KodiAPI_Player 7. Player
  \ingroup cpp
  \brief <b><em>Classes to use for audio and video playbacks</em></b>
  */

  //============================================================================
  ///
  /// \ingroup CPP_KodiAPI_Player_CPlayer_Defs
  /// @{
  /// @brief Handle to use as independent pointer for player
  typedef void* PLAYERHANDLE;
  /// @}
  //----------------------------------------------------------------------------


  //============================================================================
  ///
  /// \ingroup CPP_KodiAPI_Player_CPlayList_Defs
  /// @{
  /// @brief Playlist types are defined here to identify the lists.
  ///
  typedef enum AddonPlayListType
  {
    /// To define playlist is for music
    PlayList_Music   = 0,
    /// To define playlist is for video
    PlayList_Video   = 1,
    /// To define playlist is for pictures
    PlayList_Picture = 2
  } AddonPlayListType;
  /// @}
  //----------------------------------------------------------------------------

} /* extern "C" */
} /* namespace KodiAPI */

END_NAMESPACE()
