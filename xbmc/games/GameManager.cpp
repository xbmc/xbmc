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

#include "GameManager.h"
#include "addons/AddonDatabase.h"
#include "Application.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/Directory.h"
#include "GameFileLoader.h"
#include "profiles/ProfilesManager.h"
#include "threads/SingleLock.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

using namespace ADDON;
using namespace GAME_INFO;
using namespace GAMES;
using namespace XFILE;
using namespace std;


/* TEMPORARY */
// Remove this struct when libretro has an API call to query the number of
// controller ports a game supports. If this code is still here in six months,
// Garrett will be very unhappy. I found a buffer overflow in SNES9x when
// trying to set controller ports 3-8, so this API call needs to happen.
/*
struct PortMapping
{
  GamePlatform platform;
  int          ports;
};

static const PortMapping ports[] =
{
  { PLATFORM_GAMEBOY,              1 },
  { PLATFORM_GAMEBOY_COLOR,        1 },
  { PLATFORM_GAMEBOY_ADVANCE,      1 },
  { PLATFORM_NEO_GEO_POCKET_COLOR, 1 },
  { PLATFORM_SEGA_MASTER_SYSTEM,   2 },
  { PLATFORM_SNES,                 2 },
};
*/


/* static */
CGameManager &CGameManager::Get()
{
  static CGameManager _instance;
  return _instance;
}

void CGameManager::Start()
{
  CAddonMgr::Get().RegisterAddonMgrCallback(ADDON_GAMEDLL, this);
  CAddonMgr::Get().RegisterObserver(this);
  CAddonDatabase::RegisterAddonDatabaseCallback(ADDON_GAMEDLL, this);

  CAddonDatabase database;
  if (database.Open())
  {
    VECADDONS addons;
    database.GetAddons(addons); // TODO: Filter by type ADDON_GAMEDLL
    // TODO: either rename this function to UpdateExtensions(), which is more
    // transparent to what it actually does, or abstract it further by creating
    // a callback interface for the Repository Updated action (see Repository.cpp).
    // For now, compromise and pretend that they are remote add-ons.
    UpdateRemoteAddons(addons);
  }

  UpdateAddons();
}

void CGameManager::UpdateRemoteAddons(const ADDON::VECADDONS &addons)
{
  CSingleLock lock(m_critSection);

  for (VECADDONS::const_iterator it = addons.begin(); it != addons.end(); ++it)
  {
    const AddonPtr &addon = *it;
    if (!addon->IsType(ADDON_GAMEDLL))
      continue;

    GameClientPtr gc = boost::dynamic_pointer_cast<CGameClient>(addon);
    if (!gc)
      continue;

    bool bIsBroken = !gc->Props().broken.empty();
    if (!bIsBroken)
    {
      bool bHasExtensions = !gc->GetExtensions().empty();
      if (bHasExtensions)
        m_gameExtensions.insert(gc->GetExtensions().begin(), gc->GetExtensions().end());
      else
        CLog::Log(LOGDEBUG, "GameManager - No extensions for %s version %s", gc->ID().c_str(), gc->Version().c_str());
    }
  }
  CLog::Log(LOGDEBUG, "GameManager: tracking %d remote extensions", (int)(m_gameExtensions.size()));
}

bool CGameManager::UpdateAddons()
{
  VECADDONS gameClients;
  CAddonMgr::Get().GetAddons(ADDON_GAMEDLL, gameClients);
  RegisterAddons(gameClients);
  return true;
}

void CGameManager::Stop()
{
  CAddonMgr::Get().UnregisterAddonMgrCallback(ADDON_GAMEDLL);
  CAddonMgr::Get().UnregisterObserver(this);
  CAddonDatabase::UnregisterAddonDatabaseCallback(ADDON_GAMEDLL);
}

void CGameManager::RegisterAddons(const VECADDONS &addons)
{
  for (VECADDONS::const_iterator it = addons.begin(); it != addons.end(); it++)
  {
    if ((*it)->Type() == ADDON_GAMEDLL)
      RegisterAddon(boost::dynamic_pointer_cast<CGameClient>(*it));
  }
}

bool CGameManager::RegisterAddon(const GameClientPtr &client)
{
#if 0 // TODO
  if (!client || !client->Enabled())
    return false;
#else
  if (!client)
    return false;

  CAddonDatabase database;
  if (!database.Open())
    return false;

  // It's possible for game clients to be enabled but not enabled in the
  // database when they're installed but not configured yet.
  bool bEnabled = client->Enabled() && !database.IsAddonDisabled(client->ID());
  if (!bEnabled)
    return false;
#endif

  CSingleLock lock(m_critSection);

  GameClientPtr &registeredClient = m_gameClients[client->ID()];
  if (registeredClient)
  {
    CLog::Log(LOGDEBUG, "GameManager: Already registered: %s!", client->ID().c_str());
    return true;
  }

  if (!client->Init())
  {
    CLog::Log(LOGERROR, "GameManager: failed to load DLL for %s, disabling in database", client->ID().c_str());
    CGUIDialogKaiToast::QueueNotification(client->Icon(), client->Name(), g_localizeStrings.Get(15023)); // Error loading DLL

    // Removes the game client from m_gameClients via CAddonDatabase callback
    CAddonDatabase database;
    if (database.Open())
      database.DisableAddon(client->ID());
    return false;
  }

  client->DeInit();

  registeredClient = client;
  CLog::Log(LOGDEBUG, "GameManager: Registered add-on %s", client->ID().c_str());

  // If a file was queued by RetroPlayer, try to launch the newly installed game client
  m_fileLauncher.Launch(client);

  return true;
}

void CGameManager::UnregisterAddonByID(const string &strId)
{
  CSingleLock lock(m_critSection);

  GameClientPtr client = m_gameClients[strId];
  if (client)
  {
    if (client->IsInitialized())
      client->DeInit();
  }
  else
  {
    CLog::Log(LOGERROR, "GameManager: can't unregister %s - not registered!", strId.c_str());
  }
  m_gameClients.erase(m_gameClients.find(strId));
}

void CGameManager::AddonEnabled(AddonPtr addon, bool bDisabled)
{
  if (!addon)
    return;

  // If the enabled addon is a game client, register it
  RegisterAddon(boost::dynamic_pointer_cast<CGameClient>(addon));
}

void CGameManager::AddonDisabled(AddonPtr addon)
{
  if (!addon)
    return;

  if (addon->Type() == ADDON_GAMEDLL)
    UnregisterAddonByID(addon->ID());
}

void CGameManager::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageAddons)
    UpdateAddons();
}

bool CGameManager::GetClient(const string &strClientId, GameClientPtr &addon) const
{
  CSingleLock lock(m_critSection);

  map<string, GameClientPtr>::const_iterator itr = m_gameClients.find(strClientId);
  if (itr != m_gameClients.end())
  {
    addon = itr->second;
    return true;
  }
  return false;
}

bool CGameManager::GetConnectedClient(const string &strClientId, GameClientPtr &addon) const
{
  return GetClient(strClientId, addon) && addon->IsInitialized();
}

bool CGameManager::IsConnectedClient(const string &strClientId) const
{
  GameClientPtr client;
  return GetConnectedClient(strClientId, client);
}

bool CGameManager::IsConnectedClient(const AddonPtr addon) const
{
  // See if we are tracking the client
  CSingleLock lock(m_critSection);
  map<string, GameClientPtr>::const_iterator itr = m_gameClients.find(addon->ID());
  if (itr != m_gameClients.end())
    return itr->second->IsInitialized();
  return false;
}

void CGameManager::GetGameClientIDs(const CFileItem& file, vector<string> &candidates) const
{
  CSingleLock lock(m_critSection);

  CStdString requestedClient = file.GetProperty("gameclient").asString();

  for (map<string, GameClientPtr>::const_iterator it = m_gameClients.begin(); it != m_gameClients.end(); it++)
  {
    if (!requestedClient.empty() && requestedClient != it->first)
      continue;

    CLog::Log(LOGDEBUG, "GameManager: To open or not to open using %s, that is the question", it->second->ID().c_str());
    if (CGameFileLoader::CanOpen(*it->second, file))
    {
      CLog::Log(LOGDEBUG, "GameManager: Adding client %s as a candidate", it->second->ID().c_str());
      candidates.push_back(it->second->ID());
    }

    if (!requestedClient.empty())
      break; // If the requested client isn't installed, it's not a valid candidate
  }
}

void CGameManager::GetExtensions(vector<string> &exts) const
{
  exts.insert(exts.end(), m_gameExtensions.begin(), m_gameExtensions.end());
}

bool CGameManager::IsGame(const std::string &path) const
{
  CSingleLock lock(m_critSection);

  // Get the file extension (must use a CURL, if the string is top-level zip
  // directory it might not end in .zip)
  string extension(URIUtils::GetExtension(CURL(path).GetFileNameWithoutPath()));
  StringUtils::ToLower(extension);
  if (extension.empty())
    return false;

  return m_gameExtensions.find(extension) != m_gameExtensions.end();
}

bool CGameManager::StopClient(AddonPtr client, bool bRestart)
{
  // This lock is to ensure that ReCreate() or Destroy() are not started from
  // multiple threads.
  CSingleLock lock(m_critSection);

  GameClientPtr mappedClient;
  if (GetClient(client->ID(), mappedClient))
  {
    CLog::Log(LOGDEBUG, "%s - %s add-on '%s'", __FUNCTION__, bRestart ? "restarting" : "stopping", mappedClient->Name().c_str());
    if (bRestart)
      mappedClient->Init();
    else
      mappedClient->DeInit();

    return bRestart ? mappedClient->IsInitialized() : true;
  }

  return false;
}
