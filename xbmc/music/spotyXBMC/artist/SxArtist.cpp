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

#include <stdio.h>
#include <math.h>
#include "SxArtist.h"
#include "../session/Session.h"
#include "../Logger.h"
#include "../Utils.h"

#include "../album/SxAlbum.h"
#include "../album/AlbumStore.h"
#include "../track/SxTrack.h"
#include "../track/TrackStore.h"
#include "../artist/SxArtist.h"
#include "../artist/ArtistStore.h"
#include "../thumb/SxThumb.h"
#include "../thumb/ThumbStore.h"

namespace addon_music_spotify {

	SxArtist::SxArtist(sp_artist *artist, bool loadTracksAndAlbums) {
		m_spArtist = artist;
		Logger::printOut("creating artist");
		m_references = 1;
		m_browse = NULL;
		m_isLoadingDetails = false;
		m_hasDetails = false;
		m_hasTracksAndAlbums = false;
		m_thumb = NULL;
		m_hasThumb = false;
		m_bio = "";
		sp_link *link = sp_link_create_from_artist(artist);
		m_uri = new char[256];
		sp_link_as_string(link, m_uri, 256);
		sp_link_release(link);

			//check if there is a thumb
			const byte* image = sp_artist_portrait(artist);
			if (image) {
				m_thumb = ThumbStore::getInstance()->getThumb(image);
				if (m_thumb)
					m_hasThumb = true;
			}

		m_loadTrackAndAlbums = loadTracksAndAlbums;
		if (loadTracksAndAlbums)
			doLoadTracksAndAlbums();
		//sometimes the thumb is not loaded correct here, do a small artist browse and try again!
		else if (Settings::getInstance()->getPreloadArtistDetails() || !m_hasThumb)
			doLoadDetails();
		else
			m_hasDetails = true;
		Logger::printOut("creating artist done");

		m_fanart = ThumbStore::getInstance()->getFanart(sp_artist_name(m_spArtist));
	}

	SxArtist::~SxArtist() {
		while (m_isLoadingDetails) {
			//Session::getInstance()->processEvents();
			Logger::printOut("waiting for artist to die");
		}

		removeAllTracks();
		removeAllAlbums();
		removeAllArtists();

		if (m_thumb)
			ThumbStore::getInstance()->removeThumb(m_thumb);
		delete m_uri;
		if (hasDetails() && m_browse != NULL)
			sp_artistbrowse_release(m_browse);
		sp_artist_release(m_spArtist);
	}

	bool SxArtist::isAlbumsLoaded() {
		if (!m_hasTracksAndAlbums)
			return false;
		return albumsLoaded();
	}

	bool SxArtist::isTracksLoaded() {
		if (!m_hasTracksAndAlbums)
			return false;
		return tracksLoaded();
	}

	bool SxArtist::isArtistsLoaded() {
		if (!m_hasTracksAndAlbums)
			return false;
		return artistsLoaded();
	}

	void SxArtist::doLoadTracksAndAlbums() {
		Logger::printOut("loading artist tracks and albums");
		m_loadTrackAndAlbums = true;
		//if we allready have it all..
		if (m_hasTracksAndAlbums)
			return;

		m_isLoadingDetails = true;
		m_browse = sp_artistbrowse_create(Session::getInstance()->getSpSession(),
				m_spArtist, SP_ARTISTBROWSE_FULL, &cb_artistBrowseComplete, this);
	}

	void SxArtist::doLoadDetails() {
		if (m_hasDetails || m_isLoadingDetails)
			return;

		m_isLoadingDetails = true;
		m_browse = sp_artistbrowse_create(Session::getInstance()->getSpSession(),
				m_spArtist, SP_ARTISTBROWSE_NO_ALBUMS, &cb_artistBrowseComplete, this);
	}

	void SxArtist::detailsLoaded(sp_artistbrowse *result) {
		if (sp_artistbrowse_error(result) == SP_ERROR_OK) {
			//check if there is a thumb
			if (!m_hasThumb && sp_artistbrowse_num_portraits > 0) {
				const byte* image = sp_artistbrowse_portrait(result, 0);
				if (image) {
					m_thumb = ThumbStore::getInstance()->getThumb(image);
				}
			}
			if (m_thumb != NULL)
				m_hasThumb = true;

			m_bio = sp_artistbrowse_biography(result);
			//remove the links from the bio text (it contains spotify uris so maybe we can do something fun with it later)
			Utils::cleanTags(m_bio);

			if (m_loadTrackAndAlbums) {
				//add the albums
				int maxAlbums =
						Settings::getInstance()->getArtistNumberAlbums() == -1 ?
								sp_artistbrowse_num_albums(m_browse) :
								Settings::getInstance()->getArtistNumberAlbums();

				int addedAlbums = 0;
				for (int index = 0;
						index < sp_artistbrowse_num_albums(m_browse)
								&& addedAlbums < maxAlbums; index++) {
					if (sp_album_is_available(sp_artistbrowse_album(m_browse, index))) {
						m_albums.push_back(
								AlbumStore::getInstance()->getAlbum(
										sp_artistbrowse_album(m_browse, index), true));
						addedAlbums++;
					}
				}

				//add the tracks
				int maxTracks =
						Settings::getInstance()->getArtistNumberTracks() == -1 ?
								sp_artistbrowse_num_tracks(m_browse) :
								Settings::getInstance()->getArtistNumberTracks();

				int addedTracks = 0;
				for (int index = 0;
						index < sp_artistbrowse_num_tracks(m_browse)
								&& addedTracks < maxTracks; index++) {
					if (sp_track_get_availability(Session::getInstance()->getSpSession(),
							sp_artistbrowse_track(m_browse, index))) {
						m_tracks.push_back(
								TrackStore::getInstance()->getTrack(
										sp_artistbrowse_track(m_browse, index)));
						addedTracks++;
					}
				}

				//add the artists
				int maxArtists =
						Settings::getInstance()->getArtistNumberArtists() == -1 ?
								sp_artistbrowse_num_similar_artists(m_browse) :
								Settings::getInstance()->getArtistNumberArtists();

				int addedArtists = 0;
				for (int index = 0;
						index < sp_artistbrowse_num_similar_artists(m_browse)
								&& addedArtists < maxArtists; index++) {
					m_artists.push_back(
							ArtistStore::getInstance()->getArtist(
									sp_artistbrowse_similar_artist(m_browse, index), false));
					addedArtists++;
				}

				m_hasTracksAndAlbums = true;
			}
		}
		m_isLoadingDetails = false;
		m_hasDetails = true;
		Logger::printOut("artist browse complete done");
	}

	void SxArtist::cb_artistBrowseComplete(sp_artistbrowse *result,
			void *userdata) {
		Logger::printOut("artist browse complete");
		SxArtist *artist = (SxArtist*) (userdata);
		Logger::printOut(artist->getArtistName());
		artist->detailsLoaded(result);
	}

	bool SxArtist::getTrackItems(CFileItemList& items) {
		return true;
	}

	bool SxArtist::getAlbumItems(CFileItemList& items) {
		return true;
	}

	bool SxArtist::getArtistItems(CFileItemList& items) {
		return true;
	}

} /* namespace addon_music_spotify */

