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

#ifndef SESSIONCALLBACKS_H_
#define SESSIONCALLBACKS_H_

#ifndef SP_CALLCONV
#ifdef _WIN32
#define SP_CALLCONV __stdcall
#else
#define SP_CALLCONV
#endif
#endif

#include <libspotify/api.h>
#include "../SxSettings.h"
#include "../Logger.h"

namespace addon_music_spotify {

  class SxAlbum;

  class SessionCallbacks {
  public:
    SessionCallbacks();
    virtual ~SessionCallbacks();

    static void SP_CALLCONV cb_connectionError(sp_session *session, sp_error error);
    static void SP_CALLCONV cb_loggedIn(sp_session *session, sp_error error);
    static void SP_CALLCONV cb_loggedOut(sp_session *session);
    static void SP_CALLCONV cb_notifyMainThread(sp_session *session);
    static void SP_CALLCONV cb_logMessage(sp_session *session, const char *data);
    static void SP_CALLCONV cb_searchComplete(sp_search *search, void *userdata);
    static void SP_CALLCONV cb_topListAritstsComplete(sp_toplistbrowse *result, void *userdata);
    static void SP_CALLCONV cb_topListAlbumsComplete(sp_toplistbrowse *result, void *userdata);
    static void SP_CALLCONV cb_topListTracksComplete(sp_toplistbrowse *result, void *userdata);
    static void SP_CALLCONV cb_artistBrowseComplete(sp_artistbrowse *result, void *userdata);

    sp_session_callbacks getCallbacks() {
      return m_callbacks;
    }

  private:
    sp_session_callbacks m_callbacks;
  };

} /* namespace addon_music_spotify */
#endif /* SESSIONCALLBACKS_H_ */
