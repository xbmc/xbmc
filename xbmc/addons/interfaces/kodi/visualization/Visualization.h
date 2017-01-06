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
#pragma once

#include "addons/AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/visualization/Visualization.h"
#include "cores/AudioEngine/Interfaces/IAudioCallback.h"
#include "guilib/IRenderingCallback.h"
#include "utils/rfft.h"

#include <algorithm>
#include <map>
#include <list>
#include <memory>
#include <vector>

#define AUDIO_BUFFER_SIZE 512 // MUST BE A POWER OF 2!!!
#define MAX_AUDIO_BUFFERS 16

class CCriticalSection;

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

namespace ADDON
{
  class CVisualisation : public ADDON::CAddonInstanceInfo
                       , public IAudioCallback
                       , public IRenderingCallback
  {
  public:
    explicit CVisualisation(ADDON::AddonDllPtr addon);

    // Child functions related to IAudioCallback
    virtual void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample);
    virtual void OnAudioData(const float* pAudioData, int iAudioDataLength);

    // Child functions related to IRenderingCallback
    virtual bool Create(int x, int y, int w, int h, void *device) override;
    virtual void Render() override;
    virtual void Stop() override;

    void AudioData(const float *pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
    bool OnAction(VIS_ACTION action, void *param = nullptr);
    bool UpdateTrack();
    bool HasPresets() { return m_hasPresets; };
    bool IsLocked();
    unsigned int GetPreset();
    std::string GetPresetName();
    bool GetPresetList(std::vector<std::string>& vecpresets);

    // Static function to transfer data from add-on to kodi
    static void transfer_preset(void* kodiInstance, const char* preset);

  private:
    void CreateBuffers();
    void ClearBuffers();

    bool GetPresets();

    // cached preset list
    std::vector<std::string> m_presets;

    // audio properties
    int m_iChannels;
    int m_iSamplesPerSec;
    int m_iBitsPerSample;
    std::list<CAudioBuffer*> m_vecBuffers;
    int m_iNumBuffers;        // Number of Audio buffers
    bool m_bWantsFreq;
    float m_fFreq[AUDIO_BUFFER_SIZE];         // Frequency data
    bool m_hasPresets;
    std::unique_ptr<RFFT> m_transform;

    // track information
    std::string m_AlbumThumb;

    std::string m_name; /*!< To add-on sended name */
    std::string m_presetsPath; /*!< To add-on sended preset path */
    std::string m_profilePath; /*!< To add-on sended profile path */

    kodi::addon::CInstanceVisualization* m_addonInstance;
    AddonInstance_Visualization m_struct;
    bool m_active;
  };
}
