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

namespace ADDON
{

class CVisualization : public IAddonInstanceHandler
{
public:
  CVisualization(const AddonInfoPtr& addonInfo, float x, float y, float w, float h);
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
  bool UpdateTrack(const VIS_TRACK* track);
  bool HasPresets();
  bool GetPresetList(std::vector<std::string>& vecpresets);
  int GetActivePreset();
  std::string GetActivePresetName();
  bool IsLocked();

  // Addon callback functions
  void TransferPreset(const std::string& preset);
  void ClearPresets();

private:
  std::vector<std::string> m_presets; /*!< cached preset list */
};

} // namespace ADDON
