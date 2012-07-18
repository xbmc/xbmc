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

#include "TopLists.h"
#include "../session/Session.h"
#include "../SxSettings.h"
#include <sys/timeb.h>

namespace addon_music_spotify {

TopLists::TopLists() {
	m_albumsNextReload.SetExpired();
	m_artistsNextReload.SetExpired();
	m_tracksNextReload.SetExpired();
	m_waitingForAlbums = false;
	m_waitingForArtists = false;
	m_waitingForTracks = false;
	m_albumsLoaded = false;
	m_artistsLoaded = false;
	m_tracksLoaded = false;
	Logger::printOut("creating toplists");
	if (Settings::getInstance()->getPreloadTopLists()) {
		reLoadArtists();
		reLoadAlbums();
		reLoadTracks();
	}
}

TopLists::~TopLists() {
	unloadLists();
}

void TopLists::unloadLists() {
	m_albumsLoaded = false;
	m_artistsLoaded = false;
	m_tracksLoaded = false;

	removeAllTracks();
	removeAllAlbums();
	removeAllArtists();
}

void TopLists::reLoadArtists() {
	if (m_waitingForArtists)
		return;
	m_waitingForArtists = true;
	sp_toplistregion region =
			Settings::getInstance()->toplistRegionEverywhere() ?
					SP_TOPLIST_REGION_EVERYWHERE :
					(sp_toplistregion) sp_session_user_country(
							Session::getInstance()->getSpSession());
	sp_toplistbrowse_create(Session::getInstance()->getSpSession(),
			SP_TOPLIST_TYPE_ARTISTS, region, Settings::getInstance()->getUserName(),
			&cb_toplistArtistsComplete, this);
}

void TopLists::reLoadAlbums() {
	if (m_waitingForAlbums)
		return;
	m_waitingForAlbums = true;
	sp_toplistregion region =
			Settings::getInstance()->toplistRegionEverywhere() ?
					SP_TOPLIST_REGION_EVERYWHERE :
					(sp_toplistregion) sp_session_user_country(
							Session::getInstance()->getSpSession());
	sp_toplistbrowse_create(Session::getInstance()->getSpSession(),
			SP_TOPLIST_TYPE_ALBUMS, region, Settings::getInstance()->getUserName(),
			&cb_toplistAlbumsComplete, this);
}

void TopLists::reLoadTracks() {
	if (m_waitingForTracks)
		return;
	m_waitingForTracks = true;
	sp_toplistregion region =
			Settings::getInstance()->toplistRegionEverywhere() ?
					SP_TOPLIST_REGION_EVERYWHERE :
					(sp_toplistregion) sp_session_user_country(
							Session::getInstance()->getSpSession());
	sp_toplistbrowse_create(Session::getInstance()->getSpSession(),
			SP_TOPLIST_TYPE_TRACKS, region, Settings::getInstance()->getUserName(),
			&cb_toplistTracksComplete, this);
}

bool TopLists::isArtistsLoaded() {
	if (!m_artistsLoaded || m_artistsNextReload.IsTimePast())
		return false;
	return artistsLoaded();
}

bool TopLists::isAlbumsLoaded() {
	if (!m_albumsLoaded || m_albumsNextReload.IsTimePast())
		return false;
	return albumsLoaded();
}

bool TopLists::isTracksLoaded() {
	if (!m_tracksLoaded || m_tracksNextReload.IsTimePast())
		return false;
	return tracksLoaded();
}

bool TopLists::getArtistItems(CFileItemList& items) {
	return true;
}

bool TopLists::getAlbumItems(CFileItemList& items) {
	return true;
}

bool TopLists::getTrackItems(CFileItemList& items) {
	return true;
}

void TopLists::cb_toplistArtistsComplete(sp_toplistbrowse *result,
		void *userdata) {
	Logger::printOut("toplist artists callback");
	TopLists *lists = (TopLists*) (userdata);

	vector<SxArtist*> newArtists;

	for (int index = 0; index < sp_toplistbrowse_num_artists(result); index++) {
		//dont load the albums and tracks for all artists here, it takes forever
		SxArtist* artist = ArtistStore::getInstance()->getArtist(
				sp_toplistbrowse_artist(result, index), false);
		if (artist != NULL)
			newArtists.push_back(artist);
	}

	lists->removeAllArtists();
	lists->m_artists = newArtists;

	struct timeb tmb;
	ftime(&tmb);
	lists->m_artistsNextReload.Set(1000 * 60 * 60 * 24);
	lists->m_waitingForArtists = false;
	lists->m_artistsLoaded = true;
	Logger::printOut("Toplist artists loaded");
}

void TopLists::cb_toplistAlbumsComplete(sp_toplistbrowse *result,
		void *userdata) {
	Logger::printOut("toplist albums callback");
	TopLists *lists = (TopLists*) (userdata);

	vector<SxAlbum*> newAlbums;

	for (int index = 0; index < sp_toplistbrowse_num_albums(result); index++) {
		sp_album* tempalbum = sp_toplistbrowse_album(result, index);
		if (tempalbum == NULL)
			continue;
		while (!sp_album_is_loaded(tempalbum))
			;

		if (sp_album_is_available(tempalbum)) {
			SxAlbum* album = AlbumStore::getInstance()->getAlbum(tempalbum, true);
			if (album != NULL)
				newAlbums.push_back(album);
		}
	}

	lists->removeAllAlbums();
	lists->m_albums = newAlbums;

	struct timeb tmb;
	ftime(&tmb);
	lists->m_albumsNextReload.Set(1000 * 60 * 60 * 24);
	lists->m_waitingForAlbums = false;
	lists->m_albumsLoaded = true;
	Logger::printOut("Toplist albums loaded");
}

void TopLists::cb_toplistTracksComplete(sp_toplistbrowse *result,
		void *userdata) {
	Logger::printOut("toplist tracks callback");
	TopLists *lists = (TopLists*) (userdata);
	//add the tracks

	vector<SxTrack*> newTracks;

	for (int index = 0; index < sp_toplistbrowse_num_tracks(result); index++) {
		if (sp_track_get_availability(Session::getInstance()->getSpSession(),
				sp_toplistbrowse_track(result, index))) {
			SxTrack* track = TrackStore::getInstance()->getTrack(
					sp_toplistbrowse_track(result, index));
			if (track != NULL)
				newTracks.push_back(track);
		}
	}

	lists->removeAllTracks();
	lists->m_tracks = newTracks;

	struct timeb tmb;
	ftime(&tmb);
	lists->m_tracksNextReload.Set(1000 * 60 * 60 * 24);
	lists->m_waitingForTracks = false;
	lists->m_tracksLoaded = true;
	Logger::printOut("Toplist tracks loaded");
}

} /* namespace addon_music_spotify */
