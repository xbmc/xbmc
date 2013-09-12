/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "games/GameClient.h"
#include "utils/StdString.h"
#include <map>

namespace GAMES
{
  class ILibretroCallbacksDLL;

  class CLibretroEnvironment
  {
  public:
    // XBMC's interface for libretro 
    static bool EnvironmentCallback(unsigned cmd, void *data);

    /**
     * Set callbacks and CGameClient object for loading a libretro DLL. The
     * CGameClient object is used when loading and running the game to
     * communicate with XBMC (such as querying or updating settings).
     */
    static void SetDLLCallbacks(ILibretroCallbacksDLL *callbacks, GameClientPtr activeClient);

    /**
     * Called after the active game cient is unloaded.
     */
    static void ResetCallbacks();

    /**
     * Abort() returns true if the game client currently loading should be aborted.
     */
    static bool Abort() { return m_bAbort; }

    /**
     * Process a libretro variable, then then process it a little more.
     */
    static bool ParseVariable(const LIBRETRO::retro_variable &var, CStdString &strDefault);

  private:
    static ILibretroCallbacksDLL            *m_callbacksDLL;
    static GameClientPtr                    m_activeClient;
    static std::map<CStdString, CStdString> m_varMap;
    static CStdString                       m_systemDirectory;
    static bool                             m_bAbort;
  };
} // namespace GAMES
