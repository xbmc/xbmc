/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "games/addons/disc/GameClientDiscModel.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CDiscManagerGame
{
public:
  CDiscManagerGame() = default;
  ~CDiscManagerGame() = default;

  // Lifecycle functions
  void Initialize(GameClientPtr gameClient);
  void Deinitialize();

  // Game interface
  bool IsEjected() const;
  void GetState(bool& ejected, std::string& selectedDisc) const;
  unsigned int GetSelectedIndex(std::optional<size_t> selectedIndex, bool allowSelectNoDisc) const;
  std::optional<std::string> GetDiscPathByIndex(std::optional<size_t> discIndex) const;

  // Static game interface
  static GameClientPtr GetGameClient();

private:
  // Game parameters
  GameClientPtr m_gameClient;
  CGameClientDiscModel m_initialDiscModel;
};
} // namespace GAME
} // namespace KODI
