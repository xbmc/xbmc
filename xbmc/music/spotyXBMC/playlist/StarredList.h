/*
 spotyxbmc2 - A project to integrate Spotify into XBMC
 Copyright (C) 2011  David Erenger

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 For contact with the author:
 david.erenger@gmail.com
 */

#ifndef STARREDLISTS_H_
#define STARREDLISTS_H_

#include "SxPlaylist.h"
#include "../album/AlbumContainer.h"
#include "StarredBackgroundLoader.h"

namespace addon_music_spotify {

  class SxAlbum;
  class SxArtist;

  class StarredList: public SxPlaylist, private AlbumContainer {
  public:
    StarredList(sp_playlist* spPlaylist);
    virtual ~StarredList();

    void populateAlbumsAndArtists();
    bool isLoaded();
    void reLoad();

    int getNumberOfAlbums() {
      return m_albums.size();
    }
    SxAlbum* getAlbum(int index);

    bool getAlbumItems(CFileItemList& items);

    int getNumberOfArtists() {
      return m_artists.size();
    }
    SxArtist* getArtist(int index);

    friend class StarredBackgroundLoader;

  private:
    vector<SxArtist*> m_artists;

    bool m_isBackgroundLoading;
    bool m_reload;
    StarredBackgroundLoader* m_backgroundLoader;
  };

} /* namespace addon_music_spotify */
#endif /* STARREDLISTS_H_ */
