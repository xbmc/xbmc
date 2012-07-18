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

#ifndef SXARTIST_H_
#define SXARTIST_H_

#ifndef SP_CALLCONV
#ifdef _WIN32
#define SP_CALLCONV __stdcall
#else
#define SP_CALLCONV
#endif
#endif

#include <libspotify/api.h>
#include <vector>
#include <string>
#include "../album/AlbumContainer.h"
#include "../track/TrackContainer.h"
#include "ArtistContainer.h"
#include "../thumb/SxThumb.h"

using namespace std;

namespace addon_music_spotify {
	class SxTrack;
	class SxAlbum;
	class SxArtist;
	class ArtistStore;
	class SxArtist: private TrackContainer,
			private AlbumContainer,
			private ArtistContainer {
	public:

		static void SP_CALLCONV cb_artistBrowseComplete(sp_artistbrowse *result,
				void *userdata);
		void doLoadDetails();
		void doLoadTracksAndAlbums();

		void detailsLoaded(sp_artistbrowse *result);

		friend class ArtistStore;

		void addRef() {
			m_references++;
		}

		int getReferencesCount() {
			return m_references;
		}

		bool isLoaded() {
			return !m_isLoadingDetails
					&& (!m_hasThumb || (m_hasThumb && m_thumb->isLoaded()));
		}

		bool isAlbumsLoaded();
		bool isTracksLoaded();
		bool isArtistsLoaded();

		const char* getArtistName() {
			return sp_artist_name(m_spArtist);
		}

		SxThumb* getThumb() {
			return m_thumb;
		}

		sp_artist* getSpArtist() {
			return m_spArtist;
		}

		vector<SxTrack*> getTracks() {
			return m_tracks;
		}

		bool getTrackItems(CFileItemList& items);

		vector<SxAlbum*> getAlbums() {
			return m_albums;
		}

		bool getAlbumItems(CFileItemList& items);

		vector<SxArtist*> getArtists() {
			return m_artists;
		}

		bool getArtistItems(CFileItemList& items);

		bool hasDetails() {
			return m_hasDetails;
		}

		bool hasTracksAndAlbums() {
			return m_hasTracksAndAlbums;
		}

		bool hasThumb() {
			return m_hasThumb;
		}

		string getBio() {
			return m_bio;
		}

		const char *getUri() {
			return m_uri;
		}

		CStdString *getFanart() {
			return m_fanart;
		}


	private:
		SxArtist(sp_artist *artist, bool loadTracksAndAlbums);
		virtual ~SxArtist();

		void rmRef() {
			m_references--;
		}

		bool m_isLoadingDetails;
		bool m_loadTrackAndAlbums;
		bool m_hasDetails;
		bool m_hasTracksAndAlbums;
		int m_references;

		string m_bio;
		sp_artistbrowse* m_browse;

		char *m_uri;
		sp_artist *m_spArtist;
		SxThumb *m_thumb;

		CStdString *m_fanart;

		bool m_hasThumb;
	};

} /* namespace addon_music_spotify */
#endif /* SXARTIST_H_ */
