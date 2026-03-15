/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"

#include <cstddef>
#include <optional>
#include <string>

namespace KODI
{
namespace GAME
{
class CGameClientDiscModel;

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

  // Static game interface
  static GameClientPtr GetGameClient();

private:
  // Game parameters
  GameClientPtr m_gameClient;
};
} // namespace GAME
} // namespace KODI
