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

#ifndef SXALBUM_H_
#define SXALBUM_H_

#include <libspotify/api.h>
#include <vector>
#include <string>
#include "../thumb/SxThumb.h"
#include "../track/TrackContainer.h"
#include "URL.h"

using namespace std;

namespace addon_music_spotify {

  class AlbumStore;
  class SxAlbum: private TrackContainer {
  public:

    static void SP_CALLCONV cb_albumBrowseComplete(sp_albumbrowse *result, void *userdata);
    void doLoadTracksAndDetails();
    void doLoadThumb();
    void tracksLoaded(sp_albumbrowse *result);
    const char *getReview() const;

    friend class AlbumStore;

    void addRef() {
      m_references++;
    }

    int getReferencesCount() {
      return m_references;
    }

    bool isLoaded() {
      return !m_isLoadingTracks && (!m_hasThumb || (m_hasThumb && m_thumb->isLoaded()));
    }

    const char* getAlbumName() {
      return sp_album_name(m_spAlbum);
    }
    const char* getAlbumArtistName() {
      return sp_artist_name(sp_album_artist(m_spAlbum));
    }
    int getAlbumRating() {
      return m_rating;
    }
    SxThumb* getAlbumThumb() {
      return m_thumb;
    }
    sp_album* getSpAlbum() {
      return m_spAlbum;
    }
    vector<SxTrack*> getTracks() {
      return m_tracks;
    }

    bool isStarred();
    bool toggleStar();

    bool getTrackItems(CFileItemList& items);

    bool hasTracksAndDetails() {
      return m_hasTracksAndDetails;
    }
    bool hasThumb() {
      return m_hasThumb;
    }
    bool isLoadingThumb() {
      return m_thumb->isLoaded();
    }

    int getAlbumYear() {
      return m_year;
    }

    string getReview() {
      return m_review;
    }

    const char *getUri() {
      return m_uri;
    }

    int getNumberOfDiscs() {
      return m_numberOfDiscs;
    }

    SxThumb* getThumb() {
      return m_thumb;
    }

    CStdString *getFanart(){
    	return m_fanart;
    }

  private:
    SxAlbum(sp_album *album, bool loadTracksAndDetails);
    virtual ~SxAlbum();

    void rmRef() {
      m_references--;
    }

    int m_numberOfDiscs;

    bool m_isLoadingTracks;
    bool m_isLoadingThumb;
    bool m_hasTracksAndDetails;
    bool m_hasThumb;
    CStdString *m_fanart;
    int m_references;

    char *m_uri;
    sp_album *m_spAlbum;
   // vector<SxTrack*> m_tracks;
    SxThumb *m_thumb;
    string m_review;
    int m_rating;
    int m_year;
  };

} /* namespace addon_music_spotify */
#endif /* SXALBUM_H_ */
