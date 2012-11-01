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

#include "PlayList.h"
#include "PlayListPlayer.h"
#include "settings/Settings.h"
#include "Application.h"
#include "playlists/PlayListFactory.h"
#include "utils/URIUtils.h"

using namespace PLAYLIST;

namespace XBMCAddon
{
  namespace xbmc
  {
    // TODO: need a means to check for a valid construction
    //  either by throwing an exception or by an "isValid" check
    PlayList::PlayList(int playList) throw (PlayListException) : 
      AddonClass("PlayList"),
      refs(1), iPlayList(playList), pPlayList(NULL)
    {
      // we do not create our own playlist, just using the ones from playlistplayer
      if (iPlayList != PLAYLIST_MUSIC &&
          iPlayList != PLAYLIST_VIDEO)
        throw PlayListException("PlayList does not exist");

      pPlayList = &g_playlistPlayer.GetPlaylist(playList);
      iPlayList = playList;
    }

    PlayList::~PlayList()  { }

    void PlayList::add(const String& url, XBMCAddon::xbmcgui::ListItem* listitem, int index)
    {
      CFileItemList items;

      if (listitem != NULL)
      {
        // an optional listitem was passed
        // set m_strPath to the passed url
        listitem->item->SetPath(url);

        items.Add(listitem->item);
      }
      else
      {
        CFileItemPtr item(new CFileItem(url, false));
        item->SetLabel(url);

        items.Add(item);
      }

      pPlayList->Insert(items, index);
    }

    bool PlayList::load(const char* cFileName) throw (PlayListException)
    {
      CFileItem item(cFileName);
      item.SetPath(cFileName);

      if (item.IsPlayList())
      {
        // load playlist and copy al items to existing playlist

        // load a playlist like .m3u, .pls
        // first get correct factory to load playlist
        std::auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(item));
        if ( NULL != pPlayList.get())
        {
          // load it
          if (!pPlayList->Load(item.GetPath()))
            //hmmm unable to load playlist?
            return false;

          // clear current playlist
          g_playlistPlayer.ClearPlaylist(this->iPlayList);

          // add each item of the playlist to the playlistplayer
          for (int i=0; i < (int)pPlayList->size(); ++i)
          {
            CFileItemPtr playListItem =(*pPlayList)[i];
            if (playListItem->GetLabel().IsEmpty())
              playListItem->SetLabel(URIUtils::GetFileName(playListItem->GetPath()));

            this->pPlayList->Add(playListItem);
          }
        }
      }
      else
        // filename is not a valid playlist
        throw PlayListException("Not a valid playlist");

      return true;
    }

    void PlayList::remove(const char* filename)
    {
      pPlayList->Remove(filename);
    }

    void PlayList::clear()
    {
      pPlayList->Clear();
    }

    int PlayList::size()
    {
      return pPlayList->size();
    }

    void PlayList::shuffle()
    {
      pPlayList->Shuffle();
    }

    void PlayList::unshuffle()
    {
      pPlayList->UnShuffle();
    }

    int PlayList::getposition()
    {
      return g_playlistPlayer.GetCurrentSong();
    }

    XBMCAddon::xbmcgui::ListItem* PlayList::operator [](long i) throw (PlayListException)
    {
      long pos = -1;
      int iPlayListSize = size();

      pos = i;
      if (pos < 0) pos += iPlayListSize;

      if (pos < 0 || pos >= iPlayListSize)
        throw PlayListException("array out of bound");

      CFileItemPtr ptr((*pPlayList)[pos]);

      return new XBMCAddon::xbmcgui::ListItem(ptr);
    }
  }
}

