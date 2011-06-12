/*
    spotyxbmc - A project to integrate Spotify into XBMC
    Copyright (C) 2010  David Erenger

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

#pragma once

#ifndef SP_CALLCONV
#ifdef _WIN32
#define SP_CALLCONV __stdcall
#else
#define SP_CALLCONV
#endif
#endif

namespace spotify {
#include <libspotify/api.h>
}

using namespace spotify;
#include <stdio.h>
#include <stdint.h>
#include <cstdlib>
#include <vector>
#include "utils/StringUtils.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "Util.h"
#include "FileItem.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "guilib/GUIDialog.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"

class SpotifyInterface
{
public:
  SpotifyInterface();
  ~SpotifyInterface();

  enum SPOTIFY_TYPE{
    SEARCH_ARTIST,
    SEARCH_ALBUM,
    SEARCH_TRACK,
    ARTISTBROWSE_ARTIST,
    ARTISTBROWSE_ALBUM,
    ALBUMBROWSE_TRACK,
    PLAYLIST_TRACK,
    TOPLIST_ARTIST,
    TOPLIST_ALBUM,
    TOPLIST_TRACK
  };

  //session functions
  bool connect(bool forceNewUser = false);
  bool disconnect();
  bool reconnect(bool forceNewUser = false);
  bool processEvents();
  sp_session * getSession(){return m_session; }

  //callback functions definied in api.h
  static void SP_CALLCONV cb_connectionError(sp_session *session, sp_error error);
  static void SP_CALLCONV cb_loggedIn(sp_session *session, sp_error error);
  static void SP_CALLCONV cb_notifyMainThread(sp_session *session);
  static void SP_CALLCONV cb_logMessage(sp_session *session, const char *data);
  static void SP_CALLCONV cb_searchComplete(sp_search *search, void *userdata);
  static void SP_CALLCONV cb_albumBrowseComplete(sp_albumbrowse *result, void *userdata);
  static void SP_CALLCONV cb_topListAritstsComplete(sp_toplistbrowse *result, void *userdata);
  static void SP_CALLCONV cb_topListAlbumsComplete(sp_toplistbrowse *result, void *userdata);
  static void SP_CALLCONV cb_topListTracksComplete(sp_toplistbrowse *result, void *userdata);
  static void SP_CALLCONV cb_artistBrowseComplete(sp_artistbrowse *result, void *userdata);
  static int SP_CALLCONV cb_musicDelivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames);
  static void SP_CALLCONV cb_imageLoaded(sp_image *image, void *userdata);

  bool getDirectory(const CStdString &strPath, CFileItemList &items);
  XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE getChildType(const CStdString &strPath);

private:
  sp_session *m_session;
  CGUIDialogProgress *searchProgress;
  sp_session_config m_config;
  sp_error m_error;
  sp_session_callbacks m_callbacks;
  bool m_showDisclaimer;
  int m_nextEvent;
  const char *m_uri;
  CStdString m_thumbDir;
  CStdString m_playlistsThumbDir;
  CStdString m_toplistsThumbDir;
  CStdString m_currentPlayingDir;
  void clean(bool search, bool artistbrowse, bool albumbrowse, bool playlists, bool toplists, bool searchthumbs, bool playliststhumbs, bool toplistthumbs, bool currentplayingthumbs);
  void clean();

  //functions for searching
  bool search();
  bool search(CStdString searchstring);
  bool hasSearchResults() { return (!m_searchArtistVector.IsEmpty() || !m_searchAlbumVector.IsEmpty() || !m_searchTrackVector.IsEmpty()); }
  void waitForThumbs();

  //menus
  void getMainMenuItems(CFileItemList &items);
  void getSettingsMenuItems(CFileItemList &items);
  void getSearchMenuItems(CFileItemList &items);
  void getPlaylistItems(CFileItemList &items);

  //browsing album
  bool getBrowseAlbumTracks(CStdString strPath, CFileItemList &items);
  bool addAlbumToLibrary();

  //browsing album
  bool browseArtist(CStdString strPath);
  bool getBrowseArtistMenu(CStdString strPath, CFileItemList &items);
  bool getBrowseArtistAlbums(CStdString strPath, CFileItemList &items);
  bool getBrowseArtistArtists(CStdString strPath, CFileItemList &items);

  //browsing toplist
  void getBrowseToplistMenu(CFileItemList &items);
  bool getBrowseToplistAlbums(CFileItemList &items);
  bool getBrowseToplistArtists(CFileItemList &items);
  bool getBrowseToplistTracks(CFileItemList &items);

  //playlists
  bool getPlaylistTracks(CFileItemList &items, int playlist);

  //usefull dialogs
  CGUIDialogProgress *m_reconectingDialog;
  CGUIDialogProgress *m_progressDialog;

  //dialog functions
  CStdString getUsername();
  CStdString getPassword();
  void showDisclaimer();
  bool m_isShowingReconnect;
  void showReconectingDialog();
  void hideReconectingDialog();
  void showProgressDialog(CStdString message);
  void hideProgressDialog();
  void showConnectionErrorDialog(sp_error error);

  //search
  sp_search *m_search;
  CStdString m_searchStr;
  bool m_isSearching;
  CFileItemList m_searchArtistVector;
  CFileItemList m_searchAlbumVector;
  CFileItemList m_searchTrackVector;

  //browsing album
  sp_albumbrowse *m_albumBrowse;
  CStdString m_albumBrowseStr;
  CFileItemList m_browseAlbumVector;

  //browsing artist
  sp_artistbrowse *m_artistBrowse;
  CStdString m_artistBrowseStr;
  CFileItemList m_browseArtistMenuVector;
  CFileItemList m_browseArtistAlbumVector;
  CFileItemList m_browseArtistSimilarArtistsVector;

  //browsing toplist
  sp_toplistbrowse *m_toplistArtistsBrowse;
  sp_toplistbrowse *m_toplistAlbumsBrowse;
  sp_toplistbrowse *m_toplistTracksBrowse;
  CFileItemList m_browseToplistArtistsVector;
  CFileItemList m_browseToplistAlbumVector;
  CFileItemList m_browseToplistTracksVector;


  //playlists
  CFileItemList m_playlistItems;

  //converting functions
  CFileItemPtr spArtistToItem(sp_artist *spArtist);
  CFileItemPtr spAlbumToItem(sp_album *spAlbum, SPOTIFY_TYPE type);
  CFileItemPtr spTrackToItem(sp_track *spTrack, SPOTIFY_TYPE type, bool loadthumb = false);

  //thumbnail handling
  typedef std::pair<sp_image*,CFileItemPtr> imageItemPair;
  std::vector<imageItemPair> m_searchWaitingThumbs;
  std::vector<imageItemPair> m_artistWaitingThumbs;
  std::vector<imageItemPair> m_playlistWaitingThumbs;
  std::vector<imageItemPair> m_toplistWaitingThumbs;
  int m_noWaitingThumbs;
  bool requestThumb(unsigned char *imageId, CStdString Uri, CFileItemPtr pItem, SPOTIFY_TYPE type);
};

extern SpotifyInterface *g_spotifyInterface;
