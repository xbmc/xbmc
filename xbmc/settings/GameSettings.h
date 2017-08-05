/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "cores/IPlayer.h"

class CGameSettings
{
public:
  CGameSettings() { Reset(); }

  // Restore game settings to default
  void Reset();

  bool operator==(const CGameSettings &rhs) const;
  bool operator!=(const CGameSettings &rhs) const { return !(*this == rhs); }

  ESCALINGMETHOD ScalingMethod() const { return m_scalingMethod; }
  void SetScalingMethod(ESCALINGMETHOD scalingMethod) { m_scalingMethod = scalingMethod; }
  
  enum ViewMode ViewMode() const { return m_viewMode; }
  void SetViewMode(enum ViewMode viewMode) { m_viewMode = viewMode; }

private:
  // Video settings
  ESCALINGMETHOD m_scalingMethod;
  enum ViewMode m_viewMode;
};
