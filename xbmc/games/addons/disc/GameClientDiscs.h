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
  bool SupportsDiskControl() const;

  // Disc interface
  void Initialize(const std::string& gamePath);
  void RestoreDiscList();
  void RefreshDiscState();
  const CGameClientDiscModel& GetDiscs() const { return *m_discModel; }
  bool IsEjected() const { return m_isEjected; }
  bool SetEjected(bool ejected);
  bool AddDisc(const std::string& filePath);
  bool RemoveDisc(const std::string& filePath);
  bool RemoveDiscByIndex(size_t index);
  bool InsertDisc(const std::string& filePath);
  bool InsertDiscByIndex(size_t index);

private:
  void LoadModelFromCore(CGameClientDiscModel& model) const;
  void SaveDiscState() const;

  static void PruneRemovedDiscs(CGameClientDiscModel& model);

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
