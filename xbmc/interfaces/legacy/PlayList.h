/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"
#include "Exception.h"
#include "ListItem.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListTypes.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    XBMCCOMMONS_STANDARD_EXCEPTION(PlayListException);

#ifndef SWIG
    // the values of xbmc.PLAYLIST_MUSIC and xbmc.PLAYLIST_VIDEO
    constexpr int PLAYLIST_MUSIC_ID = 0;
    constexpr int PLAYLIST_VIDEO_ID = 1;
#endif

    //
    /// \defgroup python_PlayList PlayList
    /// \ingroup python_xbmc
    /// @{
    /// @brief **Kodi's Play List class.**
    ///
    /// \python_class{ xbmc.PlayList(playList) }
    ///
    /// To create and edit a playlist which can be handled by the player.
    ///
    /// @param playList              [integer] To define the stream type
    /// | Value | Integer String      | Description                            |
    /// |:-----:|:--------------------|:---------------------------------------|
    /// |   0   | xbmc.PLAYLIST_MUSIC | Playlist for music files or streams    |
    /// |   1   | xbmc.PLAYLIST_VIDEO | Playlist for video files or streams    |
    ///
    ///
    ///
    ///-------------------------------------------------------------------------
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
      KODI::PLAYLIST::Type m_type;
      const KODI::PLAYLIST::CPlayList* pPlayList;

    public:
      explicit PlayList(int playList);
      ~PlayList() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayList
      /// @brief \python_func{ getPlayListId() }
      /// Get the PlayList Identifier
      ///
      /// @return                    Id as an integer.
      ///
      getPlayListId();
#else
      int getPlayListId() const;
#endif

#ifndef SWIG
      KODI::PLAYLIST::Type GetType() const { return m_type; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayList
      /// @brief \python_func{ add(url[, listitem, index]) }
      /// Adds a new file to the playlist.
      ///
      /// @param url                 string or unicode - filename or url to add.
      /// @param listitem            [opt] listitem - used with setInfo() to set different infolabels.
      /// @param index               [opt] integer - position to add playlist item. (default=end)
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
      /// playlist.add(url=video, listitem=listitem, index=7)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      add(...);
#else
      void add(const String& url, XBMCAddon::xbmcgui::ListItem* listitem = NULL, int index = -1);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayList
      /// @brief \python_func{ load(filename) }
      /// Load a playlist.
      ///
      /// Clear current playlist and copy items from the file to this Playlist
      /// filename can be like .pls or .m3u ...
      ///
      /// @param filename            File with list to play inside
      /// @return                    False if unable to load playlist
      ///
      load(...);
#else
      bool load(const char* filename);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayList
      /// @brief \python_func{ remove(filename) }
      /// Remove an item with this filename from the playlist.
      ///
      /// @param filename            The file to remove from list.
      ///
      remove(...);
#else
      void remove(const char* filename);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayList
      /// @brief \python_func{ clear() }
      /// Clear all items in the playlist.
      ///
      clear();
#else
      void clear();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayList
      /// @brief \python_func{ size() }
      /// Returns the total number of PlayListItems in this playlist.
      ///
      /// @return                    Amount of playlist entries.
      ///
      size();
#else
      int size();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayList
      /// @brief \python_func{ shuffle() }
      /// Turn on shuffled play order. The items keep their positions.
      ///
      shuffle();
#else
      void shuffle();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayList
      /// @brief \python_func{ unshuffle() }
      /// Turn off shuffled play order. The items keep their positions.
      ///
      unshuffle();
#else
      void unshuffle();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayList
      /// @brief \python_func{ getposition() }
      /// Returns the position of the current entry in this playlist.
      ///
      /// @return                    Position of the current entry
      ///
      getposition();
#else
      int getposition();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayList
      /// @brief \python_func{ [] }
      /// Retrieve the item at the given position.
      ///
      /// @param i                   Pointer in list
      /// @return                    The selected item on list
      ///
      /// @note A negative index means
      /// from the end rather than from the start.
      ///
      [](...);
#else
      XBMCAddon::xbmcgui::ListItem* operator[](long i);
#endif
    };
    /// @}
  }
}

