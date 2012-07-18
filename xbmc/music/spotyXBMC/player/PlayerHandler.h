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

#ifndef PLAYERHANDLER_H_
#define PLAYERHANDLER_H_

#include <string>
#include <libspotify/api.h>

using namespace std;

namespace addon_music_spotify {

  class Codec;
  class PlayerHandler {
  public:

    static PlayerHandler *getInstance();
    static void deInit();

    void removeCodec();

    Codec* getCodec();
    Codec* getCurrentCodec() {
      return m_currentCodec;
    }

    static int SP_CALLCONV cb_musicDelivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames);
    static void SP_CALLCONV cb_endOfTrack(sp_session *session);

  private:
    PlayerHandler();
    virtual ~PlayerHandler();
    static PlayerHandler *m_instance;

    Codec* m_currentCodec;
    Codec* m_preloadingCodec;

  };

} /* namespace addon_music_spotify */
#endif /* PLAYERHANDLER_H_ */
