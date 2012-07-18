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

#include "SxSettings.h"
#include "URL.h"

namespace addon_music_spotify {

	Settings* Settings::m_instance = 0;
	Settings *Settings::getInstance() {
		return m_instance ? m_instance : (m_instance = new Settings);
	}

	bool Settings::init(){
		ADDON::AddonPtr addon;
		const CStdString pluginId = "plugin.music.spotyXBMC";
		CStdString value = "";

		if (ADDON::CAddonMgr::Get().GetAddon(pluginId, addon)) {
		  ADDON::CAddonMgr::Get().LoadAddonDescription(pluginId, addon);

		  m_enabled = addon->GetSetting("enable") == "true";
		  if (!m_enabled)
		  	return false;
		  m_userName =  addon->GetSetting("username");
		  m_password = addon->GetSetting("password");

		  m_cachePath = CSpecialProtocol::TranslatePath("special://temp/spotify/cache/");
		  m_thumbPath = CSpecialProtocol::TranslatePath("special://temp/spotify/thumbs/");
		  m_artistThumbPath = CSpecialProtocol::TranslatePath("special://temp/spotify/artistthumbs/");

		  m_useHighBitrate = addon->GetSetting("highBitrate") == "true";
		  m_useNormalization = addon->GetSetting("normalization") == "true";

		  if (addon->GetSetting("enablefanart") == "true"){
		    if (addon->GetSetting("customfanart") == "true"){
		      m_fanart = addon->GetSetting("fanart");
		    }else{
		      m_fanart = addon->FanArt();
		    }
		  }

		  m_useHTFanarts = addon->GetSetting("htfanart") == "true";
		  m_useHTArtistThumbs =  addon->GetSetting("htartistthumb") == "true";
		  m_startDelay = 1000 * atoi(addon->GetSetting("delay"));
		  m_saveLogToFile =  addon->GetSetting("logtofile") == "true";
		  m_searchNumberArtists = 10 * atoi(addon->GetSetting("searchNoArtists")) + 1;
		  m_searchNumberAlbums = 10 * atoi(addon->GetSetting("searchNoAlbums")) + 1;
		  m_searchNumberTracks = 10 * atoi(addon->GetSetting("searchNoTracks")) + 1;
		  m_preloadArtistDetails = addon->GetSetting("preloadArtistDetails") == "true";

		  int i = atoi(addon->GetSetting("artistNoArtists")) + 1;
		  if (i == 11) m_artistNumberTracks = -1;
		  m_artistNumberArtists = 10 * i;

	    i = atoi(addon->GetSetting("artistNoAlbums")) + 1;
		  if (i == 11) m_artistNumberAlbums = -1;
		  m_artistNumberAlbums = 10 * i;

		  i = atoi(addon->GetSetting("artistNoTracks")) + 1;
		  if (i == 11) m_artistNumberTracks = -1;
		  m_artistNumberTracks = 10 * i;

		  m_radio1Name = addon->GetSetting("radio1name");
		  m_radio1From = 1900 + (10 * (4 + atoi(addon->GetSetting("radio1from"))));
		  m_radio1To = 1900 + (10 * (4 + atoi(addon->GetSetting("radio1to"))));

		  m_radio1Genres = getRadioGenres(addon, 1);

		  m_radio2Name = addon->GetSetting("radio2name");
		  m_radio2From = 1900 + (10 * (4 + atoi(addon->GetSetting("radio2from"))));
		  m_radio2To = 1900 + (10 * (4 + atoi(addon->GetSetting("radio2to"))));

		  m_radio2Genres = getRadioGenres(addon, 2);

		  m_radioNumberTracks = atoi(addon->GetSetting("radioNoTracks")) + 3;
		  m_toplistRegionEverywhere = addon->GetSetting("topListRegion") == "1";
		  m_preloadTopLists = addon->GetSetting("preloadToplists") == "true";
		  m_byString = addon->GetString(30002);
		  m_topListArtistString = addon->GetString(30500);
		  m_topListAlbumString = addon->GetString(30501);
		  m_topListTrackString = addon->GetString(30502);
		  m_radioPrefixString = addon->GetString(30503);
		  m_similarArtistsString = addon->GetString(30504);
		  m_inboxString = addon->GetString(30505);
		  m_starTrackString = addon->GetString(30600);
		  m_unstarTrackString = addon->GetString(30601);
		  m_starAlbumString = addon->GetString(30602);
		  m_unstarAlbumString = addon->GetString(30603);
		  m_browseAlbumString = addon->GetString(30604);
		  m_browseArtistString = addon->GetString(30605);

		  return true;
		}
		return false;
	}

	void Settings::deInit(){
		delete m_instance;
	}

  Settings::Settings() {
  }

  Settings::~Settings() {
  }

  sp_radio_genre Settings::getRadioGenres(ADDON::AddonPtr addon, int radio) {

  	CStdString radioC;
  	radioC.Format("radio%igenre", radio);

    struct {
      bool enable;
      int id;
    } radiogenres[] = {
    		{ addon->GetSetting(radioC + "1") == "true", 0x1 },
    		{ addon->GetSetting(radioC + "2") == "true", 0x2 },
    		{ addon->GetSetting(radioC + "3") == "true", 0x4 },
    		{ addon->GetSetting(radioC + "4") == "true", 0x8 },
    		{ addon->GetSetting(radioC + "") == "true", 0x10 },
    		{ addon->GetSetting(radioC + "6") == "true", 0x20 },
    		{ addon->GetSetting(radioC + "7") == "true", 0x40 },
    		{ addon->GetSetting(radioC + "8") == "true", 0x80 },
    		{ addon->GetSetting(radioC + "9") == "true", 0x100 },
    		{ addon->GetSetting(radioC + "10") == "true", 0x200 },
    		{ addon->GetSetting(radioC + "11") == "true", 0x400 },
    		{ addon->GetSetting(radioC + "12") == "true", 0x800 },
    		{ addon->GetSetting(radioC + "13") == "true", 0x1000 },
    		{ addon->GetSetting(radioC + "14") == "true", 0x2000 },
    		{ addon->GetSetting(radioC + "15") == "true", 0x4000 },
    		{ addon->GetSetting(radioC + "16") == "true", 0x8000 },
    		{ addon->GetSetting(radioC + "17") == "true", 0x10000 },
    		{ addon->GetSetting(radioC + "18") == "true", 0x20000 },
			  { addon->GetSetting(radioC + "19") == "true", 0x40000 },
    		{ addon->GetSetting(radioC + "20") == "true", 0x80000 },
    		{ addon->GetSetting(radioC + "21") == "true", 0x100000 },
    		{ addon->GetSetting(radioC + "22") == "true", 0x200000 },
    		{ addon->GetSetting(radioC + "23") == "true", 0x400000 },
    		{ addon->GetSetting(radioC + "24") == "true", 0x800000 },
    		{ addon->GetSetting(radioC + "25") == "true", 0x1000000 },
    		{ addon->GetSetting(radioC + "26") == "true", 0x2000000 },
    		{ addon->GetSetting(radioC + "27") == "true", 0x4000000 },
    };

    int mask = 0;
    for (int i = 0; i < sizeof(radiogenres) / sizeof(radiogenres[0]); i++)
      if (radiogenres[i].enable) mask |= radiogenres[i].id;

    return (sp_radio_genre) mask;
  }
} /* namespace addon_music_spotify */
