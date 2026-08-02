/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RConsoleIDs.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace KODI
{
namespace GAME
{
class CGameClient;
}

namespace RETRO
{
class CCheevos
{
public:
  ~CCheevos();
  CCheevos(GAME::CGameClient* gameClient,
           const std::string& userName,
           const std::string& loginToken);

  /*!
   * \brief Stop background work before the game client is closed
   */
  void Stop();

  /*!
   * \brief Perform the actual HTTP login exchange with RetroAchievements
   *
   * Call this with the user's PASSWORD when they press the Login button.
   * On success the returned token is stored internally and persisted to
   * settings — the password is never stored.
   *
   * Uses the r=login2 endpoint with credentials in the HTTPS POST body:
   *
   *   - https://api-docs.retroachievements.org/connect/standalone.html
   *
   * \param password  The user's account password (not a token)
   *
   * \return true on successful login.
   */
  bool RCLogin(const std::string& password);

  /*!
   * \brief Reset the runtime
   */
  void ResetRuntime();

  /*!
   * \brief Fetch achievement patch data for the loaded game
   */
  bool LoadData();

  /*!
   * \brief Enable rich presence
   */
  void EnableRichPresence();

  std::string GetRichPresenceEvaluation();

  /*!
   * \brief Achievement activation and trigger detection
   */
  void ActivateAchievement();

  void CallbackUrlId(const std::string& achievementUrl, unsigned int cheevoId);

private:
  using ActivatedCheevoMap = std::unordered_map<unsigned, std::vector<std::string>>;
  using CheevoTitleMap = std::unordered_map<unsigned, std::pair<std::string, std::string>>;

  /*!
   * \brief Rich presence periodic ping thread
   */
  void RichPresencePingThread();

  // Helper functions
  void DownloadThread();
  bool QueueDownload(std::function<void()> task);
  RConsoleID ConsoleID();

  const std::string RA_USER_AGENT;

  // Construction parameters
  GAME::CGameClient* m_gameClient;
  std::string m_userName;
  std::string m_loginToken;

  bool m_richPresenceLoaded{false};
  std::string m_richPresenceScript;

  // Published as a complete snapshot after loading and copied by readers
  std::mutex m_activatedCheevoMutex;
  ActivatedCheevoMap m_activatedCheevoMap;
  std::string m_gameTitle;
  unsigned int m_gameId{0};

  // So CallbackUrlId can look up titles
  std::mutex m_cheevoTitlesMutex;
  CheevoTitleMap m_cheevoTitles;

  // Set true when RA flags this emulator as unsupported
  bool m_unsupportedEmulator{false};

  // Persistent achievement callback — must outlive game session, so stored as member
  std::function<void(const std::string&, unsigned int)> m_cheevoCallback;

  // Rich presence periodic ping
  std::atomic<bool> m_richPresenceRunning{false};
  std::thread m_richPresenceThread;

  // Bounded background image download queue.
  std::mutex m_downloadThreadsMutex;
  std::condition_variable m_downloadCondition;
  std::deque<std::function<void()>> m_downloadQueue;
  bool m_downloadRunning{true};
  std::thread m_downloadThread;
};
} // namespace RETRO
} // namespace KODI
