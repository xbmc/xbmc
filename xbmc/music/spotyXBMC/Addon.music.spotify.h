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

#ifndef ADDONMUSICSPOTIFY_H_
#define ADDONMUSICSPOTIFY_H_

#include "track/SxTrack.h"
#include "artist/SxArtist.h"
#include "session/Session.h"
#include <vector>
#include <string>
#include "FileItem.h"
#include "../../cores/paplayer/ICodec.h"
#include "../../dialogs/GUIDialogContextMenu.h"

using namespace std;
using namespace addon_music_spotify;

class Addon_music_spotify {
public:
  Addon_music_spotify();
  virtual ~Addon_music_spotify();

  bool enable(bool enable);
  bool isEnabled() {
    return m_isEnabled;
  }
  bool isReady();

  bool GetTracks(CFileItemList& items, CStdString& path, CStdString artistName, int albumId);
  bool GetOneTrack(CFileItemList& items, CStdString& path);
  bool GetAlbums(CFileItemList& items, CStdString& path, CStdString artistName);
  bool GetArtists(CFileItemList& items, CStdString& path);
  bool GetPlaylists(CFileItemList& items);
  bool GetTopLists(CFileItemList& items);
  bool GetCustomEntries(CFileItemList& items);

  // Context menu related functions
  bool GetContextButtons(CFileItemPtr& item, CContextButtons &buttons);
  bool ToggleStarTrack(CFileItemPtr& item);
  bool ToggleStarAlbum(CFileItemPtr& item);

  bool Search(CStdString query, CFileItemList& items);
  ICodec* GetCodec();

private:
  bool m_isEnabled;
  bool getAlbumTracksFromTrack(CFileItemList& items, CStdString& trackUri);
  bool getAlbumTracks(CFileItemList& items, CStdString& path);
  bool getArtistTracks(CFileItemList& items, CStdString& path);
  bool getAllTracks(CFileItemList& items, CStdString& path);
  bool getPlaylistTracks(CFileItemList& items, int index);
  bool getTopListTracks(CFileItemList& items);
  bool getRadioTracks(CFileItemList& items, int radio);

  bool getAllAlbums(CFileItemList& items, CStdString& path);
  bool getArtistAlbums(CFileItemList& items, CStdString& path);
  bool getArtistAlbums(CFileItemList& items, sp_artist* spArtist);
  bool getTopListAlbums(CFileItemList& items);

  bool getAllArtists(CFileItemList& items);
  bool getArtistSimilarArtists(CFileItemList& items, CStdString uri);
  bool getTopListArtists(CFileItemList& items);
};
extern Addon_music_spotify* g_spotify;
#endif /* ADDONMUSICSPOTIFY_H_ */
