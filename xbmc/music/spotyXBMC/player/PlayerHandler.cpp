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

#include "PlayerHandler.h"
#include "Codec.h"
#include "../session/Session.h"

namespace addon_music_spotify {

  PlayerHandler::PlayerHandler() {
    m_currentCodec = NULL;
    m_preloadingCodec = NULL;
  }

  PlayerHandler* PlayerHandler::m_instance = 0;
  PlayerHandler *PlayerHandler::getInstance() {
    return m_instance ? m_instance : (m_instance = new PlayerHandler);
  }

  void PlayerHandler::deInit() {
    if (m_instance == NULL)
      return;
    if (m_instance->m_currentCodec != NULL)
      m_instance->m_currentCodec->unloadPlayer();
    //The codec is probably deleted by xbmc on exit
    //  delete m_currentCodec();
  }

  Codec *PlayerHandler::getCodec() {
    //do some checks if it is a spotify track and if its loaded and so on
    if (m_currentCodec == NULL) {
      //start with a init flag
      m_currentCodec = new Codec();
      return m_currentCodec;
    }
    return NULL;
    //}
    //if there is an other preloading track, remove it
    //if (m_preloadingCodec)
    //  delete m_preloadingCodec;
    //start with a preload flag
    //m_preloadingCodec = new Codec();
    //return m_preloadingCodec;
  }

  PlayerHandler::~PlayerHandler() {
  }

  void PlayerHandler::removeCodec() {
    m_currentCodec = NULL;
  }

  int PlayerHandler::cb_musicDelivery(sp_session *session,
      const sp_audioformat *format, const void *frames, int num_frames) {
    return m_instance->getCurrentCodec()->musicDelivery(format->channels,
        format->sample_rate, frames, num_frames);
  }

  void PlayerHandler::cb_endOfTrack(sp_session *session) {
    m_instance->getCurrentCodec()->endOfTrack();
  }
}
/* namespace addon_music_spotify */
