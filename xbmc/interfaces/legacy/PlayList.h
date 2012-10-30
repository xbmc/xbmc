/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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

    class PlayList : public AddonClass
    {
      long refs;
      int iPlayList;
      PLAYLIST::CPlayList *pPlayList;

    public:
      PlayList(int playList) throw (PlayListException);
      virtual ~PlayList();

      inline int getPlayListId() const { return iPlayList; }

      /**
       * add(url[, listitem, index]) -- Adds a new file to the playlist.
       * 
       * url            : string or unicode - filename or url to add.
       * listitem       : [opt] listitem - used with setInfo() to set different infolabels.
       * index          : [opt] integer - position to add playlist item. (default=end)
       * 
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - playlist = xbmc.PlayList(xbmc.PLAYLIST_VIDEO)
       *   - video = 'F:\\\\movies\\\\Ironman.mov'
       *   - listitem = xbmcgui.ListItem('Ironman', thumbnailImage='F:\\\\movies\\\\Ironman.tbn')
       *   - listitem.setInfo('video', {'Title': 'Ironman', 'Genre': 'Science Fiction'})
       *   - playlist.add(url=video, listitem=listitem, index=7)\n
       */
      void add(const String& url, XBMCAddon::xbmcgui::ListItem* listitem = NULL, int index = -1);

      /**
       * load(filename) -- Load a playlist.
       * 
       * clear current playlist and copy items from the file to this Playlist
       * filename can be like .pls or .m3u ...
       * returns False if unable to load playlist
       */
      bool load(const char* filename) throw (PlayListException);

      /**
       * remove(filename) -- remove an item with this filename from the playlist.
       */
      void remove(const char* filename);

      /**
       * clear() -- clear all items in the playlist.
       */
      void clear();

      /**
       * size() -- returns the total number of PlayListItems in this playlist.
       */
      int size();

      /**
       * shuffle() -- shuffle the playlist.
       */
      void shuffle();

      /**
       * unshuffle() -- unshuffle the playlist.
       */
      void unshuffle();

      /**
       * getposition() -- returns the position of the current song in this playlist.
       */
      int getposition();

      /**
       * retrieve the item at the given position. A negative index means from the ending 
       * rather than from the start.
       */
      XBMCAddon::xbmcgui::ListItem* operator[](long i) throw (PlayListException);
    };
  }
}

