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
#include "addons/RepositoryUpdater.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "games/cheats/CheatUtils.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "jobs/JobManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/CriticalSection.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <algorithm>
#include <deque>
#include <map>
#include <utility>

using namespace KODI::GAME;

namespace
{
constexpr auto SETTING_GAMES_CHEATS_PATH = "gamesgeneral.cheatspath";

//! The add-on carrying the libretro cheat database, one zip per system
constexpr auto CHEATS_ADDON = "resource.games.cheats.libretro";

} // namespace

CGameClientCheats::Session::Session(std::string path)
  : gamePath(std::move(path)),
    fileName(CCheatUtils::GetCheatFileName(gamePath))
{
}

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

  CServiceBroker::GetRepositoryUpdater().Events().Subscribe(
      this, [this](const ADDON::CRepositoryUpdater::RepositoryUpdated&) { OnRepositoryUpdated(); });

  Reload(session, false);
}

bool CGameClientCheats::Reload(const std::shared_ptr<Session>& session,
                               bool refreshDialog,
                               const std::optional<std::string>& selection)
{
  std::map<std::string, uint64_t> revisions;
  {
    std::lock_guard lock(m_mutex);
    if (m_session != session || !m_clientTakesCheats)
      return false;
    revisions = session->addonRevisions;
  }

  const auto sources = GetSources();
  {
    std::lock_guard lock(m_mutex);
    if (m_session != session)
      return false;
    for (const auto& source : sources)
    {
      if (!source.id.empty())
        session->sourceAddons.insert(source.id);
    }
    if (!selection && m_sources && *m_sources == sources)
      return true;
  }

  if (!session->choice)
    session->choice = ReadChoice(session->gamePath);
  const std::string& choice = selection ? *selection : *session->choice;
  PackState packs;
  std::map<std::pair<std::string, std::string>, std::vector<std::pair<size_t, std::string>>> labels;
  std::optional<CCheatPack> customPack;
  if (!session->gamePath.empty())
  {
    for (const auto& source : sources)
    {
      auto candidates = FindCandidates(source, session->fileName);
      if (source.id.empty() && candidates.size() == 1)
      {
        customPack = ReadPack(candidates.front().path);
        if (customPack->IsEmpty() && choice != candidates.front().id)
        {
          customPack.reset();
          continue;
        }
      }
      const std::string sourceId = CURL::GetRedacted(source.id.empty() ? source.path : source.id);
      for (const auto& candidate : candidates)
      {
        labels[{candidate.name, candidate.source}].emplace_back(packs.candidates.size(), sourceId);
        packs.candidates.push_back(candidate);
      }
      // A matching custom folder overrides installed resources.
      if (source.id.empty() && !candidates.empty())
        break;
    }
  }

  for (const auto& [label, rows] : labels)
  {
    if (rows.size() < 2)
      continue;
    for (size_t i = 0; i < rows.size(); ++i)
    {
      const auto& [index, sourceId] = rows[i];
      auto& source = packs.candidates[index].source;
      if (source != sourceId)
        source += " (" + sourceId + ")";
      if (std::count_if(rows.begin(), rows.end(),
                        [&sourceId](const auto& row) { return row.second == sourceId; }) > 1)
        source += " (" + std::to_string(i + 1) + ")";
    }
  }

  auto selected = std::find_if(packs.candidates.begin(), packs.candidates.end(),
                               [&choice](const PackCandidate& pack) { return pack.id == choice; });
  const bool accepted = selection && selected != packs.candidates.end();
  if (selection && !accepted)
    selected =
        std::find_if(packs.candidates.begin(), packs.candidates.end(),
                     [&session](const PackCandidate& pack) { return pack.id == *session->choice; });
  if (selected == packs.candidates.end() && packs.candidates.size() == 1)
    selected = packs.candidates.begin();

  CCheatPack pack;
  if (selected != packs.candidates.end())
  {
    packs.selected = selected->id;
    pack = customPack ? std::move(*customPack) : ReadPack(selected->path);
  }

  // Archive searches must not hold either lock used by the player or GUI-info queries.
  std::unique_lock clientLock(m_clientAccess, std::defer_lock);
  while (true)
  {
    std::map<std::string, uint64_t> currentRevisions;
    std::set<std::string> sourceAddons;
    {
      std::lock_guard lock(m_mutex);
      if (m_session != session)
        return false;
      currentRevisions = session->addonRevisions;
      sourceAddons = session->sourceAddons;
    }
    // Classify events outside callbacks and the client lock, including newly installed resources.
    for (const auto& [id, revision] : currentRevisions)
    {
      const auto previous = revisions.find(id);
      if ((previous == revisions.end() || previous->second != revision) &&
          (id == CHEATS_ADDON || sourceAddons.contains(id) || IsResourceAddon(id)))
        return false;
    }
    revisions = std::move(currentRevisions);

    clientLock.lock();
    std::lock_guard lock(m_mutex);
    if (m_session != session)
      return false;
    if (revisions != session->addonRevisions)
    {
      clientLock.unlock();
      continue;
    }

    std::map<std::pair<std::string, std::string>, std::deque<bool>> enabled;
    if (!m_packs.selected.empty() && m_packs.selected == packs.selected)
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
    m_packs = std::move(packs);
    ++m_generation;
    m_sources = sources;
    break;
  }
  Apply();
  clientLock.unlock();
  if (accepted)
  {
    session->choice = *selection;
    SaveChoice(session->gamePath, *selection);
  }
  if (refreshDialog)
    RefreshDialog();
  return !selection || accepted;
}

void CGameClientCheats::Clear()
{
  // Unsubscribe waits for callbacks. They must never acquire the client lock.
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
  CServiceBroker::GetRepositoryUpdater().Events().Unsubscribe(this);

  std::unique_lock clientLock(m_clientAccess);
  bool hadSession;
  {
    std::lock_guard lock(m_mutex);
    hadSession = m_session != nullptr;
    m_session.reset();
    m_sources.reset();
    m_packs = {};
    ++m_generation;
    m_pack = CCheatPack();
    m_enabled.clear();
    m_clientTakesCheats = false;
    m_canInstall.reset();
    ++m_installabilityRevision;
  }

  if (hadSession && m_gameClient.IsPlaying())
    CheatReset();
}

void CGameClientCheats::OnAddonEvent(const ADDON::AddonEvent& event)
{
  std::shared_ptr<Session> session;
  {
    std::lock_guard lock(m_mutex);
    if (!m_session || !(typeid(event) == typeid(ADDON::AddonEvents::Enabled) ||
                        typeid(event) == typeid(ADDON::AddonEvents::Disabled) ||
                        typeid(event) == typeid(ADDON::AddonEvents::ReInstalled) ||
                        typeid(event) == typeid(ADDON::AddonEvents::UnInstalled)))
      return;

    m_canInstall.reset();
    ++m_installabilityRevision;
    session = m_session;
    ++session->addonRevisions[event.addonId];
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
    std::set<std::string> sourceAddons;
    {
      std::lock_guard lock(m_mutex);
      if (m_session != session || session->installQueued || session->changedAddons.empty())
      {
        session->reloadQueued = false;
        return;
      }
      changedAddons.swap(session->changedAddons);
      sourceAddons = session->sourceAddons;
    }

    // Add-on manager queries belong outside callbacks: publishing and unsubscribing
    // can otherwise wait on each other while holding the manager and stream locks.
    const bool relevant = std::any_of(
        changedAddons.begin(), changedAddons.end(), [this, &sourceAddons](const std::string& id)
        { return id == CHEATS_ADDON || sourceAddons.contains(id) || IsResourceAddon(id); });
    if (relevant)
      Reload(session);
  }
}

void CGameClientCheats::OnRepositoryUpdated()
{
  {
    std::lock_guard lock(m_mutex);
    if (!m_session)
      return;
    m_canInstall.reset();
    ++m_installabilityRevision;
  }
  RefreshDialog();
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
  return m_clientTakesCheats && m_packs.HasMatch();
}

bool CGameClientCheats::SupportsCheats() const
{
  std::lock_guard lock(m_mutex);
  return m_clientTakesCheats;
}

bool CGameClientCheats::CanInstallCheats() const
{
  unsigned int revision;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_canInstall.has_value())
      return *m_canInstall;
    revision = m_installabilityRevision;
  }

  const auto state = GetDatabaseState();
  const bool canInstall = state == DatabaseState::DISABLED ||
                          (state == DatabaseState::MISSING && IsDatabaseInstallable());

  std::lock_guard<std::mutex> lock(m_mutex);
  if (revision == m_installabilityRevision)
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
    ++m_installabilityRevision;
    if (current)
      result = m_packs.NeedsSelection() ? InstallResult::CHOOSE_PACK
               : m_pack.IsEmpty()       ? InstallResult::NO_CHEATS
                                        : InstallResult::CHEATS_FOUND;
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

std::vector<CGameClientCheats::PackCandidate> CGameClientCheats::FindCandidates(
    const Source& source, const std::string& fileName)
{
  std::vector<PackCandidate> candidates;
  const std::string sourceId = source.id.empty() ? source.path : source.id;
  const std::string sourceName =
      !source.id.empty() && source.revision ? source.revision->Name() : "";
  const std::string sourceLabel = sourceName.empty() ? CURL::GetRedacted(sourceId) : sourceName;
  const auto add = [&](const std::string& system, const std::string& folder)
  {
    const std::string path = URIUtils::AddFileToFolder(folder, fileName);
    if (!XFILE::CFile::Exists(path))
      return;

    std::string name = system;
    if (name.empty())
    {
      name = source.path;
      URIUtils::RemoveSlashAtEnd(name);
      name = URIUtils::GetFileName(name);
    }
    URIUtils::RemoveSlashAtEnd(name);
    if (URIUtils::HasExtension(name, ".zip"))
      URIUtils::RemoveExtension(name);
    candidates.push_back(
        {CURL::Encode(sourceId) + "/" + CURL::Encode(system) + "/" + CURL::Encode(fileName), name,
         sourceLabel, path});
  };

  // A file at the source root takes precedence over its system folders and archives.
  add("", source.path);
  if (!candidates.empty())
    return candidates;

  CFileItemList entries;
  if (XFILE::CDirectory::GetDirectory(source.path, entries, "", XFILE::DIR_FLAG_NO_FILE_DIRS))
  {
    for (int i = 0; i < entries.Size(); ++i)
    {
      const auto& entry = entries[i];
      std::string system = entry->GetPath();
      URIUtils::RemoveSlashAtEnd(system);
      system = URIUtils::GetFileName(system);
      if (entry->IsFolder())
        add(system + "/", entry->GetPath());
      else if (URIUtils::HasExtension(entry->GetPath(), ".zip"))
        add(system, URIUtils::CreateArchivePath("zip", CURL(entry->GetPath())).Get());
    }
  }
  std::sort(candidates.begin(), candidates.end(),
            [](const PackCandidate& left, const PackCandidate& right)
            { return left.id < right.id; });
  return candidates;
}

CCheatPack CGameClientCheats::ReadPack(const std::string& path)
{
  return CCheatPack::Load(path);
}

std::string CGameClientCheats::GetSelectionPath(const std::string& gamePath) const
{
  return URIUtils::AddFileToFolder("special://masterprofile/games/cheats",
                                   CCheatUtils::GetSelectionFileName(gamePath));
}

std::string CGameClientCheats::ReadChoice(const std::string& gamePath) const
{
  if (gamePath.empty())
    return {};
  CXBMCTinyXML2 xml;
  if (xml.LoadFile(GetSelectionPath(gamePath)))
  {
    const auto* root = xml.FirstChildElement("cheatpack");
    if (root && root->Attribute("game", gamePath.c_str()))
    {
      if (const char* candidate = root->Attribute("candidate"))
        return candidate;
    }
  }
  return {};
}

void CGameClientCheats::SaveChoice(const std::string& gamePath, const std::string& candidate) const
{
  const std::string path = GetSelectionPath(gamePath);
  CXBMCTinyXML2 xml;
  auto* root = xml.NewElement("cheatpack");
  root->SetAttribute("game", gamePath.c_str());
  root->SetAttribute("candidate", candidate.c_str());
  xml.InsertEndChild(root);
  if (!XFILE::CDirectory::Create(URIUtils::GetDirectory(path)) || !xml.SaveFile(path))
    CLog::Log(LOGWARNING, "CGameClientCheats: Failed to save pack selection for {}",
              CURL::GetRedacted(gamePath));
}

CGameClientCheats::PackState CGameClientCheats::GetPacks() const
{
  std::lock_guard lock(m_mutex);
  PackState state = m_packs;
  if (m_session)
    state.fileName = m_session->fileName;
  state.generation = m_generation;
  state.cheats = m_pack.Cheats();
  for (size_t i = 0; i < state.cheats.size(); ++i)
    state.cheats[i].enabled = m_enabled[i];
  return state;
}

std::function<bool(const std::string&)> CGameClientCheats::GetSelectionTask(
    const PackState& expected)
{
  std::lock_guard lock(m_mutex);
  if (!m_session || !m_clientTakesCheats || m_packs.candidates.size() < 2 ||
      expected.generation != m_generation)
    return {};
  return [client = m_gameClient.weak_from_this(), session = m_session](const std::string& id)
  {
    if (const auto owner = client.lock())
    {
      std::unique_lock workLock(session->workMutex);
      return std::static_pointer_cast<CGameClient>(owner)->Cheats().Reload(session, true, id);
    }
    return false;
  };
}

std::vector<Cheat> CGameClientCheats::GetCheats() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::vector<Cheat> cheats = m_pack.Cheats();
  for (size_t i = 0; i < cheats.size() && i < m_enabled.size(); ++i)
    cheats[i].enabled = m_enabled[i];

  return cheats;
}

bool CGameClientCheats::SetEnabled(unsigned int index,
                                   bool enabled,
                                   const Cheat& expected,
                                   uint64_t generation)
{
  std::unique_lock clientLock(m_clientAccess);

  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if ((generation != 0 && generation != m_generation) || index >= m_enabled.size() ||
        m_pack.Cheats()[index].code != expected.code ||
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
