/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"
#include "utils/Observer.h"

#include <string>

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

  const std::string &VideoFilter() const { return m_videoFilter; }
  void SetVideoFilter(const std::string &videoFilter);

  KODI::RETRO::STRETCHMODE StretchMode() const { return m_stretchMode; }
  void SetStretchMode(KODI::RETRO::STRETCHMODE stretchMode);

  unsigned int RotationDegCCW() const { return m_rotationDegCCW; }
  void SetRotationDegCCW(unsigned int rotation);

private:
  // Video settings
  std::string m_videoFilter;
  KODI::RETRO::STRETCHMODE m_stretchMode;
  unsigned int m_rotationDegCCW;
};
