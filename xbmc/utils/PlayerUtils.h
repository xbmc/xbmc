/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

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
  static void AdvanceTempoStep(std::shared_ptr<CApplicationPlayer> appPlayer,
                               TempoStepChange change);
};
