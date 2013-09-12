/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

#include "FileItem.h"
#include "GameClient.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "threads/Thread.h"

namespace GAMES
{
  /**
    * Class providing auto-launch functionality for the game browser. When the
    * user launches a game with no compatible clients, the game browser will
    * enqueue the file with CGameManager. RetroPlayer will give the user the
    * option of visiting the add-on manager, where the user can manually install
    * game clients. When a compatible game client is installed, the user will
    * then be prompted to launch the queued game file. The purpose of this is
    * to enable seamless launching of newly-installed game clients.
    */
  class CGameFileAutoLauncher : protected CThread
  {
  public:
    CGameFileAutoLauncher();
    ~CGameFileAutoLauncher();

    /**
     * Queue a file to be launched when a compatible emulator is installed.
     */
    void SetAutoLaunch(const CFileItem &file);
    void ClearAutoLaunch();

    /**
     * Launch the queued file (if any) with the specified game client, if possible.
     */
    void Launch(const GameClientPtr &gameClient);

  protected:
    /**
      * Monitor state of app and dequeue file when appropriate (user leaves
      * add-on manager, too much time passes, etc).
      */
    virtual void Process();

  private:
    /**
     * Return true if app is in a launch-ready state (add-on browser still
     * visible, timeout hasn't elapsed).
     */
    bool IsStateValid();

  private:
    CFileItemPtr         m_queuedFile;
    CCriticalSection     m_critSection;
    XbmcThreads::EndTime m_timeout;
  };
} // namespace GAMES
