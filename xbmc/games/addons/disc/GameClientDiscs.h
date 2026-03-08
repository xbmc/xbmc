/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/GameClientSubsystem.h"

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <set>
#include <string>

struct AddonInstance_Game;
class CCriticalSection;

namespace KODI
{
namespace GAME
{

class CGameClient;
class CGameClientDiscM3U;
class CGameClientDiscModel;
class CGameClientDiscTransport;
class CGameClientDiscXML;

/*!
 * \ingroup games
 *
 * \brief Subsystem for the Disc Control Interface
 *
 * The disc control interface provides add-ons with the ability to manage
 * multiple discs, including disc swapping and ejection.
 *
 * For disc operation that return a bool, the following convention is used:
 *
 *   - False: Internal core failure. The user should be notified with a
 *            dialog.
 *   - True:  No internal core failure. Even if an error occurred, the UI
 *            should quietly recover.
 *
 * The intention is to reduce friction by keeping the UI quiet when possible,
 * while still surfacing errors when the add-on explicitly fails to perform an
 * operation.
 */
class CGameClientDiscs : protected CGameClientSubsystem
{
public:
  CGameClientDiscs(CGameClient& gameClient,
                   AddonInstance_Game& addonStruct,
                   CCriticalSection& clientAccess);
  ~CGameClientDiscs() override;

  // Game client capabilities
  bool SupportsDiscControl() const;

  // Lifecycle interface
  void Initialize(const std::string& gamePath);
  void Deinitialize();

  // Disc interface
  void RestoreDiscList();
  void RefreshDiscState();
  const CGameClientDiscModel& GetDiscs() const { return *m_discModel; }
  bool IsEjected() const { return m_isEjected; }
  std::string GetDiscLabel() const;
  bool IsTrayEmpty() const;
  bool SetEjected(bool ejected);
  bool AddDisc(const std::string& filePath);
  bool RemoveDisc(const std::string& filePath);
  bool RemoveDiscByIndex(size_t index);
  bool InsertDisc(const std::string& filePath);
  bool InsertDiscByIndex(size_t index);

private:
  /*!
   * \brief Clear in-memory disc state that is scoped to a single running
   * game session
   *
   * Persisted disc state is keyed by game path and remains on disk. This
   * reset is only for the live frontend model so state from a previous game
   * cannot leak into a new one.
   */
  void ResetSessionState();

  void LoadModelFromCore(CGameClientDiscModel& model) const;
  void SaveDiscState() const;

  static void PruneRemovedDiscs(CGameClientDiscModel& model);
  static void PruneExtensions(CGameClientDiscModel& model,
                              const std::set<std::string>& supportedExtensions);

  // Add-on parameters
  std::unique_ptr<CGameClientDiscTransport> m_transport;
  std::unique_ptr<CGameClientDiscXML> m_discXml;
  std::unique_ptr<CGameClientDiscM3U> m_discM3u;

  // Game parameters
  std::unique_ptr<CGameClientDiscModel> m_discModel;
  std::atomic<bool> m_isEjected{false};
};
} // namespace GAME
} // namespace KODI
