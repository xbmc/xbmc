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

extern "C"
{
namespace V2
{
namespace KodiAPI
{

  //============================================================================
  ///
  /// \ingroup CPP_V2_KodiAPI_Player
  /// @{
  /// @brief Handle to use as independent pointer for player
  typedef void* PLAYERHANDLE;
  /// @}
  //----------------------------------------------------------------------------


  //============================================================================
  ///
  /// \ingroup CPP_V2_KodiAPI_Player_CPlayList
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

}; /* namespace KodiAPI */
}; /* namespace V2 */
}; /* extern "C" */
