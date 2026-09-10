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
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/AddonEvents.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/GameResource.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "jobs/JobManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/CriticalSection.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <deque>
#include <map>
#include <utility>

using namespace KODI::GAME;

namespace
{
constexpr auto SETTING_GAMES_CHEATS_PATH = "gamesgeneral.cheatspath";

constexpr auto CHEAT_EXTENSION = ".cht";

//! The add-on carrying the libretro cheat database, one zip per system
constexpr auto CHEATS_ADDON = "resource.games.cheats.libretro";

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

} // namespace

CGameClientCheats::CGameClientCheats(CGameClient& gameClient,
                                     AddonInstance_Game& addonStruct,
                                     CCriticalSection& clientAccess)
  : CGameClientSubsystem(gameClient, addonStruct, clientAccess)
{
}

CGameClientCheats::~CGameClientCheats() = default;

void CGameClientCheats::Load(const std::string& gamePath)
{
  Clear();
  const auto session = std::make_shared<Session>(gamePath);
  std::unique_lock workLock(session->workMutex);
  {
    std::unique_lock clientLock(m_clientAccess);
    const bool clientTakesCheats = CheatReset();
    std::lock_guard lock(m_mutex);
    m_session = session;
    m_clientTakesCheats = clientTakesCheats;
  }

  CServiceBroker::GetAddonMgr().Events().Subscribe(this, [this](const ADDON::AddonEvent& event)
                                                   { OnAddonEvent(event); });

  Reload(session, false);
}

bool CGameClientCheats::Reload(const std::shared_ptr<Session>& session, bool refreshDialog)
{
  {
    std::lock_guard lock(m_mutex);
    if (m_session != session || !m_clientTakesCheats)
      return false;
  }

  const auto sources = GetSources();
  {
    std::lock_guard lock(m_mutex);
    if (m_session != session)
      return false;
    if (m_sources && *m_sources == sources)
      return true;
  }

  std::string name = URIUtils::GetFileName(session->gamePath);
  URIUtils::RemoveExtension(name);
  CCheatPack pack;
  std::optional<Source> packSource;
  if (!session->gamePath.empty())
  {
    for (const auto& source : sources)
    {
      pack = ReadPack(source.path, name + CHEAT_EXTENSION);
      if (!pack.IsEmpty())
      {
        packSource = source;
        break;
      }
    }
  }

  // Archive searches must not hold either lock used by the player or GUI-info queries.
  std::unique_lock clientLock(m_clientAccess);
  {
    std::lock_guard lock(m_mutex);
    if (m_session != session)
      return false;

    std::map<std::pair<std::string, std::string>, std::deque<bool>> enabled;
    if (m_packSource && packSource && m_packSource->id == packSource->id &&
        m_packSource->path == packSource->path)
    {
      const auto& oldCheats = m_pack.Cheats();
      for (size_t i = 0; i < oldCheats.size(); ++i)
        enabled[{oldCheats[i].description, oldCheats[i].code}].push_back(m_enabled[i]);
    }

    m_enabled.clear();
    for (const auto& cheat : pack.Cheats())
    {
      const auto it = enabled.find({cheat.description, cheat.code});
      if (it != enabled.end() && !it->second.empty())
      {
        m_enabled.push_back(it->second.front());
        it->second.pop_front();
      }
      else
        m_enabled.push_back(cheat.enabled);
    }
    m_pack = std::move(pack);
    m_packSource = std::move(packSource);
    m_sources = sources;
  }
  Apply();
  clientLock.unlock();
  if (refreshDialog)
    RefreshDialog();
  return true;
}

void CGameClientCheats::Clear()
{
  // Unsubscribe waits for callbacks. They must never acquire the client lock.
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);

  std::unique_lock clientLock(m_clientAccess);
  bool hadSession;
  {
    std::lock_guard lock(m_mutex);
    hadSession = m_session != nullptr;
    m_session.reset();
    m_sources.reset();
    m_packSource.reset();
    m_pack = CCheatPack();
    m_enabled.clear();
    m_clientTakesCheats = false;
    m_canInstall.reset();
    ++m_availabilityRevision;
  }

  if (hadSession && m_gameClient.IsPlaying())
    CheatReset();
}

void CGameClientCheats::OnAddonEvent(const ADDON::AddonEvent& event)
{
  std::shared_ptr<Session> session;
  {
    std::lock_guard lock(m_mutex);
    m_canInstall.reset();
    ++m_availabilityRevision;
    if (!m_session || !(typeid(event) == typeid(ADDON::AddonEvents::Enabled) ||
                        typeid(event) == typeid(ADDON::AddonEvents::Disabled) ||
                        typeid(event) == typeid(ADDON::AddonEvents::ReInstalled) ||
                        typeid(event) == typeid(ADDON::AddonEvents::UnInstalled)))
      return;

    session = m_session;
    session->changedAddons.insert(event.addonId);
  }
  QueueReload(session);
}

void CGameClientCheats::QueueReload(const std::shared_ptr<Session>& session)
{
  {
    std::lock_guard lock(m_mutex);
    if (m_session != session || !m_clientTakesCheats || session->reloadQueued ||
        session->installQueued || session->changedAddons.empty())
      return;
    session->reloadQueued = true;
  }

  Submit(
      [client = m_gameClient.weak_from_this(), session]
      {
        if (const auto owner = client.lock())
          std::static_pointer_cast<CGameClient>(owner)->Cheats().ProcessReload(session);
      });
}

void CGameClientCheats::ProcessReload(const std::shared_ptr<Session>& session)
{
  std::unique_lock workLock(session->workMutex);
  while (true)
  {
    std::set<std::string> changedAddons;
    std::vector<Source> previousSources;
    {
      std::lock_guard lock(m_mutex);
      if (m_session != session || session->installQueued || session->changedAddons.empty())
      {
        session->reloadQueued = false;
        return;
      }
      changedAddons.swap(session->changedAddons);
      if (m_sources)
        previousSources = *m_sources;
    }

    // Add-on manager queries belong outside callbacks: publishing and unsubscribing
    // can otherwise wait on each other while holding the manager and stream locks.
    const bool relevant =
        std::any_of(changedAddons.begin(), changedAddons.end(),
                    [this, &previousSources](const std::string& id)
                    {
                      return id == CHEATS_ADDON ||
                             std::any_of(previousSources.begin(), previousSources.end(),
                                         [&id](const Source& source) { return source.id == id; }) ||
                             IsResourceAddon(id);
                    });
    if (relevant)
      Reload(session);
  }
}

void CGameClientCheats::Submit(std::function<void()> job)
{
  CServiceBroker::GetJobManager()->Submit(std::move(job));
}

void CGameClientCheats::RefreshDialog()
{
  if (const auto gui = CServiceBroker::GetGUI())
  {
    CGUIMessage message(GUI_MSG_UPDATE, WINDOW_DIALOG_GAME_CHEATS, -1);
    gui->GetWindowManager().SendThreadMessage(message, WINDOW_DIALOG_GAME_CHEATS);
  }
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

  return CanInstallCheats();
}

bool CGameClientCheats::CanInstallCheats() const
{
  unsigned int revision;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_canInstall.has_value())
      return *m_canInstall;
    revision = m_availabilityRevision;
  }

  const auto state = GetDatabaseState();
  const bool canInstall = state == DatabaseState::DISABLED ||
                          (state == DatabaseState::MISSING && IsDatabaseInstallable());

  std::lock_guard<std::mutex> lock(m_mutex);
  if (revision == m_availabilityRevision)
    m_canInstall = canInstall;
  return canInstall;
}

CGameClientCheats::DatabaseState CGameClientCheats::GetDatabaseState() const
{
  const auto& addons = CServiceBroker::GetAddonMgr();
  if (!addons.IsAddonInstalled(CHEATS_ADDON))
    return DatabaseState::MISSING;
  return addons.IsAddonDisabled(CHEATS_ADDON) ? DatabaseState::DISABLED : DatabaseState::AVAILABLE;
}

bool CGameClientCheats::IsDatabaseInstallable() const
{
  ADDON::AddonPtr addon;
  return CServiceBroker::GetAddonMgr().FindInstallableById(CHEATS_ADDON, addon);
}

bool CGameClientCheats::InstallDatabase()
{
  ADDON::AddonPtr addon;
  return ADDON::CAddonInstaller::GetInstance().InstallModal(CHEATS_ADDON, addon,
                                                            ADDON::InstallModalPrompt::CHOICE_NO);
}

bool CGameClientCheats::EnableDatabase()
{
  return CServiceBroker::GetAddonMgr().EnableAddon(CHEATS_ADDON);
}

std::function<CGameClientCheats::InstallResult()> CGameClientCheats::GetInstallTask()
{
  std::lock_guard lock(m_mutex);
  if (!m_session || !m_clientTakesCheats || m_session->installQueued)
    return {};
  m_session->installQueued = true;
  return [client = m_gameClient.weak_from_this(), session = m_session]
  {
    if (const auto owner = client.lock())
      return std::static_pointer_cast<CGameClient>(owner)->Cheats().InstallCheats(session);
    return InstallResult::FAILED;
  };
}

CGameClientCheats::InstallResult CGameClientCheats::InstallCheats(
    const std::shared_ptr<Session>& session)
{
  std::unique_lock workLock(session->workMutex);
  {
    std::lock_guard lock(m_mutex);
    if (m_session != session)
      return InstallResult::FAILED;
  }

  const auto state = GetDatabaseState();
  const bool ready = state == DatabaseState::AVAILABLE ||
                     (state == DatabaseState::DISABLED ? EnableDatabase() : InstallDatabase());
  const bool available = ready && GetDatabaseState() == DatabaseState::AVAILABLE;
  const bool current =
      available && Reload(session, false) && GetDatabaseState() == DatabaseState::AVAILABLE;
  InstallResult result = InstallResult::FAILED;
  {
    std::lock_guard lock(m_mutex);
    session->installQueued = false;
    if (m_session != session)
      return result;
    m_canInstall.reset();
    ++m_availabilityRevision;
    if (current)
      result = m_pack.IsEmpty() ? InstallResult::NO_CHEATS : InstallResult::CHEATS_FOUND;
  }

  QueueReload(session);
  RefreshDialog();
  return result;
}

std::vector<CGameClientCheats::Source> CGameClientCheats::GetSources() const
{
  std::vector<Source> sources;
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const std::string folder = settings->GetString(SETTING_GAMES_CHEATS_PATH);
  if (!folder.empty())
    sources.push_back({{}, folder, {}});

  ADDON::VECADDONS addons;
  CServiceBroker::GetAddonMgr().GetAddons(addons, ADDON::AddonType::RESOURCE_GAMES);
  for (const auto& addon : addons)
  {
    const auto resource = std::static_pointer_cast<ADDON::CGameResource>(addon);
    sources.push_back({addon->ID(), resource->GetFullPath(""), resource->AddonInfo()});
  }
  return sources;
}

bool CGameClientCheats::IsResourceAddon(const std::string& id) const
{
  return CServiceBroker::GetAddonMgr().GetAddonInfo(id, ADDON::AddonType::RESOURCE_GAMES) !=
         nullptr;
}

CCheatPack CGameClientCheats::ReadPack(const std::string& path, const std::string& fileName)
{
  return FindCheats(path, fileName);
}

std::vector<Cheat> CGameClientCheats::GetCheats() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::vector<Cheat> cheats = m_pack.Cheats();
  for (size_t i = 0; i < cheats.size() && i < m_enabled.size(); ++i)
    cheats[i].enabled = m_enabled[i];

  return cheats;
}

bool CGameClientCheats::SetEnabled(unsigned int index, bool enabled, const Cheat& expected)
{
  std::unique_lock clientLock(m_clientAccess);

  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (index >= m_enabled.size() || m_pack.Cheats()[index].code != expected.code ||
        m_pack.Cheats()[index].description != expected.description)
      return false;

    m_enabled[index] = enabled;
  }

  Apply();
  return true;
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
  CheatReset();

  unsigned int slot = 0;
  for (const std::string& code : codes)
    SetCheat(slot++, true, code);
}

bool CGameClientCheats::SetCheat(unsigned int index, bool enabled, const std::string& code)
{
  std::unique_lock lock(m_clientAccess);

  try
  {
    return m_gameClient.LogError(
        m_struct.toAddon->SetCheat(&m_struct, index, enabled, code.c_str()), "SetCheat()");
  }
  catch (...)
  {
    m_gameClient.LogException("SetCheat()");
  }

  return false;
}

bool CGameClientCheats::CheatReset()
{
  std::unique_lock lock(m_clientAccess);

  try
  {
    return m_gameClient.LogError(m_struct.toAddon->CheatReset(&m_struct), "CheatReset()");
  }
  catch (...)
  {
    m_gameClient.LogException("CheatReset()");
  }

  return false;
}
