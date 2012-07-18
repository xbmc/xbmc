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

#ifndef SEARCH_H_
#define SEARCH_H_

#include <libspotify/api.h>
#include <string>
#include <vector>
#include "../album/AlbumContainer.h"
#include "../artist/ArtistContainer.h"
#include "../track/TrackContainer.h"

using namespace std;

namespace addon_music_spotify {

  class Search: private TrackContainer, private AlbumContainer, private ArtistContainer  {
  public:
    Search(string query);
    virtual ~Search();

    string getQuery() {
      return m_query;
    }
    vector<SxTrack*> getTracks() {
      return m_tracks;
    }


    bool getTrackItems(CFileItemList& items);

    vector<SxAlbum*> getAlbums() {
      return m_albums;
    }

    bool getAlbumItems(CFileItemList& items);

    vector<SxArtist*> getArtists() {
      return m_artists;
    }

    bool getArtistItems(CFileItemList& items);

    static void SP_CALLCONV cb_searchComplete(sp_search *search, void *userdata);

    bool isLoaded();

    friend class SearchResultBackgroundLoader;

  private:
    void newResults(sp_search *search);

    string m_query;

    int m_maxAlbumResults;
    int m_maxArtistResults;
    int m_maxTrackResults;

    bool m_artistsDone;
    bool m_albumsDone;
    bool m_tracksDone;

    sp_search* m_currentSearch;

    bool m_cancelSearch;
  };

} /* namespace addon_music_spotify */
#endif /* SEARCH_H_ */
