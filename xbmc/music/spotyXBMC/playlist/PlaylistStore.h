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

#ifndef PLAYLISTCONTAINER_H_
#define PLAYLISTCONTAINER_H_

#include <libspotify/api.h>
#include <vector>

namespace addon_music_spotify {

  using namespace std;

  class StarredList;
  class SxPlaylist;
  class TopLists;

  class PlaylistStore {
  public:
    PlaylistStore();
    virtual ~PlaylistStore();

    void init();

    static void SP_CALLCONV pc_loaded(sp_playlistcontainer *pc, void *userdata);
    static void SP_CALLCONV pc_playlist_added(sp_playlistcontainer *pc, sp_playlist *playlist, int position, void *userdata);
    static void SP_CALLCONV pc_playlist_moved(sp_playlistcontainer *pc, sp_playlist *playlist, int position, int new_position, void *userdata);
    static void SP_CALLCONV pc_playlist_removed(sp_playlistcontainer *pc, sp_playlist *playlist, int position, void *userdata);

    bool isLoaded();
    int getPlaylistCount() {
      return m_playlists.size();
    }
    const char* getPlaylistName(int index);
    SxPlaylist* getPlaylist(int index);
    StarredList* getStarredList();
    sp_playlistcontainer* getContainer() {
      return m_spContainer;
    }
    sp_playlist* getStarredSpList() {
      return m_spStarredList;
    }

    TopLists* getTopLists() {
      return m_topLists;
    }

    void removePlaylist(int position);
    void movePlaylist(int position, int newPosition);
    void addPlaylist(sp_playlist* playlist, int position);

  private:

    static void *loadPlaylists(void *s);
    bool m_isLoaded;
    vector<SxPlaylist*> m_playlists;
    StarredList* m_starredList;
    TopLists* m_topLists;
    sp_playlist* m_spStarredList;
    sp_playlist* m_spInboxList;
    sp_playlistcontainer_callbacks m_pcCallbacks;
    sp_playlistcontainer *m_spContainer;

  };

} /* namespace addon_music_spotify */
#endif /* PLAYLISTCONTAINER_H_ */
