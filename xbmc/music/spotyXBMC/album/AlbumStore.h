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

#ifndef ALBUMSTORE_H_
#define ALBUMSTORE_H_

#include <libspotify/api.h>
#ifdef _WIN32
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif


namespace addon_music_spotify {
  class SxAlbum;
  class AlbumStore {
  public:

    static AlbumStore *getInstance();
    static void deInit();

    SxAlbum* getAlbum(sp_album *spAlbum, bool loadTracksAndDetails);
    void removeAlbum(sp_album *spAlbum);
    void removeAlbum(SxAlbum* album);

  private:

    AlbumStore();
    virtual ~AlbumStore();

    static AlbumStore *m_instance;

    typedef std::tr1::unordered_map<sp_album*, SxAlbum*> albumMap;
    albumMap m_albums;
  };

} /* namespace addon_music_spotify */
#endif /* ALBUMSTORE_H_ */
