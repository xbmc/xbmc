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

#ifndef SXPLAYLIST_H_
#define SXPLAYLIST_H_

#include <libspotify/api.h>
#include "../track/TrackContainer.h"
#include <vector>

namespace addon_music_spotify {

  using namespace std;
  class TrackStore;
  class ThumbStore;
  class SxThumb;
  class SxTrack;

  class SxPlaylist: protected TrackContainer {
  public:
    SxPlaylist(sp_playlist* spSxPlaylist, int index, bool isFolder);
    virtual ~SxPlaylist();

    void init();

    static void SP_CALLCONV cb_state_change(sp_playlist *pl, void *userdata);
    static void SP_CALLCONV cb_tracks_added(sp_playlist *pl, sp_track * const *tracks, int num_tracks, int position, void *userdata);
    static void SP_CALLCONV cb_tracks_removed(sp_playlist *pl, const int *tracks, int num_tracks, void *userdata);
    static void SP_CALLCONV cb_playlist_renamed(sp_playlist *pl, void *userdata);
    static void SP_CALLCONV cb_playlist_metadata_updated(sp_playlist *pl, void *userdata);
    static void SP_CALLCONV cb_tracks_moved(sp_playlist *pl, const int *tracks, int num_tracks, int new_position, void *userdata);

    virtual bool isLoaded();
    const char* getName();
    const char* getOwnerName();
    int getNumberOfTracks() {
      return m_tracks.size();
    }
    int getIndex() {
      return m_index;
    }
    bool isFolder() {
      return m_isFolder;
    }
    SxTrack* getTrack(int index);

    bool getTrackItems(CFileItemList& items);

    SxThumb* getThumb() {
      return m_thumb;
    }
    virtual void reLoad();
    //call this when the playlist is removed from spotify and the pointer is no longer valid
    void makeInvalid() {
      m_isValid = false;
    }

  protected:
    bool m_isValid;
    sp_playlist* m_spPlaylist;

  private:
    bool m_isFolder;
    int m_index;
    SxThumb* m_thumb;
    sp_playlist_callbacks m_plCallbacks;
    const char* ownerName;

  };

} /* namespace addon_music_spotify */
#endif /* SXPLAYLIST_H_ */
