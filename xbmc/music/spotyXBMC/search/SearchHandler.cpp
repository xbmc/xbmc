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

#include "SearchHandler.h"
#include <stdio.h>
#include "../Logger.h"

using namespace std;

namespace addon_music_spotify {

  SearchHandler* SearchHandler::m_instance = 0;
  SearchHandler *SearchHandler::getInstance() {
    return m_instance ? m_instance : (m_instance = new SearchHandler);
  }

  SearchHandler::SearchHandler() {
    m_currentSearch = NULL;

  }

  void SearchHandler::deInit() {
    delete m_instance;
  }

  bool SearchHandler::search(string query) {
    if (m_currentSearch != NULL) {
      Logger::printOut("m_currentSearch not NULL");
      if (m_currentSearch->getQuery() == query) {
        //if its the same query we are holding, return false and fetch albums instead
        return false;
      }
      Logger::printOut("delete m_currentSearch");
      delete m_currentSearch;
      m_currentSearch = NULL;
    }
    Logger::printOut("creating m_currentSearch");
    m_currentSearch = new Search(query);
    Logger::printOut("returning m_currentSearch");
    return true;
  }

  vector<SxAlbum*> SearchHandler::getAlbumResults() {
    if (m_currentSearch != NULL) return m_currentSearch->getAlbums();
  }

  vector<SxTrack*> SearchHandler::getTrackResults() {
    if (m_currentSearch != NULL) return m_currentSearch->getTracks();
  }

  vector<SxArtist*> SearchHandler::getArtistResults() {
    if (m_currentSearch != NULL) return m_currentSearch->getArtists();
  }

  SearchHandler::~SearchHandler() {
    if (m_currentSearch != NULL) delete m_currentSearch;
  }

} /* namespace addon_music_spotify */
