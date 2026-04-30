/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/disc/GameClientDiscModel.h"

namespace KODI
{
namespace GAME
{

class CGameClientDiscMergeUtils
{
public:
  /*!
   * \brief Reconcile frontend-persisted disc metadata with the current core
   * snapshot
   *
   * Why this exists:
   *
   *   - The frontend model carries user intent (removed-slot tombstones and
   *     prior selection) across refreshes
   *   - The core model carries live emulator truth (real discs, tray state,
   *     and current core-selected disc)
   *
   * Index-only replacement is insufficient because some cores compact indices
   * after removal while others keep "zombie" non-disc slots. Reconciliation
   * preserves frontend tombstones where the core has no real disc, but allows
   * real core discs to override tombstones.
   *
   * Selection is preserved by disc path when possible, and tray state always
   * follows the core.
   */
  static CGameClientDiscModel ReconcileModels(const CGameClientDiscModel& frontendDiscs,
                                              const CGameClientDiscModel& coreDiscs);
};

} // namespace GAME
} // namespace KODI
