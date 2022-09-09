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

  bool Start(int channels, int samplesPerSec, int bitsPerSample, const std::string& songName);
  void Stop();
  void AudioData(const float* audioData, int audioDataLength);
  bool IsDirty();
  void Render();
  int GetSyncDelay();
  bool NextPreset();
  bool PrevPreset();
  bool LoadPreset(int select);
  bool RandomPreset();
  bool LockPreset();
  bool RatePreset(bool plus_minus);
  bool UpdateAlbumart(const char* albumart);
  bool UpdateTrack(const KODI_ADDON_VISUALIZATION_TRACK* track);
  bool HasPresets();
  bool GetPresetList(std::vector<std::string>& vecpresets);
  int GetActivePreset();
  std::string GetActivePresetName();
  bool IsLocked();

  // Addon callback functions
  void GetProperties(struct KODI_ADDON_VISUALIZATION_PROPS* props);
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
