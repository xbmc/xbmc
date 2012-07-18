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

#include "TrackContainer.h"
#include "TrackStore.h"
#include "SxTrack.h"

namespace addon_music_spotify {

  TrackContainer::TrackContainer() {
    // TODO Auto-generated constructor stub

  }

  TrackContainer::~TrackContainer() {
    // TODO Auto-generated destructor stub
  }

  void TrackContainer::removeAllTracks() {
    while (!m_tracks.empty()) {
      TrackStore::getInstance()->removeTrack(m_tracks.back());
      m_tracks.pop_back();
    }
  }

  bool TrackContainer::tracksLoaded() {
    for (int i = 0; i < m_tracks.size(); i++) {
      if (!m_tracks[i]->isLoaded()) return false;
    }
    return true;
  }

} /* namespace addon_music_spotify */
