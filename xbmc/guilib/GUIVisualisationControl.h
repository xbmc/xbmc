#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIControl.h"
#include "addons/Visualization.h"
#include "cores/AudioEngine/Interfaces/IAudioCallback.h"
#include "utils/rfft.h"

#define AUDIO_BUFFER_SIZE 512 // MUST BE A POWER OF 2!!!
#define MAX_AUDIO_BUFFERS 16

class CAudioBuffer
{
public:
  CAudioBuffer(int iSize);
  virtual ~CAudioBuffer();
  const float* Get() const;
  int Size() const;
  void Set(const float* psBuffer, int iSize);
private:
  CAudioBuffer();
  float* m_pBuffer;
  int m_iLen;
};

class CGUIVisualisationControl : public CGUIControl, public IAudioCallback
{
public:
  CGUIVisualisationControl(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIVisualisationControl(const CGUIVisualisationControl &from);
  CGUIVisualisationControl *Clone() const override { return new CGUIVisualisationControl(*this); }; //! @todo check for naughties

  // Child functions related to IAudioCallback
  void OnInitialize(int channels, int samplesPerSec, int bitsPerSample) override;
  void OnAudioData(const float* audioData, unsigned int audioDataLength) override;

  // Child functions related to CGUIControl
  void FreeResources(bool immediately = false) override;
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  virtual bool IsDirty();
  void Render() override;
  void UpdateVisibility(const CGUIListItem *item = nullptr) override;
  bool OnAction(const CAction &action) override;
  bool OnMessage(CGUIMessage &message) override;
  bool CanFocus() const override { return false; }
  bool CanFocusFromPoint(const CPoint &point) const override;

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

  bool m_callStart;
  bool m_alreadyStarted;
  bool m_attemptedLoad;
  bool m_updateTrack;

  std::list<std::unique_ptr<CAudioBuffer>> m_vecBuffers;
  unsigned int m_numBuffers; /*!< Number of Audio buffers */
  bool m_wantsFreq;
  float m_freq[AUDIO_BUFFER_SIZE]; /*!< Frequency data */
  std::vector<std::string> m_presets; /*!< cached preset list */
  std::unique_ptr<RFFT> m_transform;

  /* values set from "OnInitialize" IAudioCallback  */
  int m_channels;
  int m_samplesPerSec;
  int m_bitsPerSample;

  std::string m_albumThumb; /*!< track information */
  std::string m_name; /*!< To add-on sended name */
  std::string m_presetsPath; /*!< To add-on sended preset path */
  std::string m_profilePath; /*!< To add-on sended profile path */

  ADDON::CVisualization* m_instance;
};
