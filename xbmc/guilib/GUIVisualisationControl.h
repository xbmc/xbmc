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
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/Visualization.h"
#include "addons/AddonDll.h"
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
  void Set(const float* psBuffer, int iSize);
private:
  CAudioBuffer();
  float* m_pBuffer;
  int m_iLen;
};

class CGUIVisualisationControl : public CGUIControl, public ADDON::IAddonInstanceHandler, public IAudioCallback
{
public:
  CGUIVisualisationControl(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIVisualisationControl(const CGUIVisualisationControl &from);
  virtual CGUIVisualisationControl *Clone() const { return new CGUIVisualisationControl(*this); }; //! @todo check for naughties

  // Child functions related to IAudioCallback
  virtual void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample);
  virtual void OnAudioData(const float* pAudioData, int iAudioDataLength);

  // Child functions related to CGUIControl
  virtual void FreeResources(bool immediately = false);
  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual bool IsDirty();
  virtual void Render();
  virtual void UpdateVisibility(const CGUIListItem *item = nullptr);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool CanFocus() const { return false; }
  virtual bool CanFocusFromPoint(const CPoint &point) const;

  std::string Name();
  void UpdateTrack();
  bool HasPresets() { return !m_presets.empty(); };
  void SetPreset(int idx);
  bool IsLocked();
  unsigned int GetPreset();
  std::string GetPresetName();
  bool GetPresetList(std::vector<std::string>& vecpresets);

private:
  bool InitVisualization();
  void DeInitVisualization();
  inline void GetPresets();
  inline void CreateBuffers();
  inline void ClearBuffers();

  // Static function to transfer data from add-on to kodi
  static void transfer_preset(void* kodiInstance, const char* preset);

  bool m_callStart;
  bool m_alreadyStarted;
  bool m_attemptedLoad;
  bool m_updateTrack;

  std::list<CAudioBuffer*> m_vecBuffers;
  int m_numBuffers; /*!< Number of Audio buffers */
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

  ADDON::AddonDllPtr m_addon;
  kodi::addon::CInstanceVisualization* m_addonInstance;
  AddonInstance_Visualization m_struct;
};
