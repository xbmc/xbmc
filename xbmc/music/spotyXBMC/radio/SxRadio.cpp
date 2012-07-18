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

#include "SxRadio.h"
#include "../Logger.h"
#include "../track/SxTrack.h"
#include "../track/TrackStore.h"
#include "../session/Session.h"
#include "RadioHandler.h"
#include "RadioBackgroundLoader.h"

namespace addon_music_spotify {

  SxRadio::SxRadio(int radioNumber, int fromYear, int toYear, sp_radio_genre genres) {
    m_radioNumber = radioNumber;
    m_fromYear = fromYear;
    m_toYear = toYear;
    m_genres = genres;
    m_currentPlayingPos = 0;
    m_currentResultPos = 0;
    m_currentSearch = NULL;
    //hard code the number of tracks to 15 for now
    m_numberOfTracksToDisplay = Settings::getInstance()->getRadioNumberTracks();
    m_isWaitingForResults = false;
    fetchNewTracks();
  }

  SxRadio::~SxRadio() {
    removeAllTracks();
  }

  bool SxRadio::isLoaded() {
    if (m_isWaitingForResults) return false;
    return tracksLoaded();
  }

  bool SxRadio::getTrackItems(CFileItemList& items) {
    return true;
  }

  void SxRadio::pushToTrack(int trackNumber) {
    Logger::printOut("SxRadio::pushToTrack");
    while (m_currentPlayingPos < trackNumber) {
      //kind of stupid to use a vector here, a list or dequeue is maybe better, change sometime...
      TrackStore::getInstance()->removeTrack(m_tracks.front());
      m_tracks.erase(m_tracks.begin());
      m_currentPlayingPos++;
    }
    m_currentPlayingPos = trackNumber;
    fetchNewTracks();
  }

  void SxRadio::fetchNewTracks() {
    RadioBackgroundLoader* loader = new RadioBackgroundLoader(this);
    loader->Create(true);
  }

  void SxRadio::newResults(sp_search* search) {
    Logger::printOut("new radio result");
    m_currentSearch = search;
    m_currentResultPos = 0;
    m_isWaitingForResults = false;
    fetchNewTracks();
  }

  void SxRadio::cb_searchComplete(sp_search *search, void *userdata) {
    SxRadio* searchObj = (SxRadio*) userdata;
    searchObj->newResults(search);
  }

} /* namespace addon_music_spotify */
