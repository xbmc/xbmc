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

#include <mutex>
#include <optional>
#include <string>
#include <vector>

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
   * A game with no cheat file, or a cheats folder that has not been set, ends
   * up with nothing, which is how the OSD knows not to offer them.
   */
  void Load(const std::string& gamePath);

  //! \brief Forget the cheats and switch off any that were applied
  void Clear();

  //! \brief True while the game being played has cheats to offer
  bool HasCheats() const;

  /*!
   * \brief True when the cheats dialog is worth opening
   *
   * Either there are cheats, or the add-on that carries them can still be
   * fetched, which is the only way the player would find out it exists.
   */
  bool CanOfferCheats() const;

  /*!
   * \brief True while the cheat add-on is missing or switched off
   *
   * \sa InstallCheats()
   */
  bool CanInstallCheats() const;

  /*!
   * \brief Fetch or re-enable the cheat add-on and look again for this game
   *
   * Blocks on the download, so it belongs on a job thread.
   *
   * \return True if cheats were found for the game afterwards
   */
  bool InstallCheats();

  //! \brief The cheats found for this game, and whether each is switched on
  std::vector<Cheat> GetCheats() const;

  /*!
   * \brief Switch one cheat on or off
   *
   * The whole set is re-sent to the client afterwards.
   */
  void SetEnabled(unsigned int index, bool enabled);

private:
  /*!
   * \brief Send the cheats that are switched on to the client
   *
   * Call with the client's lock held and m_mutex free. Every path here takes
   * the two in that order, which is the order a game being closed already
   * holds them in.
   */
  void Apply();

  //! \brief Look up the cheats for the game being played. Call under m_mutex.
  void LoadPack();

  mutable std::mutex m_mutex;

  //! The game the cheats were looked up for, so fetching the add-on can
  //! look again without being told which game is playing
  std::string m_gamePath;
  CCheatPack m_pack;
  std::vector<bool> m_enabled;

  //! Whether the client took the cheat calls. The operations are optional, and
  //! one that returns GAME_ERROR_NOT_IMPLEMENTED can never be cheated at.
  bool m_clientTakesCheats{false};

  //! Whether the database add-on could still be fetched. Looking that up
  //! searches the repositories, and the OSD asks every time the cheats row's
  //! visibility is evaluated, so the answer is kept until an add-on changes.
  mutable std::optional<bool> m_canInstall;
};
} // namespace KODI::GAME
