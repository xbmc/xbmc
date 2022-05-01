/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CAction;
class CApplicationPlayer;

/*!
 * \brief Class handling application support for audio volume management.
 */
class CApplicationVolumeHandling
{
public:
  explicit CApplicationVolumeHandling(CApplicationPlayer& appPlayer);

  float GetVolumePercent() const;
  float GetVolumeRatio() const;
  bool IsMuted() const;

  void SetVolume(float iValue, bool isPercentage = true);
  void SetMute(bool mute);
  void ToggleMute(void);

  static constexpr float VOLUME_MINIMUM = 0.0f; // -60dB
  static constexpr float VOLUME_MAXIMUM = 1.0f; // 0dB
  static constexpr float VOLUME_DYNAMIC_RANGE = 90.0f; // 60dB

protected:
  bool IsMutedInternal() const { return m_muted; }
  void ShowVolumeBar(const CAction* action = nullptr);

  void Mute();
  void UnMute();

  void SetHardwareVolume(float hardwareVolume);

  void VolumeChanged();

  CApplicationPlayer& m_appPlayer; //!< Reference to application player
  bool m_muted = false;
  float m_volumeLevel = VOLUME_MAXIMUM;
};
