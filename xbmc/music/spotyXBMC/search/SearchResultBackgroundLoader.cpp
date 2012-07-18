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

#include "../album/AlbumStore.h"
#include "../track/TrackStore.h"
#include "../artist/ArtistStore.h"
#include "../session/Session.h"
#include "SearchResultBackgroundLoader.h"
#include "../Utils.h"

namespace addon_music_spotify {

  SearchResultBackgroundLoader::SearchResultBackgroundLoader(Search* search) : CThread("Spotify SearchResultBackgroundLoader"){
    m_search = search;
  }

  SearchResultBackgroundLoader::~SearchResultBackgroundLoader() {
    // TODO Auto-generated destructor stub
  }

  void SearchResultBackgroundLoader::OnStartup() {
  }

  void SearchResultBackgroundLoader::OnExit() {
  }

  void SearchResultBackgroundLoader::OnException() {
  }

  void SearchResultBackgroundLoader::Process() {
    while (!Session::getInstance()->lock()) {
      SleepMs(1);
    }

    if (m_search->m_cancelSearch) {
      Logger::printOut("search results arived, aborting due to request");
      sp_search_release(m_search->m_currentSearch);
      Session::getInstance()->unlock();
      return;
    }
    Logger::printOut("search results arived");

    //add the albums
    for (int index = 0; index < sp_search_num_albums(m_search->m_currentSearch); index++) {
      if (sp_album_is_available(sp_search_album(m_search->m_currentSearch, index))) {
        m_search->m_albums.push_back(AlbumStore::getInstance()->getAlbum(sp_search_album(m_search->m_currentSearch, index), true));
      }
    }

    //add the tracks
    for (int index = 0; index < sp_search_num_tracks(m_search->m_currentSearch); index++) {
      if (sp_track_get_availability(Session::getInstance()->getSpSession(), sp_search_track(m_search->m_currentSearch, index))) {
        m_search->m_tracks.push_back(TrackStore::getInstance()->getTrack(sp_search_track(m_search->m_currentSearch, index)));
      }
    }

    //add the artists
    for (int index = 0; index < sp_search_num_artists(m_search->m_currentSearch); index++) {
      //dont load the albums and tracks for all artists here, it takes forever
      m_search->m_artists.push_back(ArtistStore::getInstance()->getArtist(sp_search_artist(m_search->m_currentSearch, index), false));
    }

    sp_search_release(m_search->m_currentSearch);

    //wait for all albums, tracks and artists to load before calling out
    Session::getInstance()->unlock();
    while (!m_search->isLoaded()) {
      SleepMs(1);
    }

    Utils::updateSearchResults(m_search->m_query);
    Logger::printOut("search results done");
  }

}

/* namespace addon_music_spotify */
