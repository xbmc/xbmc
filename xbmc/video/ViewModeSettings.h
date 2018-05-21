
#pragma once

/*
 *      Copyright (C) 2016-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <utility>
#include <vector>

#include "settings/lib/Setting.h"

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
  static void ViewModesFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

};
