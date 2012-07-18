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

#include "StarredList.h"
#include "../session/Session.h"
#include "../album/SxAlbum.h"
#include "../track/SxTrack.h"
#include "../artist/SxArtist.h"
#include "../album/AlbumStore.h"
#include "../track/TrackStore.h"
#include "../artist/ArtistStore.h"
#include "../Utils.h"

namespace addon_music_spotify {

  using namespace std;

  StarredList::StarredList(sp_playlist* spPlaylist) :
      SxPlaylist(spPlaylist, 0, false) {
    m_isBackgroundLoading = false;
    m_reload = true;
    populateAlbumsAndArtists();
  }

  StarredList::~StarredList() {
    removeAllTracks();
    removeAllAlbums();

    while (!m_artists.empty()) {
      ArtistStore::getInstance()->removeArtist(m_artists.back());
      m_artists.pop_back();
    }
  }

  void StarredList::populateAlbumsAndArtists() {
    Logger::printOut("Populate starred albums and artists");
    //OK so we got the tracks list populated within the playlist, now create the albums we want
    //we can't do it in this thread because the albums will never load... create a new thread
    m_backgroundLoader = new StarredBackgroundLoader();
    m_backgroundLoader->Create(true);
  }

  void StarredList::reLoad() {
    Logger::printOut("reload star");
    if (m_isBackgroundLoading) {
      Logger::printOut("starred is loading in the background, set it to redo the loading again");
      m_reload = true;
      return;
    }

    m_reload = true;
    populateAlbumsAndArtists();
  }

  SxAlbum *StarredList::getAlbum(int index) {
    if (index < getNumberOfAlbums()) return m_albums[index];
    return NULL;
  }

  bool StarredList::getAlbumItems(CFileItemList& items) {
    return true;
  }

  SxArtist *StarredList::getArtist(int index) {
    if (index < getNumberOfArtists()) return m_artists[index];
    return NULL;
  }

  bool StarredList::isLoaded() {
    if (!tracksLoaded() || !albumsLoaded()) return false;

    for (int i = 0; i < m_artists.size(); i++) {
      if (!m_artists[i]->isLoaded()) return false;
    }

    return true;
  }

} /* namespace addon_music_spotify */

