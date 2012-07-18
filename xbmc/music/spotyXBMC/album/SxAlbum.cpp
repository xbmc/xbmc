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
#include "SxAlbum.h"
#include "../Utils.h"
#include "../session/Session.h"
#include "../Logger.h"
#include "../track/SxTrack.h"
#include "../track/TrackStore.h"
#include "../thumb/ThumbStore.h"

namespace addon_music_spotify {

SxAlbum::SxAlbum(sp_album *album, bool loadTracksAndDetails) {
	m_spAlbum = album;
	// Logger::printOut("creating album");
	while (!sp_album_is_loaded(m_spAlbum))
		;
	m_references = 1;
	m_numberOfDiscs = 1;
	m_isLoadingTracks = false;
	m_hasTracksAndDetails = false;
	m_hasThumb = false;
	m_thumb = NULL;
	m_review = "none";
	m_year = sp_album_year(m_spAlbum);
	sp_link *link = sp_link_create_from_album(album);
	m_uri = new char[256];
	sp_link_as_string(link, m_uri, 256);
	sp_link_release(link);
	m_rating = 0;
	if (loadTracksAndDetails)
		doLoadTracksAndDetails();
	doLoadThumb();
	m_fanart = ThumbStore::getInstance()->getFanart(sp_artist_name(sp_album_artist(m_spAlbum)));
	// Logger::printOut("creating album slut");
}

SxAlbum::~SxAlbum() {
	removeAllTracks();

	if (m_thumb)
		ThumbStore::getInstance()->removeThumb(m_thumb);
	sp_album_release(m_spAlbum);
	delete m_uri;
}

bool SxAlbum::isStarred() {
	for (int i = 0; i < m_tracks.size(); i++) {
		if (!sp_track_is_starred(Session::getInstance()->getSpSession(),
				m_tracks[i]->getSpTrack()))
			return false;
	}
	return true;
}

bool SxAlbum::toggleStar() {
	Logger::printOut("toggle album star");
	sp_track **tracks = new sp_track*[m_tracks.size()]();

	for (int i = 0; i < m_tracks.size(); i++) {
		tracks[i] = m_tracks[i]->getSpTrack();
	}

	bool isStarred = this->isStarred();
	sp_track_set_starred(Session::getInstance()->getSpSession(), tracks,
			m_tracks.size(), !isStarred);

	delete tracks;
	return true;
}

void SxAlbum::doLoadTracksAndDetails() {
	if (m_hasTracksAndDetails || m_isLoadingTracks)
		return;

	sp_albumbrowse_create(Session::getInstance()->getSpSession(), m_spAlbum,
			&cb_albumBrowseComplete, this);
	m_isLoadingTracks = true;
}

void SxAlbum::doLoadThumb() {
	if (m_hasThumb)
		return;
	//Logger::printOut("Requesting thumb for album");
	const byte* image = sp_album_cover(m_spAlbum);
	if (image) {
		m_thumb = ThumbStore::getInstance()->getThumb(image);
		if (m_thumb)
			m_hasThumb = true;
	}
}

void SxAlbum::tracksLoaded(sp_albumbrowse *result) {
	if (sp_albumbrowse_error(result) == SP_ERROR_OK) {
		m_review = sp_albumbrowse_review(result);
		//remove the links from the review text (it contains spotify uris so maybe we can do something fun with it later)
		Utils::cleanTags(m_review);

		//get some ratings, the album dont have rating so iterate through the tracks and calculate a mean value for the album
		float rating = 0;

		for (int index = 0; index < sp_albumbrowse_num_tracks(result); index++) {
			sp_track *track = sp_albumbrowse_track(result, index);
			if (m_numberOfDiscs < sp_track_disc(track))
				m_numberOfDiscs = sp_track_disc(track);
			m_tracks.push_back(
					TrackStore::getInstance()->getTrack(
							sp_albumbrowse_track(result, index)));

			rating += sp_track_popularity(track);
		}

		if (sp_albumbrowse_num_tracks(result) != 0) {
			m_rating = ceil(rating / (sp_albumbrowse_num_tracks(result)) / 10);
		}

		m_hasTracksAndDetails = true;
	}
	m_isLoadingTracks = false;
	sp_albumbrowse_release(result);
	//Logger::printOut("album browse complete done");
}

void SxAlbum::cb_albumBrowseComplete(sp_albumbrowse *result, void *userdata) {
	//Logger::printOut("album browse complete");
	SxAlbum *album = (SxAlbum*) (userdata);
	//Logger::printOut(album->getAlbumName());
	album->tracksLoaded(result);
}

bool SxAlbum::getTrackItems(CFileItemList& items) {
	return true;
}

} /* namespace addon_music_spotify */

