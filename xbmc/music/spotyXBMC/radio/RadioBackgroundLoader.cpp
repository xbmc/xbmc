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

#include "RadioHandler.h"
#include "../album/AlbumStore.h"
#include "../track/TrackStore.h"
#include "../artist/ArtistStore.h"
#include "../session/Session.h"
#include "RadioBackgroundLoader.h"
#include "../Utils.h"

namespace addon_music_spotify {

  RadioBackgroundLoader::RadioBackgroundLoader(SxRadio* radio) : CThread("Spotify RadioBackgroundLoader") {
    m_radio = radio;
  }

  RadioBackgroundLoader::~RadioBackgroundLoader() {
    // TODO Auto-generated destructor stub
  }

  void RadioBackgroundLoader::OnStartup() {
  }

  void RadioBackgroundLoader::OnExit() {
  }

  void RadioBackgroundLoader::OnException() {
  }

  void RadioBackgroundLoader::Process() {
    if (m_radio->m_isWaitingForResults) return;
    while (!Session::getInstance()->lock()) {
      SleepMs(1);
    }

    //add the tracks
    if (m_radio->m_currentSearch != NULL) {
      //if there are no tracks, return and break
      if (sp_search_num_tracks(m_radio->m_currentSearch) == 0) return;

      for (int index = m_radio->m_currentResultPos; index < sp_search_num_tracks(m_radio->m_currentSearch) && m_radio->m_tracks.size() < m_radio->m_numberOfTracksToDisplay; index++) {
        if (sp_track_get_availability(Session::getInstance()->getSpSession(), sp_search_track(m_radio->m_currentSearch, index))) {
          m_radio->m_tracks.push_back(TrackStore::getInstance()->getTrack(sp_search_track(m_radio->m_currentSearch, index)));
        }
        m_radio->m_currentResultPos++;
      }
    }

    //are we still missing tracks? Do a new search
    if (m_radio->m_tracks.size() < m_radio->m_numberOfTracksToDisplay) {
      m_radio->m_isWaitingForResults = true;
      m_radio->m_currentSearch = sp_radio_search_create(Session::getInstance()->getSpSession(), m_radio->m_fromYear, m_radio->m_toYear, m_radio->m_genres, &m_radio->cb_searchComplete, m_radio);
      Session::getInstance()->unlock();
    } else {
      Logger::printOut("radio fetch tracks done");
      Session::getInstance()->unlock();
      while (!m_radio->isLoaded()) {
        SleepMs(1);
      }

      if (m_radio->m_tracks.size() > 0) RadioHandler::getInstance()->allTracksLoaded(m_radio->m_radioNumber);
    }
  }
}

/* namespace addon_music_spotify */
