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

#include "AlbumStore.h"
#include "../Logger.h"
#include "SxAlbum.h"

namespace addon_music_spotify {

  AlbumStore::AlbumStore() {
  }

  void AlbumStore::deInit() {
    delete m_instance;
  }

  AlbumStore::~AlbumStore() {
    for (albumMap::iterator it = m_albums.begin(); it != m_albums.end(); ++it) {
      delete it->second;
    }
  }

  AlbumStore* AlbumStore::m_instance = 0;
  AlbumStore *AlbumStore::getInstance() {
    return m_instance ? m_instance : (m_instance = new AlbumStore);
  }

  SxAlbum* AlbumStore::getAlbum(sp_album *spAlbum, bool loadTracksAndDetails) {
    //Logger::printOut("loading spAlbum");
    sp_album_add_ref(spAlbum);
    while (!sp_album_is_loaded(spAlbum))
      ;

    albumMap::iterator it = m_albums.find(spAlbum);
    SxAlbum *album;

    if (it == m_albums.end()) {
      //we need to create it
      album = new SxAlbum(spAlbum, loadTracksAndDetails);
      m_albums.insert(albumMap::value_type(spAlbum, album));
    } else {
      //Logger::printOut("loading album from store");
      album = it->second;

      if (loadTracksAndDetails) album->doLoadTracksAndDetails();

      album->addRef();
    }

    return album;
  }

  void AlbumStore::removeAlbum(sp_album *spAlbum) {
    albumMap::iterator it = m_albums.find(spAlbum);
    SxAlbum *album;
    if (it != m_albums.end()) {
      album = it->second;
      if (album->getReferencesCount() <= 1) {
        m_albums.erase(spAlbum);
        // Logger::printOut("deleting album");
        delete album;
      } else {
        album->rmRef();
        // Logger::printOut("lower album ref");
      }
    }
  }

  void AlbumStore::removeAlbum(SxAlbum* album) {
    removeAlbum(album->getSpAlbum());
  }

} /* namespace addon_music_spotify */
