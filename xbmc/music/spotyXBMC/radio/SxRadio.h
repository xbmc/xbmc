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

#ifndef SXRADIO_H_
#define SXRADIO_H_

#include <libspotify/api.h>
#include "../track/TrackContainer.h"
#include <vector>

using namespace std;

namespace addon_music_spotify {

  class SxTrack;
  class SxRadio: private TrackContainer {
  public:

    static void SP_CALLCONV cb_searchComplete(sp_search *search, void *userdata);
    void newResults(sp_search* search);

    //this is called from the player when it is advancing to next track
    void pushToTrack(int trackNumber);

    bool getTrackItems(CFileItemList& items);

    friend class RadioHandler;
    friend class RadioBackgroundLoader;


  private:
    SxRadio(int radioNumber, int fromYear, int toYear, sp_radio_genre genres);
    virtual ~SxRadio();

    bool isLoaded();

    int m_numberOfTracksToDisplay;
    int m_radioNumber;

    void fetchNewTracks();

    bool m_isWaitingForResults;
    sp_search* m_currentSearch;

    sp_radio_genre m_genres;
    int m_fromYear;
    int m_toYear;
    int m_currentPlayingPos;
    int m_currentResultPos;
  };

} /* namespace addon_music_spotify */
#endif /* SXRADIO_H_ */
