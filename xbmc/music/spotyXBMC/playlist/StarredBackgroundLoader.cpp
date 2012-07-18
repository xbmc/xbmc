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

#include "../album/AlbumStore.h"
#include "../session/Session.h"
#include "StarredBackgroundLoader.h"
#include "StarredList.h"
#include "../album/SxAlbum.h"
#include "../track/SxTrack.h"
#include "../artist/SxArtist.h"
#include "../track/TrackStore.h"
#include "../artist/ArtistStore.h"
#include "../Utils.h"
#include <set>

namespace addon_music_spotify {

StarredBackgroundLoader::StarredBackgroundLoader() : CThread("Spotify StarredBackgroundLoader"){
}

StarredBackgroundLoader::~StarredBackgroundLoader() {
}

void StarredBackgroundLoader::OnStartup() {
}

void StarredBackgroundLoader::OnExit() {
}

void StarredBackgroundLoader::OnException() {
}

void StarredBackgroundLoader::Process() {
	StarredList* ls =
			Session::getInstance()->getPlaylistStore()->getStarredList();
	ls->m_isBackgroundLoading = true;

	while (ls->m_reload) {
		Logger::printOut("Populate starred albums and artists thread start");
		//we need the session lock so that we can run processevents and wait for albums etc
		while (!Session::getInstance()->lock()) {
			SleepMs(1);
		}
		ls->m_reload = false;

		vector<SxTrack*> newTracks;
		for (int index = 0; index < sp_playlist_num_tracks(ls->m_spPlaylist);
				index++) {
			if (!sp_track_get_availability(Session::getInstance()->getSpSession(),
					sp_playlist_track(ls->m_spPlaylist, index)))
				continue;
			SxTrack* track = TrackStore::getInstance()->getTrack(
					sp_playlist_track(ls->m_spPlaylist, index));
			if (track) {
				newTracks.push_back(track);
			}
		}

		ls->removeAllTracks();

		ls->m_tracks = newTracks;

		set<sp_album*> tempAlbums;
		set<sp_artist*> tempArtists;
		for (int i = 0; i < ls->getNumberOfTracks(); i++) {

			sp_album* tempAlbum = sp_track_album(ls->m_tracks[i]->getSpTrack());
			if (tempAlbum != NULL)
				tempAlbums.insert(tempAlbum);

			sp_artist* tempArtist = sp_track_artist(ls->m_tracks[i]->getSpTrack(), 0);
			if (tempArtist != NULL)
				tempArtists.insert(tempArtist);
		}

		//now we have a set with unique sp_albums, lets load them and see if they have all tracks starred or not

		vector<SxAlbum*> newAlbums;
		//find out witch albums that has complete set of starred tracks
		for (set<sp_album*>::const_iterator albumIt = tempAlbums.begin();
				albumIt != tempAlbums.end(); ++albumIt) {
			sp_album* tempalbum = (sp_album*) *albumIt;
			if (tempalbum == NULL)
				continue;
			while (!sp_album_is_loaded(tempalbum))
				SleepMs(1);

			SxAlbum* album = AlbumStore::getInstance()->getAlbum(tempalbum, true);
			Logger::printOut(album->getAlbumName());

			Session::getInstance()->unlock();
			while (!album->isLoaded()) {
				SleepMs(1);
			}
			while (!Session::getInstance()->lock()) {
				SleepMs(1);
			}

			if (!album->hasTracksAndDetails() || album->getTracks().size() == 0
					|| !sp_album_is_available(album->getSpAlbum())) {
				AlbumStore::getInstance()->removeAlbum(album);
				continue;
			}
			bool albumIsStarred = true;
			vector<SxTrack*> tracks = album->getTracks();
			for (int i = 0; i < tracks.size(); i++) {
				if (!sp_track_is_starred(Session::getInstance()->getSpSession(),
						tracks[i]->getSpTrack())) {
					albumIsStarred = false;
					break;
				}
			}

			if (albumIsStarred)
				newAlbums.push_back(
						AlbumStore::getInstance()->getAlbum(album->getSpAlbum(), true));
		}

		ls->removeAllAlbums();

		//add all artists that we have collected

		vector<SxArtist*> newArtists;
		for (set<sp_artist*>::const_iterator artistIt = tempArtists.begin();
				artistIt != tempArtists.end(); ++artistIt) {
			sp_artist* tempartist = (sp_artist*) *artistIt;
			if (tempartist == NULL)
				continue;
			//while (!sp_artist_is_loaded(tempartist))
			//	SleepMs(1);

			SxArtist* artist = ArtistStore::getInstance()->getArtist(tempartist,
					false);

			if (artist != NULL)
				newArtists.push_back(artist);
		}

		while (!ls->m_artists.empty()) {
			ArtistStore::getInstance()->removeArtist(ls->m_artists.back());
			ls->m_artists.pop_back();
		}

		ls->m_artists = newArtists;
		ls->m_albums = newAlbums;

		//wait for all info to finish loading before updating the lists (or reloading it all)
		Session::getInstance()->unlock();
		while (!ls->isLoaded()) {
			SleepMs(1);
		}
	}

	ls->m_isBackgroundLoading = false;

	Utils::updateAllTracks();
	Utils::updateAllAlbums();
	Utils::updateAllArtists();
	Logger::printOut("Populate starred albums and artists thread done");
}
} /* namespace addon_music_spotify */
