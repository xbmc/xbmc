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

#include "TrackStore.h"
#include "../Logger.h"
#include "SxTrack.h"

using namespace std;

namespace addon_music_spotify {

  TrackStore::TrackStore() {
    // TODO Auto-generated constructor stub

  }

  void TrackStore::deInit() {
    delete m_instance;
  }

  TrackStore::~TrackStore() {
    for (trackMap::iterator it = m_tracks.begin(); it != m_tracks.end(); ++it) {
      delete it->second;
    }
  }

  TrackStore* TrackStore::m_instance = 0;
  TrackStore *TrackStore::getInstance() {
    return m_instance ? m_instance : (m_instance = new TrackStore);
  }

  SxTrack* TrackStore::getTrack(sp_track *spTrack) {
    //Logger::printOut("asking store for track");
    sp_track_add_ref(spTrack);
    while (!sp_track_is_loaded(spTrack))
      ;
    //Logger::printOut("track done loading");
    if (sp_track_error(spTrack) != SP_ERROR_OK) {
      Logger::printOut("error in track");
      return NULL;
    }
    trackMap::iterator it = m_tracks.find(spTrack);

    SxTrack *track;
    if (it == m_tracks.end()) {
      //we need to create it
      track = new SxTrack(spTrack);
      m_tracks.insert(trackMap::value_type(spTrack, track));
      //Logger::printOut("adding track to store");
    } else {
      //Logger::printOut("loading track from store");
      track = it->second;

      track->addRef();
    }

    return track;
  }

  void TrackStore::removeTrack(sp_track *spTrack) {
    trackMap::iterator it = m_tracks.find(spTrack);
    SxTrack *track;
    if (it != m_tracks.end()) {
      track = it->second;
      if (track->getReferencesCount() <= 1) {
        m_tracks.erase(spTrack);
        // Logger::printOut("removing track!");
        delete track;
      } else {
        // Logger::printOut("lower track ref!");
        track->rmRef();
      }
    }
  }

  void TrackStore::removeTrack(SxTrack* track) {
    removeTrack(track->getSpTrack());
  }

} /* namespace addon_music_spotify */
