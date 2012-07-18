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

#ifndef SXTRACK_H_
#define SXTRACK_H_

#include <string>
#include <libspotify/api.h>
#include "URL.h"

using namespace std;

namespace addon_music_spotify {

  class TrackStore;
  class SxAlbum;
  class SxThumb;
  class SxTrack {
  public:

    void addRef() {
      m_references++;
    }

    int getReferencesCount() {
      return m_references;
    }

    bool isLoaded();

    int getDuration() const {
      return m_duration;
    }

    int getRating() const {
      return m_rating;
    }

    int getTrackNumber() const {
      return m_trackNumber;
    }

    sp_track* getSpTrack() {
      return m_spTrack;
    }

    string getName() {
      return m_name;
    }

    string getArtistName() {
      return m_artistName;
    }

    string getAlbumName() {
      return m_albumName;
    }

    string getAlbumArtistName() {
      return m_albumArtistName;
    }

    int getYear() {
      return m_year;
    }

    SxThumb* getThumb() {
      return m_thumb;
    }

    bool hasThumb() {
      return m_hasTHumb;
    }

    CStdString *getFanart(){
    	return m_fanart;
    }

    const char* getUri() {
      return m_uri;
    }

    int getDisc() {
      return sp_track_disc(m_spTrack);
    }

    friend class TrackStore;

  private:
    SxTrack(sp_track* spTrack);
    virtual ~SxTrack();

    void rmRef() {
      m_references--;
    }

    int m_references;
    sp_track* m_spTrack;
    SxThumb* m_thumb;
    CStdString *m_fanart;
    bool m_hasTHumb;
    char* m_uri;
    string m_name;
    string m_artistName;
    string m_albumName;
    string m_albumArtistName;
    int m_year;
    int m_duration;
    int m_rating;
    int m_trackNumber;
  };

} /* namespace addon_music_spotify */
#endif /* SXTRACK_H_ */
