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

#ifndef SXSETTINGS_H_
#define SXSETTINGS_H_

#include <libspotify/api.h>
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"

namespace addon_music_spotify {

  using namespace std;

  class Settings {
  public:

    static Settings *getInstance();
    bool init();
    void deInit();

    bool enabled() {
      return m_enabled;
    }

    CStdString getUserName() {
      return m_userName;
    }

    CStdString getPassword() {
      return m_password;
    }

    CStdString getCachePath() {
    	return m_cachePath;
    }

    CStdString getThumbPath() {
    	return m_thumbPath;
    }

    CStdString getArtistThumbPath() {
    	return m_artistThumbPath;
    }

    bool useHighBitrate() {
      return m_useHighBitrate;
    }
    bool useNormalization(){
    	return m_useNormalization;
    }

    CStdString getFanart() {
    	return m_fanart;
    }

    bool getUseHTFanarts() {
      return m_useHTFanarts;
    }
    bool getUseHTArtistThumbs() {
      return m_useHTArtistThumbs;
    }

    int getStartDelay() {
      return m_startDelay;
    }

    bool saveLogToFile() {
      return m_saveLogToFile;
    }

    int getSearchNumberArtists() {
      return m_searchNumberArtists;
    }

    int getSearchNumberAlbums() {
      return m_searchNumberAlbums;
    }

    int getSearchNumberTracks() {
      return m_searchNumberTracks;
    }

    bool getPreloadArtistDetails() {
      return m_preloadArtistDetails;
    }

    int getArtistNumberArtists() {
    	return m_artistNumberArtists;
    }

    int getArtistNumberAlbums() {
    	return m_artistNumberAlbums;
    }

    int getArtistNumberTracks() {
    	return m_artistNumberTracks;
    }

    CStdString getRadio1Name() {
      return m_radio1Name;
    }

    int getRadio1From() {
      return m_radio1From;
    }

    int getRadio1To() {
      return m_radio1To;
    }

    sp_radio_genre getRadio1Genres() {
    	return m_radio1Genres;
    }

    CStdString getRadio2Name() {
      return m_radio2Name;
    }

    int getRadio2From() {
      return m_radio2From;
    }

    int getRadio2To() {
      return m_radio2To;
    }

    sp_radio_genre getRadio2Genres() {
    	return m_radio2Genres;
    }

    int getRadioNumberTracks() {
      return m_radioNumberTracks;
    }

    bool toplistRegionEverywhere() {
      return m_toplistRegionEverywhere;
    }

    bool getPreloadTopLists() {
      return m_preloadTopLists;
    }

    CStdString getByString() {
      return m_byString;
    }

    CStdString getTopListArtistString() {
      return m_topListArtistString;
    }

    CStdString getTopListAlbumString() {
      return m_topListAlbumString;
    }

    CStdString getTopListTrackString() {
      return m_topListTrackString;
    }

    CStdString getRadioPrefixString() {
      return m_radioPrefixString;
    }

    CStdString getSimilarArtistsString() {
      return m_similarArtistsString;
    }

    CStdString getInboxString() {
      return m_inboxString;
    }

    CStdString getStarTrackString() {
      return m_starTrackString;
    }

    CStdString getUnstarTrackString() {
      return m_unstarTrackString;
    }

    CStdString getStarAlbumString() {
      return m_starAlbumString;
    }

    CStdString getUnstarAlbumString() {
      return m_unstarAlbumString;
    }

    CStdString getBrowseAlbumString() {
      return m_browseAlbumString;
    }

    CStdString getBrowseArtistString() {
      return m_browseArtistString;
    }

  private:
    bool m_enabled;
    bool m_useHighBitrate;
    bool m_useNormalization;
    bool m_useHTFanarts;
    bool m_useHTArtistThumbs;
    bool m_saveLogToFile;
    bool m_preloadArtistDetails;
    bool m_toplistRegionEverywhere;
    bool m_preloadTopLists;

    int m_startDelay;
    int m_searchNumberArtists;
    int m_searchNumberAlbums;
    int m_searchNumberTracks;
    int m_radio1From;
    int m_radio1To;
    int m_radio2From;
    int m_radio2To;
    int m_radioNumberTracks;
    int m_artistNumberArtists;
    int m_artistNumberAlbums;
    int m_artistNumberTracks;

    sp_radio_genre m_radio1Genres;
    sp_radio_genre m_radio2Genres;

    CStdString m_userName;
    CStdString m_password;
    CStdString m_cachePath;
    CStdString m_thumbPath;
    CStdString m_artistThumbPath;
    CStdString m_byString;
    CStdString m_fanart;
    CStdString m_radio1Name;
    CStdString m_radio2Name;
    CStdString m_topListArtistString;
    CStdString m_topListAlbumString;
    CStdString m_topListTrackString;
    CStdString m_radioPrefixString;
    CStdString m_similarArtistsString;
    CStdString m_inboxString;
    CStdString m_starTrackString;
    CStdString m_unstarTrackString;
    CStdString m_starAlbumString;
    CStdString m_unstarAlbumString;
    CStdString m_browseAlbumString;
    CStdString m_browseArtistString;

    sp_radio_genre getRadioGenres(ADDON::AddonPtr addon, int radio);

    Settings();
    virtual ~Settings();

    static Settings *m_instance;
  };

} /* namespace addon_music_spotify */
#endif /* SXSETTINGS_H_ */
