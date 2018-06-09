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

#include "cores/GameSettings.h"
#include "utils/Observer.h"

class CGameSettings : public Observable
{
public:
  CGameSettings() { Reset(); }
  CGameSettings(const CGameSettings &other) { *this = other; }

  CGameSettings &operator=(const CGameSettings &rhs);

  // Restore game settings to default
  void Reset();

  bool operator==(const CGameSettings &rhs) const;
  bool operator!=(const CGameSettings &rhs) const { return !(*this == rhs); }

  KODI::RETRO::SCALINGMETHOD ScalingMethod() const { return m_scalingMethod; }
  void SetScalingMethod(KODI::RETRO::SCALINGMETHOD scalingMethod);

  KODI::RETRO::VIEWMODE ViewMode() const { return m_viewMode; }
  void SetViewMode(KODI::RETRO::VIEWMODE viewMode);

  unsigned int RotationDegCCW() const { return m_rotationDegCCW; }
  void SetRotationDegCCW(unsigned int rotation);

private:
  // Video settings
  KODI::RETRO::SCALINGMETHOD m_scalingMethod;
  KODI::RETRO::VIEWMODE m_viewMode;
  unsigned int m_rotationDegCCW;
};
