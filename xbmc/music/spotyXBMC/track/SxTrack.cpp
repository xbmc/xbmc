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
#include "SxTrack.h"
#include "../Logger.h"
#include "../session/Session.h"
#include "../album/AlbumStore.h"
#include "../album/SxAlbum.h"
#include "../thumb/ThumbStore.h"

namespace addon_music_spotify {

  SxTrack::SxTrack(sp_track *spTrack) {
//  Logger::printOut("creating track");
//  Logger::printOut(sp_track_name(spSxTrack));
    while (!sp_track_is_loaded(spTrack))
      ;

    //Logger::printOut("creating track loaded");

    m_references = 1;
    m_spTrack = spTrack;
    m_name = sp_track_name(spTrack);

    m_rating = ceil((float)sp_track_popularity(spTrack) / 10);

    m_duration = 0.001 * sp_track_duration(spTrack);
    m_trackNumber = sp_track_index(spTrack);

    m_albumName = "";
    m_albumArtistName = "";
    m_year = 0;
    m_thumb = NULL;
    m_hasTHumb = false;

    //load the album and release it when we have harvested all data we need
    sp_album * album = sp_track_album(spTrack);
    if (sp_album_is_loaded(album)) {
      SxAlbum* sAlbum = AlbumStore::getInstance()->getAlbum(sp_track_album(spTrack), false);
      m_thumb = sAlbum->getThumb();
      m_albumName = sAlbum->getAlbumName();
      m_albumArtistName = sAlbum->getAlbumArtistName();
      m_year = sAlbum->getAlbumYear();
      //release it again
      AlbumStore::getInstance()->removeAlbum(sAlbum);

      if (m_thumb != NULL) {
        m_thumb->addRef();
        m_hasTHumb = true;
      }

    } else
      Logger::printOut("no album loaded for track");

    m_artistName = sp_artist_name(sp_track_artist(spTrack, 0));
    m_fanart = ThumbStore::getInstance()->getFanart(sp_artist_name(sp_track_artist(spTrack, 0)));

    sp_link *link = sp_link_create_from_track(spTrack, 0);
    m_uri = new char[256];
    sp_link_as_string(link, m_uri, 256);
    sp_link_release(link);

  }

  SxTrack::~SxTrack() {
    if (m_thumb) ThumbStore::getInstance()->removeThumb(m_thumb);
    delete m_uri;
    sp_track_release(m_spTrack);
  }

  bool SxTrack::isLoaded() {
    //TODO the local tracks are never reported as local....why?
    //if (m_album && !sp_track_is_local(Session::getInstance()->getSpSession(),m_spSxTrack))
    //  return m_album->getAlbumSxThumb()->isLoaded();
    if (m_thumb) return m_thumb->isLoaded();
    return true;
  }

} /* namespace addon_music_spotify */

