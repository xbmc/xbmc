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
  namespace GUI
  {
    class CListItem;
  }

namespace Player
{

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_Player_CPlayList Play List (class CPlayList)
  /// \ingroup CPP_KodiAPI_Player
  /// @{
  /// @brief <b>Playpack control list</b>
  ///
  /// Class to create a list of different sources to a continuous playback of
  /// this. This also supports a random mode (shuffle) to select the next source.
  ///
  /// It has the header \ref PlayList.hpp "#include <kodi/api2/player/PlayList.hpp>" be included
  /// to enjoy it.
  ///

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_Player_CPlayList_Defs Definitions, structures and enumerators
  /// \ingroup CPP_KodiAPI_Player_CPlayList
  /// @brief <b>Library definition values</b>
  ///

  class CPlayList
  {
  public:
    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CPlayList
    /// @brief Class constructor with set of needed playlist type.
    ///
    /// @param[in] playList     Type of playlist to use
    ///
    CPlayList(AddonPlayListType playList);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CPlayList
    /// @brief Class deconstructor.
    ///
    virtual ~CPlayList();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CPlayList
    /// @brief To get the type of playlist
    ///
    /// @return                        Type of playlist
    ///
    AddonPlayListType GetPlayListType() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CPlayList
    /// @brief Load a playlist.
    ///
    /// Clear current playlist and copy items from the file to this Playlist
    /// filename can be like .pls or .m3u ...
    ///
    /// @param[in] filename            File with list to play inside
    /// @return                        False if unable to load playlist
    ///
    bool LoadPlaylist(const std::string& filename);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CPlayList
    /// @brief Adds a new file to the playlist from normal URL.
    ///
    /// @param[in] url                  string or unicode - filename or url to add.
    /// @param[in] index                [opt] integer - position to add playlist item. (default=end)
    ///
    void AddItem(const std::string& url, int index = -1);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CPlayList
    /// @brief Adds a new file descripted with list item to the playlist.
    ///
    /// @param[in] listitem             listitem - file to add
    /// @param[in] index                [opt] integer - position to add playlist item. (default=end)
    ///
    void AddItem(const API_NAMESPACE_NAME::KodiAPI::GUI::CListItem* listitem, int index = -1);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CPlayList
    /// @brief Remove an item with this filename from the playlist.
    ///
    /// @param[in] url                  The file to remove from list.
    ///
    void RemoveItem(const std::string& url);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CPlayList
    /// @brief Clear all items in the playlist
    ///
    void ClearList();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CPlayList
    /// @brief Returns the total number of PlayListItems in this playlist.
    ///
    /// @return                        Amount of playlist entries.
    ///
    int GetListSize();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CPlayList
    /// @brief Returns the position of the current song in this playlist.
    ///
    /// @return                        Position of the current song
    ///
    int GetListPosition();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CPlayList
    /// @brief Shuffle the playlist.
    ///
    /// @param[in] shuffle              If true becomes shuffle enabled, otherwise disabled
    ///
    void Shuffle(bool shuffle);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_Player_CPlayList
    /// @brief Retrieve the item at the given position.
    ///
    /// A negative index means from the ending rather than from the start.
    ///
    /// @param[in] i                   Pointer in list
    /// @return                        The selected item on list
    ///
    API_NAMESPACE_NAME::KodiAPI::GUI::CListItem* operator[](long i);
    //--------------------------------------------------------------------------

  #ifndef DOXYGEN_SHOULD_SKIP_THIS
    IMPL_ADDON_PLAYLIST;
  #endif
  };
  /// @}
} /* namespace Player */
} /* namespace KodiAPI */

END_NAMESPACE()
