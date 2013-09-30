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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "addons/AddonDatabase.h"
#include "addons/AddonManager.h"
#include "GameClient.h"
#include "GameFileAutoLauncher.h"
#include "FileItem.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

#include <map>
#include <set>
#include <string>

namespace GAMES
{
  /**
   * The main function of CGameManager is resolving file items into CGameClients.
   *
   * A manager is needed for resolving game clients as they are selected by the
   * file extensions they support. This is determined by loading the DLL and
   * querying it directly, so it is desirable to only do this once and cache the
   * information.
   */
  class CGameManager :
    public ADDON::IAddonMgrCallback, public IAddonDatabaseCallback, public Observer
  {
  protected:
    CGameManager() { }

  public:
    static CGameManager &Get();
    virtual ~CGameManager() { Stop(); }

    virtual void Start();
    virtual void Stop();

    //** Functions to notify CGameManager about stuff it manages **//

    /**
     * Create and maintain a cache of game client add-on information. If a file
     * has been placed in the queue via SetAutolaunch(), it will be launched if a
     * compatible emulator is registered.
     */
    void RegisterAddons(const ADDON::VECADDONS &addons);
    bool RegisterAddon(const GAMES::GameClientPtr &client);
    void UnregisterAddonByID(const std::string &strId);

    /**
     * Register the supported extensions of game clients in the add-on database
     * (including remote ones) for use by IsGame().
     */
    void RegisterExtensions();

    /**
     * Notify the game manager of add-ons in the database (including remote
     * add-ons). Called on Start() and when the Repository Updated action is
     * complete (see Repository.cpp). We must call this after the Repository
     * Updated action so that the game manager's knowledge of file extensions
     * (which it uses to determine whether files are games) contains the
     * extensions supported by the game clients of new/updated repositories.
     *
     * TODO: either rename this function to UpdateExtensions(), which is more
     * transparent to what it actually does, or abstract it further by creating
     * a callback interface for the Repository Updated action. For now,
     * compromise and always pretend that they are remote add-ons.
     */
    void UpdateRemoteAddons(const ADDON::VECADDONS &addons);

    // Inherited from IAddonDatabaseCallback
    virtual void AddonEnabled(ADDON::AddonPtr addon, bool bDisabled);
    virtual void AddonDisabled(ADDON::AddonPtr addon);

    // Inherited from Observer
    virtual void Notify(const Observable &obs, const ObservableMessage msg);

    /**
     * Queue a file to be launched when the next game client is installed.
     */
    void SetAutoLaunch(const CFileItem &file) { m_fileLauncher.SetAutoLaunch(file); }
    void ClearAutoLaunch() { m_fileLauncher.ClearAutoLaunch(); }

    //** Functions to get info for stuff that CGameManager manages **//

    virtual bool GetClient(const std::string &strClientId, GameClientPtr &addon) const;
    virtual bool GetConnectedClient(const std::string &strClientId, GameClientPtr &addon) const;
    virtual bool IsConnectedClient(const std::string &strClientId) const;
    virtual bool IsConnectedClient(const ADDON::AddonPtr addon) const;

    /**
     * Resolve a file item to a list of game client IDs.
     *
     *   # If the file forces a particular game client via file.SetProperty("gameclient", id),
     *     the result will contain no more than one possible candidate.
     *   # If the file's game info tag provides a "platform", the available game
     *     clients will be filtered by this platform (given the <platform> tag
     *     in their addon.xml).
     *   # If file is a zip file, the contents of that zip will be used to find
     *     suitable candidates (which may yield multiple if there are several
     *     different kinds of ROMs inside).
     */
    void GetGameClientIDs(const CFileItem& file, std::vector<std::string> &candidates) const;

    /**
     * Get a list of valid game client extensions (as determined by the tag in
     * addon.xml). Includes game clients in remote repositories.
     */
    void GetExtensions(std::vector<std::string> &exts) const;

    /**
     * Returns true if the file extension is supported by an add-on in an enabled
     * repository.
     */
    bool IsGame(const std::string &path) const;

    //** Functions that operate on the clients **//

    virtual bool StopClient(ADDON::AddonPtr client, bool bRestart);

    // Inherited from IAddonMgrCallback
    virtual bool RequestRestart(ADDON::AddonPtr addon, bool datachanged) { return StopClient(addon, true); }
    virtual bool RequestRemoval(ADDON::AddonPtr addon)                   { return StopClient(addon, false); }

  private:
    // Initialize m_gameClients with enabled game clients
    virtual bool UpdateAddons();

    std::map<std::string, GameClientPtr> m_gameClients;
    std::set<std::string>                m_gameExtensions;
    CGameFileAutoLauncher                m_fileLauncher;
    CCriticalSection                     m_critSection;
  };
} // namespace GAMES
