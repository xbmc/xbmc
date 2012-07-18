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

#ifndef ARTISTSTORE_H_
#define ARTISTSTORE_H_

#include <libspotify/api.h>
#include <string>
#ifdef _WIN32
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif

namespace addon_music_spotify {
  class SxArtist;
  class ArtistStore {
  public:

    static ArtistStore *getInstance();
    static void deInit();

    SxArtist* getArtist(sp_artist *spArtist, bool loadDetails);
    void removeArtist(sp_artist *spArtist);
    void removeArtist(SxArtist* artist);

  private:

    ArtistStore();
    virtual ~ArtistStore();

    static ArtistStore *m_instance;

    typedef std::tr1::unordered_map<sp_artist*, SxArtist*> artistMap;
    artistMap m_artists;
  };

} /* namespace addon_music_spotify */
#endif /* ARTISTSTORE_H_ */
