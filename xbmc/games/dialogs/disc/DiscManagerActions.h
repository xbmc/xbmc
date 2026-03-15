/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"

#include <string>

namespace KODI
{
namespace GAME
{
class CDialogGameDiscManager;

/*!
 * \ingroup games
 */
class CDiscManagerActions
{
public:
  explicit CDiscManagerActions(CDialogGameDiscManager& discManager);
  ~CDiscManagerActions() = default;

  // Lifecycle functions
  void Initialize(GameClientPtr gameClient);
  void Deinitialize();

  // Commands
  void OnSelectDisc();
  void OnEjectInsert();
  void OnAdd();
  void OnRemove();
  void OnApplyDiscChange();
  void OnResumeGame();

private:
  // Helper functions
  bool BrowseForDiscImage(const std::string& startingPath, std::string& filePath);
  void ShowInternalError();

  // Construction parameters
  CDialogGameDiscManager& m_discManager;

  // Game parameters
  GameClientPtr m_gameClient;
};
} // namespace GAME
} // namespace KODI
