/*
 * TrackContainer.cpp
 *
 *  Created on: Aug 17, 2011
 *      Author: david
 */

#include "AlbumContainer.h"
#include "AlbumStore.h"

namespace addon_music_spotify {

 AlbumContainer::AlbumContainer() {
    // TODO Auto-generated constructor stub

  }

 AlbumContainer::~AlbumContainer() {
    // TODO Auto-generated destructor stub
  }

  void AlbumContainer::removeAllAlbums() {
    while (!m_albums.empty()) {
      AlbumStore::getInstance()->removeAlbum(m_albums.back());
      m_albums.pop_back();
    }
  }

  bool AlbumContainer::albumsLoaded() {
    for (int i = 0; i < m_albums.size(); i++) {
      if (!m_albums[i]->isLoaded()) return false;
    }
    return true;
  }

} /* namespace addon_music_spotify */
