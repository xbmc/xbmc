/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/Visualization.h"

namespace KODI
{
namespace ADDONS
{

class CVisualization : public ADDON::IAddonInstanceHandler
{
public:
  CVisualization(const ADDON::AddonInfoPtr& addonInfo, float x, float y, float w, float h);
  ~CVisualization() override;

  bool Start(int channels, int samplesPerSec, int bitsPerSample, const std::string& songName) const;
  void Stop() const;
  void AudioData(const float* audioData, int audioDataLength) const;
  bool IsDirty() const;
  void Render() const;
  int GetSyncDelay() const;
  bool NextPreset() const;
  bool PrevPreset() const;
  bool LoadPreset(int select) const;
  bool RandomPreset() const;
  bool LockPreset() const;
  bool RatePreset(bool plus_minus) const;
  bool UpdateAlbumart(const char* albumart) const;
  bool UpdateTrack(const KODI_ADDON_VISUALIZATION_TRACK* track) const;
  bool HasPresets() const;
  bool GetPresetList(std::vector<std::string>& vecpresets) const;
  int GetActivePreset() const;
  std::string GetActivePresetName();
  bool IsLocked() const;

  // Addon callback functions
  void GetProperties(struct KODI_ADDON_VISUALIZATION_PROPS* props) const;
  void TransferPreset(const std::string& preset);
  void ClearPresets();

private:
  const int m_x;
  const int m_y;
  const int m_width;
  const int m_height;
  std::vector<std::string> m_presets; /*!< cached preset list */
};

} // namespace ADDONS
} // namespace KODI
