/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

struct AddonInstance_Game;
struct game_rc_progress_indicator;
struct game_rc_challenge_indicator;
struct game_rc_achievement_progress;
struct game_rc_achievement_triggered;
struct game_rc_game_loaded;
struct game_rc_login_result;

namespace KODI
{

namespace GAME
{

class CGameClient;

/*!
 * \ingroup games
 */
class CGameClientCheevos
{
public:
  CGameClientCheevos(CGameClient& gameClient, AddonInstance_Game& addonStruct);

  /*!
   * \name RetroAchievements events received from the add-on
   *
   * These are called on the add-on's thread. They publish to the achievement
   * runtime and post notifications; they must not block.
   */
  //@{
  void OnGameLoaded(const game_rc_game_loaded& data);
  void OnAchievementTriggered(const game_rc_achievement_triggered& data);
  void OnGameCompleted(const std::string& title, bool hardcore);
  void OnRichPresenceUpdated(const std::string& evaluation);
  void OnLoginResult(const game_rc_login_result& data);
  void OnAchievementProgress(const game_rc_achievement_progress* progress, unsigned int count);

  //! \brief Show the achievement the player is working towards, or hide it when null
  void OnProgressIndicator(const game_rc_progress_indicator* indicator);

  //! \brief Show the achievement being attempted, or hide it when null
  void OnChallengeIndicator(const game_rc_challenge_indicator* indicator);
  void OnServerError(const std::string& message, const std::string& api);
  void OnConnectionChanged(bool connected);
  //@}

  /*!
   * \brief Drop the published state when the game closes
   *
   * The next game's state doesn't arrive until the add-on has identified it,
   * which may be several seconds away and may never happen. Without this the
   * OSD would keep showing the previous game's achievements in the meantime.
   */
  void OnGameClosed();

  /*!
   * \brief Hand the add-on the credentials to sign in with
   *
   * The account is entered in Kodi's settings, so the add-on can only sign in
   * with what Kodi gives it. Sent before each game is loaded, since the token
   * can change between games.
   *
   * Sent for every game, including when the player is signed out, where the
   * credentials are empty and tell the client to drop any session it holds.
   *
   * \return True if the client accepted them
   */
  bool SendCredentials();

private:
  CGameClient& m_gameClient;
  AddonInstance_Game& m_struct;
};
} // namespace GAME
} // namespace KODI
