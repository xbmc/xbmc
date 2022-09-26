/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIControl.h"
#include "cores/AudioEngine/Interfaces/IAudioCallback.h"

#include <list>
#include <string>
#include <vector>

namespace KODI
{
namespace ADDONS
{
class CVisualization;
} // namespace ADDONS
} // namespace KODI

class CAudioBuffer
{
public:
  explicit CAudioBuffer(int iSize);
  virtual ~CAudioBuffer();
  const float* Get() const;
  int Size() const;
  void Set(const float* psBuffer, int iSize);

private:
  CAudioBuffer(const CAudioBuffer&) = delete;
  CAudioBuffer& operator=(const CAudioBuffer&) = delete;
  CAudioBuffer();
  float* m_pBuffer;
  int m_iLen;
};

class CGUIVisualisationControl : public CGUIControl, public IAudioCallback
{
public:
  CGUIVisualisationControl(
      int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIVisualisationControl(const CGUIVisualisationControl& from);
  CGUIVisualisationControl* Clone() const override
  {
    return new CGUIVisualisationControl(*this);
  }; //! @todo check for naughties

  // Child functions related to IAudioCallback
  void OnInitialize(int channels, int samplesPerSec, int bitsPerSample) override;
  void OnAudioData(const float* audioData, unsigned int audioDataLength) override;

  // Child functions related to CGUIControl
  void FreeResources(bool immediately = false) override;
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  void Render() override;
  void UpdateVisibility(const CGUIListItem* item = nullptr) override;
  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;
  bool CanFocus() const override { return false; }
  bool CanFocusFromPoint(const CPoint& point) const override;

  std::string Name();
  void UpdateTrack();
  bool HasPresets();
  void SetPreset(int idx);
  bool IsLocked();
  int GetActivePreset();
  std::string GetActivePresetName();
  bool GetPresetList(std::vector<std::string>& vecpresets);

private:
  bool InitVisualization();
  void DeInitVisualization();
  inline void CreateBuffers();
  inline void ClearBuffers();

  bool m_callStart{false};
  bool m_alreadyStarted{false};
  bool m_attemptedLoad{false};
  bool m_updateTrack{false};

  std::list<std::unique_ptr<CAudioBuffer>> m_vecBuffers;
  unsigned int m_numBuffers; /*!< Number of Audio buffers */
  std::vector<std::string> m_presets; /*!< cached preset list */

  /* values set from "OnInitialize" IAudioCallback  */
  int m_channels;
  int m_samplesPerSec;
  int m_bitsPerSample;

  std::string m_albumThumb; /*!< track information */
  std::string m_name; /*!< To add-on sended name */
  std::string m_presetsPath; /*!< To add-on sended preset path */
  std::string m_profilePath; /*!< To add-on sended profile path */

  std::unique_ptr<KODI::ADDONS::CVisualization> m_instance;
};
