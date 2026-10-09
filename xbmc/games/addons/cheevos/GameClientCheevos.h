/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Observer.h"

#include <atomic>
#include <string>

class CCriticalSection;

struct AddonInstance_Game;
struct game_rc_achievement_challenge;
struct game_rc_achievement_progress;
struct game_rc_achievement_progress_indicator;
struct game_rc_leaderboard;
struct game_rc_leaderboard_scoreboard;
struct game_rc_leaderboard_tracker;
struct game_rc_achievement_triggered;
struct game_rc_game_loaded;
struct game_rc_login_result;

namespace KODI
{

namespace GAME
{

class CAchievementRuntime;
class CGameClient;

/*!
 * \ingroup games
 */
class CGameClientCheevos : public Observer
{
public:
  CGameClientCheevos(CGameClient& gameClient,
                     AddonInstance_Game& addonStruct,
                     CCriticalSection& clientAccess);
  ~CGameClientCheevos() override;

  // Implementation of Observer
  void Notify(const Observable& obs, const ObservableMessage msg) override;

  /*!
   * \name RetroAchievements events received from the add-on
   *
   * These are called on the add-on's thread. They publish to the achievement
   * runtime and post notifications; they must not block.
   */
  //@{
  void OnGameLoaded(const game_rc_game_loaded& data);
  void OnAchievementTriggered(const game_rc_achievement_triggered& data);
  static void OnAchievementTriggered(const game_rc_achievement_triggered& data,
                                     CAchievementRuntime& runtime,
                                     bool encoreModeEnabled);
  void OnGameCompleted(const std::string& title, bool hardcore);
  void OnRichPresenceUpdated(const std::string& evaluation);
  void OnLoginResult(const game_rc_login_result& data);
  void OnAchievementProgress(const game_rc_achievement_progress* progress, unsigned int count);
  void OnServerError(const std::string& message, const std::string& api);
  void OnConnectionChanged(bool connected);

  void OnChallengeIndicator(const game_rc_achievement_challenge& data, bool show);

  void OnAchievementProgressIndicator(const game_rc_achievement_progress_indicator& data,
                                      bool show);

  void OnLeaderboardStarted(const game_rc_leaderboard& data);

  void OnLeaderboardFailed(const game_rc_leaderboard& data);

  void OnLeaderboardSubmitted(const game_rc_leaderboard& data);

  void OnLeaderboardTracker(const game_rc_leaderboard_tracker& data, bool show);

  void OnLeaderboardScoreboard(const game_rc_leaderboard_scoreboard& data);

  void OnReset();

  void OnSubsetCompleted(const std::string& title);
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

  /*!
   * \brief Watch the achievement modes for as long as the client is loaded
   *
   * Called from CGameClient, which holds no lock at that point. It must stay
   * that way: a settings change reaches Notify() with the observer list held,
   * and Notify() then takes the client lock, so registering with the client
   * lock already held would be the other order.
   *
   * Not done when the subsystem is built either - the game services are still
   * being constructed at that point, and the settings live there.
   */
  void ObserveSettings();

  /*!
   * \brief Stop watching, for a client that outlives the settings
   */
  void StopObservingSettings();

private:
  /*!
   * \brief Give the client the RetroAchievements account to sign in with
   *
   * The account is held by Kodi, which owns the settings it is entered in.
   */
  bool SetRetroAchievementsCredentials(const std::string& username, const std::string& token);

  /*!
   * \brief Tell the client which mode achievements are being earned in
   *
   * Applied as it arrives rather than at the next load: the client resets the
   * game when hardcore is switched on, because RetroAchievements does not
   * allow a session begun in casual mode to carry on into hardcore.
   */
  bool SetHardcoreEnabled(bool enabled);

  /*!
   * \brief Turn hardcore off after the client wouldn't take it
   *
   * The client plays on in casual mode, so Kodi stops enforcing hardcore and
   * stops saying it is on.
   */
  void DropHardcore();

  /*!
   * \brief Tell the client that earned achievements are armed again
   *
   * Read when a game loads, so a client told once forgets by the next one.
   */
  bool SetEncoreModeEnabled(bool enabled);

  CGameClient& m_gameClient;
  AddonInstance_Game& m_struct;
  CCriticalSection& m_clientAccess;

  //! Whether the add-on accepted encore before the current game loaded; a
  //! setting change applies to the next one. Atomic because the achievement
  //! callbacks read it on the add-on's thread while loading and closing a game
  //! write it on Kodi's.
  std::atomic<bool> m_encoreModeEnabled{false};

  //! Hardcore as last applied to this client. Atomic because the setting can
  //! change on the player's thread as well as Kodi's.
  std::atomic<bool> m_hardcoreEnabled{false};

  //! Whether this instance is registered with the game settings. Only touched
  //! on Kodi's thread, either side of a game.
  bool m_observingSettings{false};
};
} // namespace GAME
} // namespace KODI
