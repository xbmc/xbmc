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

#include "ArtistStore.h"
#include "../Logger.h"
#include "SxArtist.h"

namespace addon_music_spotify {

ArtistStore::ArtistStore() {
}

void ArtistStore::deInit() {
	delete m_instance;
}

ArtistStore::~ArtistStore() {
	Logger::printOut("deleting artistStore");
	for (artistMap::iterator it = m_artists.begin(); it != m_artists.end();
			++it) {
		delete it->second;
	}
}

ArtistStore* ArtistStore::m_instance = 0;
ArtistStore *ArtistStore::getInstance() {
	return m_instance ? m_instance : (m_instance = new ArtistStore);
}

SxArtist* ArtistStore::getArtist(sp_artist *spArtist,
		bool loadAlbumsAndTracks) {
	Logger::printOut("loading spArtist");
	sp_artist_add_ref(spArtist);
	while (!sp_artist_is_loaded(spArtist))
		;

	artistMap::iterator it = m_artists.find(spArtist);
	SxArtist *artist;

	if (it == m_artists.end()) {
		//we need to create it
		artist = new SxArtist(spArtist, loadAlbumsAndTracks);
		m_artists.insert(artistMap::value_type(spArtist, artist));
	} else {
		//Logger::printOut("loading artist from store");
		artist = it->second;
		if (loadAlbumsAndTracks)
			artist->doLoadTracksAndAlbums();

		artist->addRef();
	}

	return artist;
}

void ArtistStore::removeArtist(sp_artist *spArtist) {
	artistMap::iterator it = m_artists.find(spArtist);
	SxArtist *artist;
	if (it != m_artists.end()) {
		artist = it->second;
		if (artist->getReferencesCount() <= 1) {
			m_artists.erase(spArtist);
			Logger::printOut("deleting artist");
			delete artist;
		} else {
			artist->rmRef();
			Logger::printOut("lower artist ref");
		}
	}
}

void ArtistStore::removeArtist(SxArtist* artist) {
	removeArtist(artist->getSpArtist());
}

} /* namespace addon_music_spotify */
