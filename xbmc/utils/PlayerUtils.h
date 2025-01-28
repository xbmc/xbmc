/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

class CApplicationPlayer;
class CFileItem;

enum class TempoStepChange
{
  INCREASE,
  DECREASE,
};

class CPlayerUtils
{
public:
  static bool IsItemPlayable(const CFileItem& item);
  static void AdvanceTempoStep(const std::shared_ptr<CApplicationPlayer>& appPlayer,
                               TempoStepChange change);

  /*!
   \brief Get the players available for the given file item.
   \param item The item
   \return the players
   */
  static std::vector<std::string> GetPlayersForItem(const CFileItem& item);

  /*!
   \brief Check whether multiple players are available for the given item.
   \param item The ite.
   \return True if multiple players are available, false otherwise
   */
  static bool HasItemMultiplePlayers(const CFileItem& item);
};
