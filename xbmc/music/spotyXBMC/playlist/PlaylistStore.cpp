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

#include "PlaylistStore.h"
#include "../session/Session.h"

#include "SxPlaylist.h"
#include "StarredList.h"
#include "TopLists.h"
#include "../session/Session.h"
#include "../track/TrackStore.h"
#include "../thumb/ThumbStore.h"
#include "../track/SxTrack.h"
#include "../thumb/SxThumb.h"
#include "../Utils.h"

namespace addon_music_spotify {

  PlaylistStore::PlaylistStore() {
    Logger::printOut("init playliststore");
    m_isLoaded = false;
    m_starredList = NULL;
    m_topLists = NULL;
    m_spContainer = sp_session_playlistcontainer(Session::getInstance()->getSpSession());
    m_spStarredList = sp_session_starred_create(Session::getInstance()->getSpSession());
    m_spInboxList = sp_session_inbox_create(Session::getInstance()->getSpSession());

    for (int i = 0; i < sp_playlistcontainer_num_playlists(m_spContainer); i++) {
      sp_playlist_set_in_ram(Session::getInstance()->getSpSession(), sp_playlistcontainer_playlist(m_spContainer, i), true);
    }

    sp_playlist_set_in_ram(Session::getInstance()->getSpSession(), m_spInboxList , true);
    sp_playlist_set_in_ram(Session::getInstance()->getSpSession(), m_spStarredList, true);

    m_pcCallbacks.playlist_added = &pc_playlist_added;
    m_pcCallbacks.playlist_removed = &pc_playlist_removed;
    m_pcCallbacks.container_loaded = &pc_loaded;
    m_pcCallbacks.playlist_moved = &pc_playlist_moved;

    sp_playlistcontainer_add_callbacks(m_spContainer, &m_pcCallbacks, this);
  }

  PlaylistStore::~PlaylistStore() {
    sp_playlistcontainer_remove_callbacks(m_spContainer, &m_pcCallbacks, this);
    Logger::printOut("delete PlaylistStore");
    while (!m_playlists.empty()) {
      delete m_playlists.back();
      m_playlists.pop_back();
    }
    Logger::printOut("delete PlaylistStore starred");
    delete m_starredList;
    delete m_topLists;
    m_starredList = NULL;

    Logger::printOut("delete PlaylistStore done");
  }

  bool PlaylistStore::isLoaded() {
    if (!m_isLoaded) return false;

    for (int i = 0; i < m_playlists.size(); i++) {
      if (!m_playlists[i]->isLoaded()) {
        return false;
      }
    }
    if (m_starredList) if (!m_starredList->isLoaded()) return false;

    if (m_topLists) if (!m_topLists->isLoaded()) return false;

    return true;
  }

  const char *PlaylistStore::getPlaylistName(int index) {
    if (index < m_playlists.size()) {

      return m_playlists[index]->getName();
    }
    return "";
  }

  SxPlaylist *PlaylistStore::getPlaylist(int index) {
    if (index < m_playlists.size()) {
      return m_playlists[index];
    }
    return NULL;
  }

  void PlaylistStore::pc_loaded(sp_playlistcontainer *pc, void *userdata) {
    Logger::printOut("pc loaded");
    PlaylistStore *store = (PlaylistStore*) userdata;

    vector<SxPlaylist*> newPlaylists;
    int playlistNumber = 0;
    //add the inbox playlist first
    newPlaylists.push_back(new SxPlaylist(store->m_spInboxList, playlistNumber, false));
    playlistNumber++;


    for (int i = 0; i < sp_playlistcontainer_num_playlists(store->getContainer()); i++) {
      sp_playlist_type spType = sp_playlistcontainer_playlist_type(store->m_spContainer, i);
      if (spType == SP_PLAYLIST_TYPE_PLAYLIST) {
        newPlaylists.push_back(new SxPlaylist(sp_playlistcontainer_playlist(store->getContainer(), i), playlistNumber, false));
        playlistNumber++;
      }
    }

    Utils::updatePlaylists();

    store->m_starredList = new StarredList(store->m_spStarredList);
    Logger::printOut("m_starredList created");

    Logger::printOut("All playlists loaded");

    //empty the old one if we are updating
    while (!store->m_playlists.empty()) {
      delete store->m_playlists.back();
      store->m_playlists.pop_back();
    }
    store->m_playlists = newPlaylists;

    if (store->m_topLists == NULL
    ) store->m_topLists = new TopLists();

    store->m_isLoaded = true;
    Utils::updateToplistMenu();

  }

  void PlaylistStore::pc_playlist_added(sp_playlistcontainer *pc, sp_playlist *playlist, int position, void *userdata) {
    PlaylistStore *store = (PlaylistStore*) userdata;
    store->addPlaylist(playlist, position);
  }

  void PlaylistStore::pc_playlist_moved(sp_playlistcontainer *pc, sp_playlist *playlist, int position, int new_position, void *userdata) {
    PlaylistStore *store = (PlaylistStore*) userdata;
    store->movePlaylist(position, new_position);
  }

  void PlaylistStore::pc_playlist_removed(sp_playlistcontainer *pc, sp_playlist *playlist, int position, void *userdata) {
    PlaylistStore *store = (PlaylistStore*) userdata;
    store->removePlaylist(position);
  }

  StarredList *PlaylistStore::getStarredList() {
    return m_starredList;
  }

  void PlaylistStore::removePlaylist(int position) {
    Logger::printOut("removing playlist");
    Logger::printOut(position);
    if (position < m_playlists.size()) {
      Logger::printOut("removing playlist inner");
      m_isLoaded = false;
      m_playlists[position]->makeInvalid();
      Logger::printOut("removing playlist inner 2");
    }
  }

  void PlaylistStore::movePlaylist(int position, int newPosition) {
    Logger::printOut("moving playlist");
    //skip this now since the playlists are ordered by name anyway
    //m_isLoaded = false;
  }

  void PlaylistStore::addPlaylist(sp_playlist *playlist, int position) {
    Logger::printOut("adding playlist");
    m_isLoaded = false;
  }

} /* namespace addon_music_spotify */
