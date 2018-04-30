/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>

// This class encapsulates support for monitor power-saving features (DPMS).
// An instance is connected to a Surface, provides information on which
// power-saving features are available for that screen, and it is able to
// turn power-saving on an off.
// Note that SDL turns off DPMS timeouts at the beginning of the application.
class CDPMSSupport
{
public:
  // All known DPMS power-saving modes, on any platform.
  enum PowerSavingMode
  {
    STANDBY,
    SUSPEND,
    OFF,
    NUM_MODES,
  };

  CDPMSSupport();
  virtual ~CDPMSSupport() = default;

  // Whether power-saving is supported on this screen.
  bool IsSupported() const { return !m_supportedModes.empty(); }

  // Which power-saving modes are supported, in the order of preference (i.e.
  // the first mode should be the best choice).
  const std::vector<PowerSavingMode>& GetSupportedModes() const
  {
    return m_supportedModes;
  }

  // Whether a given mode is supported.
  bool IsModeSupported(PowerSavingMode mode) const;

  // Turns on the specified power-saving mode, which must be valid
  // and supported. Returns false if this failed.
  virtual bool EnablePowerSaving(PowerSavingMode mode) = 0;

  // Turns off power-saving mode. You should only call this if the display
  // is currently in a power-saving mode, to avoid visual artifacts.
  virtual bool DisablePowerSaving() = 0;

protected:
  std::vector<PowerSavingMode> m_supportedModes;
};
