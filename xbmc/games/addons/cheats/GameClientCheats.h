/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/GameClientSubsystem.h"
#include "games/cheats/CheatPack.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace ADDON
{
class CAddonInfo;
struct AddonEvent;
} // namespace ADDON

namespace KODI::GAME
{
/*!
 * \ingroup games
 *
 * \brief The cheats available to the game this client is playing
 *
 * Owned by the client, so the codes go to the client that was asked for them
 * and are gone once it is.
 */
class CGameClientCheats : protected CGameClientSubsystem
{
public:
  CGameClientCheats(CGameClient& gameClient,
                    AddonInstance_Game& addonStruct,
                    CCriticalSection& clientAccess);
  ~CGameClientCheats() override;

  /*!
   * \brief Look for cheats for a game and hold what is found
   *
   * \param gamePath The path to the game for which cheats should be loaded
   */
  void Load(const std::string& gamePath);

  /*!
   * \brief Forget the cheats and switch off any that were applied
   */
  void Clear();

  /*!
   * \brief True if an exact matching cheat-pack candidate was discovered for this game
   *
   * Independent of pack selection and whether the pack contains usable cheat entries.
   */
  bool HasCheats() const;

  //! Whether the current game client implements the cheat API.
  bool SupportsCheats() const;

  /*!
   * \brief True while the cheat add-on is missing or switched off
   *
   * \sa GetInstallTask()
   */
  bool CanInstallCheats() const;

  enum class InstallResult
  {
    FAILED,
    NO_CHEATS,
    CHEATS_FOUND,
    CHOOSE_PACK,
  };

  /*!
   * \brief Capture an installation request for the current game
   *
   * Returns an empty task when another installation is pending.
   * Run the task on a job thread.
   */
  std::function<InstallResult()> GetInstallTask();

  struct PackCandidate
  {
    std::string id;
    std::string name;
    std::string source;
    std::string path;
  };

  struct PackState
  {
    std::string fileName;
    std::vector<PackCandidate> candidates;
    std::string selected;
    std::vector<Cheat> cheats;
    uint64_t generation{0};

    bool HasMatch() const { return !candidates.empty(); }
    bool NeedsSelection() const { return candidates.size() > 1 && selected.empty(); }
  };

  PackState GetPacks() const;

  // Capture before opening the chooser; run only a confirmed choice on a job thread.
  std::function<bool(const std::string&)> GetSelectionTask(const PackState& expected);

  /*!
   * \brief The cheats found for this game, and whether each is switched on
   *
   * \return A vector of cheats, each paired with a boolean indicating if it is enabled
   */
  std::vector<Cheat> GetCheats() const;

  /*!
   * \brief Switch one cheat on or off
   *
   * The whole set is re-sent to the client afterwards. A stale displayed row
   * is rejected if a reload has replaced it.
   */
  bool SetEnabled(unsigned int index, bool enabled, const Cheat& expected, uint64_t generation = 0);

protected:
  enum class DatabaseState
  {
    MISSING,
    DISABLED,
    AVAILABLE,
  };

  struct Source
  {
    std::string id;
    std::string path;
    std::shared_ptr<const ADDON::CAddonInfo> revision;

    bool operator==(const Source&) const = default;
  };

  virtual DatabaseState GetDatabaseState() const;
  virtual bool IsDatabaseInstallable() const;
  virtual bool InstallDatabase();
  virtual bool EnableDatabase();
  virtual std::vector<Source> GetSources() const;
  virtual bool IsResourceAddon(const std::string& id) const;
  virtual std::vector<PackCandidate> FindCandidates(const Source& source,
                                                    const std::string& fileName);
  virtual CCheatPack ReadPack(const std::string& path);
  virtual std::string GetSelectionPath(const std::string& gamePath) const;
  virtual void Submit(std::function<void()> job);
  void OnAddonEvent(const ADDON::AddonEvent& event);

private:
  struct Session
  {
    explicit Session(std::string path);

    const std::string gamePath;
    const std::string fileName;
    // Clear invalidates the session without waiting for this worker lock.
    std::mutex workMutex;
    std::set<std::string> changedAddons;
    std::map<std::string, uint64_t> addonRevisions;
    // Keep resource IDs recognizable after removal from the add-on manager.
    std::set<std::string> sourceAddons;
    bool reloadQueued{false};
    bool installQueued{false};
    std::optional<std::string> choice;
  };

  InstallResult InstallCheats(const std::shared_ptr<Session>& session);
  bool Reload(const std::shared_ptr<Session>& session,
              bool refreshDialog = true,
              const std::optional<std::string>& selection = std::nullopt);
  std::string ReadChoice(const std::string& gamePath) const;
  void SaveChoice(const std::string& gamePath, const std::string& candidate) const;
  void QueueReload(const std::shared_ptr<Session>& session);
  void ProcessReload(const std::shared_ptr<Session>& session);
  void OnRepositoryUpdated();
  static void RefreshDialog();

  /*!
   * \brief Hand the client a cheat to apply, or take one away
   *
   * The caller must hold m_clientAccess.
   *
   * \param index The slot the code occupies, which is how it is turned off again
   * \param enabled Whether the code should be applied
   * \param code The code, in whatever form the emulated system uses
   *
   * \return True if the cheat was successfully applied or removed, false otherwise
   */
  bool SetCheat(unsigned int index, bool enabled, const std::string& code);

  /*!
   * \brief Drop every cheat the client is holding
   *
   * The caller must hold m_clientAccess.
   *
   * \return True if the cheats were successfully dropped, false otherwise
   */
  bool CheatReset();

  /*!
   * \brief Send the cheats that are switched on to the client
   *
   * Call with the client's lock held and m_mutex free. Every path here takes
   * the two in that order, which is the order a game being closed already
   * holds them in.
   */
  void Apply();

  mutable std::mutex m_mutex;
  std::shared_ptr<Session> m_session;
  std::optional<std::vector<Source>> m_sources;
  PackState m_packs;
  uint64_t m_generation{0};
  CCheatPack m_pack;
  std::vector<bool> m_enabled;

  //! Whether the client took the cheat calls. The operations are optional, and
  //! one that returns GAME_ERROR_NOT_IMPLEMENTED can never be cheated at.
  bool m_clientTakesCheats{false};

  //! Cache installation availability to avoid repeated repository searches
  //! until an add-on's installation state or repository metadata changes.
  mutable std::optional<bool> m_canInstall;
  unsigned int m_installabilityRevision{0};
};
} // namespace KODI::GAME
