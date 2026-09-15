/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientCheats.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/GameResource.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/CriticalSection.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI::GAME;

namespace
{
constexpr auto SETTING_GAMES_CHEATS_PATH = "gamesgeneral.cheatspath";
constexpr auto CHEAT_EXTENSION = ".cht";

//! The add-on carrying the libretro cheat database, one zip per system
constexpr auto CHEATS_ADDON = "resource.games.cheats.libretro";

//! \brief What to search one level down, given a folder of cheats
//!
//! Real subfolders, as the libretro cheat database is published, and the inside
//! of any zip, as an add-on ships one per system rather than tens of thousands
//! of loose files. Kodi reads an archive through the same calls either way.
std::vector<std::string> SystemFolders(const std::string& cheatsFolder)
{
  std::vector<std::string> folders;

  CFileItemList entries;
  if (!XFILE::CDirectory::GetDirectory(cheatsFolder, entries, "", XFILE::DIR_FLAG_NO_FILE_DIRS))
    return folders;

  for (int i = 0; i < entries.Size(); ++i)
  {
    const CFileItemPtr& entry = entries[i];

    if (entry->IsFolder())
      folders.emplace_back(entry->GetPath());
    else if (URIUtils::HasExtension(entry->GetPath(), ".zip"))
      folders.emplace_back(URIUtils::CreateArchivePath("zip", CURL(entry->GetPath())).Get());
  }

  return folders;
}

//! \brief Look for a cheat file beside the folder, then one level inside it
//!
//! The libretro cheat database is published a folder per system, so the
//! setting can point at the database itself or at a folder of loose files.
CCheatPack FindCheats(const std::string& cheatsFolder, const std::string& fileName)
{
  const std::string direct = URIUtils::AddFileToFolder(cheatsFolder, fileName);
  if (XFILE::CFile::Exists(direct))
    return CCheatPack::Load(direct);

  // A database laid out one folder per system can hold the same game name
  // under more than one console, and nothing here says which is meant. Sending
  // another console's codes is worse than sending none.
  std::string match;
  for (const std::string& system : SystemFolders(cheatsFolder))
  {
    const std::string path = URIUtils::AddFileToFolder(system, fileName);
    if (!XFILE::CFile::Exists(path))
      continue;

    if (!match.empty())
    {
      CLog::Log(LOGDEBUG, "CGameClientCheats: \"{}\" is in more than one system, using none",
                fileName);
      return {};
    }

    match = path;
  }

  if (!match.empty())
    return CCheatPack::Load(match);

  return {};
}

//! \brief Every folder a cheat file might be in, in the order they are asked
//!
//! The player's own folder comes first: somebody who has pointed the setting at
//! a database of their own meant it to be used. Behind it come the installed
//! game resource add-ons, which is how cheats are shipped and updated through
//! the repository without anyone downloading anything by hand.
std::vector<std::string> CheatSources()
{
  std::vector<std::string> sources;

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const std::string folder = settings->GetString(SETTING_GAMES_CHEATS_PATH);
  if (!folder.empty())
    sources.emplace_back(folder);

  ADDON::VECADDONS addons;
  if (CServiceBroker::GetAddonMgr().GetAddons(addons, ADDON::AddonType::RESOURCE_GAMES))
  {
    for (const ADDON::AddonPtr& addon : addons)
    {
      const auto resource = std::static_pointer_cast<ADDON::CGameResource>(addon);
      sources.emplace_back(resource->GetFullPath(""));
    }
  }

  return sources;
}
} // namespace

CGameClientCheats::CGameClientCheats(CGameClient& gameClient,
                                     AddonInstance_Game& addonStruct,
                                     CCriticalSection& clientAccess)
  : CGameClientSubsystem(gameClient, addonStruct, clientAccess)
{
  CServiceBroker::GetAddonMgr().Events().Subscribe(this,
                                                   [this](const ADDON::AddonEvent& /*event*/)
                                                   {
                                                     std::lock_guard<std::mutex> lock(m_mutex);
                                                     m_canInstall.reset();
                                                   });
}

CGameClientCheats::~CGameClientCheats()
{
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
}

void CGameClientCheats::Load(const std::string& gamePath)
{
  std::unique_lock clientLock(m_clientAccess);

  // Asked here rather than when a cheat is applied, which is too late to
  // decide whether to offer any. Resetting an untouched game changes nothing.
  const bool clientTakesCheats = m_gameClient.CheatReset();

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_clientTakesCheats = clientTakesCheats;
    m_gamePath = gamePath;

    // Looking for a pack means enumerating every game resource add-on and
    // reading into archives, which is wasted on a client that cannot be
    // cheated at and whose row will never be offered
    if (clientTakesCheats)
      LoadPack();
  }

  Apply();
}

void CGameClientCheats::LoadPack()
{
  m_pack = CCheatPack();
  m_enabled.clear();

  if (m_gamePath.empty())
    return;

  // The cheat file is named after the game, which is how the libretro cheat
  // database is published
  std::string name = URIUtils::GetFileName(m_gamePath);
  URIUtils::RemoveExtension(name);

  for (const std::string& source : CheatSources())
  {
    m_pack = FindCheats(source, name + CHEAT_EXTENSION);
    if (!m_pack.IsEmpty())
      break;
  }

  if (m_pack.IsEmpty())
    return;

  m_enabled.reserve(m_pack.Cheats().size());
  for (const Cheat& cheat : m_pack.Cheats())
    m_enabled.push_back(cheat.enabled);

  CLog::Log(LOGINFO, "CGameClientCheats: {} cheat(s) for \"{}\"", m_pack.Cheats().size(), name);
}

void CGameClientCheats::Clear()
{
  std::unique_lock clientLock(m_clientAccess);

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pack = CCheatPack();
    m_enabled.clear();
    // Also the game and what the client said about it: the same client can go
    // on to open a standalone title, which never looks a game path up, and
    // would otherwise be offered the last game's cheats
    m_gamePath.clear();
    m_clientTakesCheats = false;
  }

  if (m_gameClient.IsPlaying())
    m_gameClient.CheatReset();
}

bool CGameClientCheats::HasCheats() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_clientTakesCheats && !m_pack.IsEmpty();
}

bool CGameClientCheats::CanOfferCheats() const
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_clientTakesCheats)
      return false;
    if (!m_pack.IsEmpty())
      return true;
  }

  // Nothing for this game, but the player has never been offered the database
  return CanInstallCheats();
}

bool CGameClientCheats::CanInstallCheats() const
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_canInstall.has_value())
      return *m_canInstall;
  }

  ADDON::CAddonMgr& addons = CServiceBroker::GetAddonMgr();

  bool canInstall = false;
  if (addons.IsAddonInstalled(CHEATS_ADDON))
  {
    canInstall = addons.IsAddonDisabled(CHEATS_ADDON);
  }
  else
  {
    // Searches the repositories, which is why the answer is kept rather than
    // asked again for every evaluation of the cheats row
    ADDON::AddonPtr addon;
    canInstall = addons.FindInstallableById(CHEATS_ADDON, addon);
  }

  std::lock_guard<std::mutex> lock(m_mutex);
  m_canInstall = canInstall;
  return canInstall;
}

bool CGameClientCheats::InstallCheats()
{
  ADDON::CAddonMgr& addons = CServiceBroker::GetAddonMgr();

  bool ready = false;
  if (addons.IsAddonDisabled(CHEATS_ADDON))
  {
    ready = addons.EnableAddon(CHEATS_ADDON);
  }
  else if (!addons.IsAddonInstalled(CHEATS_ADDON))
  {
    ADDON::AddonPtr addon;
    ready = ADDON::CAddonInstaller::GetInstance().InstallModal(
        CHEATS_ADDON, addon, ADDON::InstallModalPrompt::CHOICE_NO);
  }

  if (!ready)
    return false;

  {
    std::unique_lock clientLock(m_clientAccess);

    {
      std::lock_guard<std::mutex> lock(m_mutex);
      LoadPack();
    }

    Apply();
  }

  return HasCheats();
}

std::vector<Cheat> CGameClientCheats::GetCheats() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::vector<Cheat> cheats = m_pack.Cheats();
  for (size_t i = 0; i < cheats.size() && i < m_enabled.size(); ++i)
    cheats[i].enabled = m_enabled[i];

  return cheats;
}

void CGameClientCheats::SetEnabled(unsigned int index, bool enabled)
{
  std::unique_lock clientLock(m_clientAccess);

  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (index >= m_enabled.size())
      return;

    m_enabled[index] = enabled;
  }

  Apply();
}

void CGameClientCheats::Apply()
{
  if (!m_gameClient.IsPlaying())
    return;

  std::vector<std::string> codes;
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Only the ones switched on are sent. Cores are not obliged to honour the
    // enabled flag and several ignore it outright, applying whatever they are
    // handed -- fceumm adds every code it is given -- so a cheat that is off
    // has to be left out rather than sent as disabled.
    const std::vector<Cheat>& cheats = m_pack.Cheats();
    for (size_t i = 0; i < cheats.size() && i < m_enabled.size(); ++i)
    {
      if (m_enabled[i])
        codes.emplace_back(cheats[i].code);
    }
  }

  // A cheat is identified by the slot it was given, so the set is sent whole
  // rather than one code at a time: switching one off means the ones after it
  // would otherwise answer to the wrong index.
  m_gameClient.CheatReset();

  unsigned int slot = 0;
  for (const std::string& code : codes)
    m_gameClient.SetCheat(slot++, true, code);
}
