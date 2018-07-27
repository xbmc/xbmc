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
class DPMSSupport
{
public:
  // All known DPMS power-saving modes, on any platform.
  enum PowerSavingMode
  {
    STANDBY, SUSPEND, OFF,
    NUM_MODES
  };

  // Initializes an instance tied to the specified Surface. The Surface object
  // must be alive for as long as this instance is in use.
  DPMSSupport();

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

  // Untranslated name of the mode, for logging.
  static const char* GetModeName(PowerSavingMode mode);
  // Returns true if the given mode is valid (a member of PowerSavingMode).
  // Returns false and logs an error message otherwise.
  static bool CheckValidMode(PowerSavingMode mode);

  // Turns on the specified power-saving mode, which must be valid
  // and supported. Returns false if this failed.
  bool EnablePowerSaving(PowerSavingMode mode);
  // Turns off power-saving mode. You should only call this if the display
  // is currently in a power-saving mode, to avoid visual artifacts.
  bool DisablePowerSaving();

private:
  std::vector<PowerSavingMode> m_supportedModes;

  // Platform-specific code: add new #ifdef'ed implementations in the .cc file.
  //
  // Initializes DPMS support. Should populate m_supportedModes with the
  // preferred order of the modes, if supported, otherwise leave it empty.
  // If the latter, it should log a message about exactly why DPMS is not
  // available.
  void PlatformSpecificInit();
  // Should turn on power-saving on the current platform. The mode is
  // guaranteed to be one of m_supportedModes. Should return false on failure,
  // and log a (platform-specific) ERROR message.
  bool PlatformSpecificEnablePowerSaving(PowerSavingMode mode);
  // Should turn off power-saving on the current platform. Should return
  // false on failure and log a (platform-specific) ERROR message.
  bool PlatformSpecificDisablePowerSaving();
};

