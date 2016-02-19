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

#pragma once

#include "playlists/PlayList.h"
#include "AddonClass.h"
#include "Exception.h"

#include "ListItem.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    XBMCCOMMONS_STANDARD_EXCEPTION(PlayListException);

    //
    /// \defgroup python_PlayList PlayList
    /// \ingroup python_xbmc
    /// @{
    /// @brief <b>Kodi's Play List class.</b>
    ///
    /// <b><c>xbmc.PlayList(playList)</c></b>
    ///
    /// To create and edit a playlist which can be handled by the player.
    ///
    /// @param[in] playList              [integer] To define the stream type
    /// | Value | Integer String      | Description                             |
    /// |:-----:|:--------------------|:----------------------------------------|
    /// |   0   | xbmc.PLAYLIST_MUSIC | Playlist for music files or streams     |
    /// |   1   | xbmc.PLAYLIST_VIDEO | Playlist for video files or streams     |
    ///
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// play=xbmc.PlayList(xbmc.PLAYLIST_VIDEO)
    /// ...
    /// ~~~~~~~~~~~~~
    //
    class PlayList : public AddonClass
    {
      long refs;
      int iPlayList;
      PLAYLIST::CPlayList *pPlayList;

    public:
      PlayList(int playList);
      virtual ~PlayList();

      ///
      /// \ingroup python_PlayList
      /// @brief Get the PlayList Identifier
      ///
      /// @return                        Id as an integer.
      ///
      inline int getPlayListId() const { return iPlayList; }

      ///
      /// \ingroup python_PlayList
      /// @brief Adds a new file to the playlist.
      ///
      /// @param[in] url                 string or unicode - filename or url to add.
      /// @param[in] listitem            [opt] listitem - used with setInfo() to set different infolabels.
      /// @param[in] index               [opt] integer - position to add playlist item. (default=end)
      ///
      /// @note You can use the above as keywords for arguments and skip certain optional arguments.\n
      ///       Once you use a keyword, all following arguments require the keyword.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// playlist = xbmc.PlayList(xbmc.PLAYLIST_VIDEO)
      /// video = 'F:\\movies\\Ironman.mov'
      /// listitem = xbmcgui.ListItem('Ironman', thumbnailImage='F:\\movies\\Ironman.tbn')
      /// listitem.setInfo('video', {'Title': 'Ironman', 'Genre': 'Science Fiction'})
      /// playlist.add(url=video, listitem=listitem, index=7)n
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      void add(const String& url,
#ifndef DOXYGEN_SHOULD_SKIP_THIS
               XBMCAddon::xbmcgui::
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
               ListItem* listitem = NULL,
               int index = -1);

      ///
      /// \ingroup python_PlayList
      /// @brief Load a playlist.
      ///
      /// Clear current playlist and copy items from the file to this Playlist
      /// filename can be like .pls or .m3u ...
      ///
      /// @param[in] filename            File with list to play inside
      /// @return                        False if unable to load playlist
      ///
      bool load(const char* filename);

      ///
      /// \ingroup python_PlayList
      /// @brief Remove an item with this filename from the playlist.
      ///
      /// @param[in] filename            The file to remove from list.
      ///
      void remove(const char* filename);

      ///
      /// \ingroup python_PlayList
      /// @brief Clear all items in the playlist.
      ///
      void clear();

      ///
      /// \ingroup python_PlayList
      /// @brief Returns the total number of PlayListItems in this playlist.
      ///
      /// @return                        Amount of playlist entries.
      ///
      int size();

      ///
      /// \ingroup python_PlayList
      /// @brief Shuffle the playlist.
      ///
      void shuffle();

      ///
      /// \ingroup python_PlayList
      /// @brief Unshuffle the playlist.
      ///
      void unshuffle();

      ///
      /// \ingroup python_PlayList
      /// @brief Returns the position of the current song in this playlist.
      ///
      /// @return                        Position of the current song
      ///
      int getposition();

      ///
      /// \ingroup python_PlayList
      /// @brief Retrieve the item at the given position. A negative index means
      /// from the ending rather than from the start.
      ///
      /// @param[in] i                   Pointer in list
      /// @return                        The selected item on list
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      XBMCAddon::xbmcgui::
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      ListItem* operator[](long i);
    };
    /// @}
  }
}

