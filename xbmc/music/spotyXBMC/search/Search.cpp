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

#include "Search.h"
#include "../session/Session.h"
#include "../Logger.h"
#include "../Utils.h"
#include "../SxSettings.h"
#include "SearchResultBackgroundLoader.h"
#include "../album/SxAlbum.h"
#include "../track/SxTrack.h"
#include "../artist/SxArtist.h"

namespace addon_music_spotify {

  Search::Search(string query) {
    m_maxArtistResults = Settings::getInstance()->getSearchNumberArtists();
    m_maxAlbumResults = Settings::getInstance()->getSearchNumberAlbums();
    m_maxTrackResults = Settings::getInstance()->getSearchNumberTracks();

    m_query = query;

    m_artistsDone = false;
    m_albumsDone = false;
    m_tracksDone = false;

    //do the initial search
    m_cancelSearch = false;
    Logger::printOut("creating search");
    Logger::printOut(query);
    m_currentSearch = sp_search_create(Session::getInstance()->getSpSession(), m_query.c_str(), 0, m_maxTrackResults, 0, m_maxAlbumResults, 0, m_maxArtistResults, &cb_searchComplete, this);

  }

  Search::~Search() {
    //we need to wait for the results
    //m_cancelSearch = true;
    //while (m_currentSearch != NULL)
    //  ;
    Logger::printOut("cleaning after search");
    removeAllTracks();
    removeAllAlbums();
    removeAllArtists();
    Logger::printOut("cleaning after search done");

  }

  bool Search::getTrackItems(CFileItemList& items) {
    return true;
  }

  bool Search::getAlbumItems(CFileItemList& items) {
    return true;
  }

  bool Search::getArtistItems(CFileItemList& items) {
    return true;
  }

  bool Search::isLoaded() {
    return (tracksLoaded() && albumsLoaded() && artistsLoaded());
  }

  void Search::cb_searchComplete(sp_search *search, void *userdata) {
    Search* searchObj = (Search*) userdata;
    searchObj->m_currentSearch = search;
    SearchResultBackgroundLoader* loader = new SearchResultBackgroundLoader(searchObj);
    loader->Create(true);
  }

} /* namespace addon_music_spotify */
