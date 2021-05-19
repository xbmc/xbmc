/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/Setting.h"

#include <string>
#include <utility>
#include <vector>

struct IntegerSettingOption;

class CViewModeSettings
{
private:
  CViewModeSettings();
  ~CViewModeSettings() = default;

public:
  /** Gets the next view mode for quick cycling through the modes
   *
   * @param viewMode The current view mode
   * @return The next view mode
   */
  static int GetNextQuickCycleViewMode(int viewMode);

  /** Gets the string index for the view mode
   *
   * @param viewMode The current view mode
   * @return The string index
   */
  static int GetViewModeStringIndex(int viewMode);

  /** Fills the list with all visible view modes
   */
  static void ViewModesFiller(const std::shared_ptr<const CSetting>& setting,
                              std::vector<IntegerSettingOption>& list,
                              int& current,
                              void* data);
};
