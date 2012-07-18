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

#ifndef SEARCHHANDLER_H_
#define SEARCHHANDLER_H_

#include <vector>
#include <string>
#include <libspotify/api.h>
#include "Search.h"
#include "../album/SxAlbum.h"
#include "../artist/SxArtist.h"

using namespace std;
namespace addon_music_spotify {

  class SearchHandler {
  public:
    static SearchHandler *getInstance();
    static void deInit();

    bool search(string query);
    bool hasSearch() {
      return m_currentSearch != NULL;
    }
    vector<SxAlbum*> getAlbumResults();
    vector<SxTrack*> getTrackResults();
    vector<SxArtist*> getArtistResults();

  private:
    SearchHandler();
    virtual ~SearchHandler();

    Search* m_currentSearch;

    static SearchHandler *m_instance;
  };

} /* namespace addon_music_spotify */
#endif /* SEARCHHANDLER_H_ */
