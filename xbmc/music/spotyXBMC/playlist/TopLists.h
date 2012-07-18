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

#ifndef TOPLISTS_H_
#define TOPLISTS_H_

#ifndef SP_CALLCONV
#ifdef _WIN32
#define SP_CALLCONV __stdcall
#else
#define SP_CALLCONV
#endif
#endif

#include <libspotify/api.h>
#include <vector>
#include "../../../threads/SystemClock.h"
#include "../track/TrackStore.h"
#include "../album/AlbumStore.h"
#include "../artist/ArtistStore.h"
#include "../artist/ArtistContainer.h"
#include "../album/AlbumContainer.h"
#include "../track/TrackContainer.h"

using namespace std;

namespace addon_music_spotify {

  class TopLists: private AlbumContainer, private TrackContainer, private ArtistContainer {
  public:
    TopLists();
    virtual ~TopLists();

    static void SP_CALLCONV cb_toplistArtistsComplete(sp_toplistbrowse *result, void *userdata);
    void artistListLoaded(sp_toplistbrowse *result);

    static void SP_CALLCONV cb_toplistAlbumsComplete(sp_toplistbrowse *result, void *userdata);
    void albumListLoaded(sp_toplistbrowse *result);

    static void SP_CALLCONV cb_toplistTracksComplete(sp_toplistbrowse *result, void *userdata);
    void trackListLoaded(sp_toplistbrowse *result);

    //we can load on demand, so even if nothing is loaded, the object is loaded
    bool isLoaded() {
      return true;
    }

    //call this from the advance api thread, it will reload every 12 hours
    //TODO actually do it!
    void reLoadArtists();
    void reLoadAlbums();
    void reLoadTracks();
    void unloadLists();

    bool isArtistsLoaded();
    bool isAlbumsLoaded();
    bool isTracksLoaded();

    vector<SxArtist*> getArtists() {
      return m_artists;
    }

    bool getArtistItems(CFileItemList& items);

    vector<SxAlbum*> getAlbums() {
      return m_albums;
    }

    bool getAlbumItems(CFileItemList& items);

    vector<SxTrack*> getTracks() {
      return m_tracks;
    }

    bool getTrackItems(CFileItemList& items);

  private:
    bool m_albumsLoaded;
    bool m_artistsLoaded;
    bool m_tracksLoaded;
    bool m_waitingForAlbums;
    bool m_waitingForArtists;
    bool m_waitingForTracks;
    XbmcThreads::EndTime m_albumsNextReload;
    XbmcThreads::EndTime m_artistsNextReload;
    XbmcThreads::EndTime m_tracksNextReload;
  };

} /* namespace addon_music_spotify */
#endif /* TOPLISTS_H_ */
