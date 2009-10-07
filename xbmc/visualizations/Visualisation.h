/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "Key.h"
#include "DllVisualisation.h"
#include "addons/include/libvisualisation.h"
#include "addons/include/xbmc_vis_types.h"
#include "AddonDll.h"
#include "cores/IAudioCallback.h"

#include <map>
#include <memory>

#define AUDIO_BUFFER_SIZE 512 // MUST BE A POWER OF 2!!!
#define MAX_AUDIO_BUFFERS 16

class CCriticalSection;

class CAudioBuffer
{
public:
  CAudioBuffer(int iSize);
  virtual ~CAudioBuffer();
  const short* Get() const;
  void Set(const unsigned char* psBuffer, int iSize, int iBitsPerSample);
private:
  CAudioBuffer();
  short* m_pBuffer;
  int m_iLen;
};

class CVisualisation : public ADDON::CAddonDll<DllVisualisation, Visualisation, VIS_PROPS>
                     , public IAudioCallback
{
public:
  ~CVisualisation();
  
  CVisualisation(const AddonProps &props) : ADDON::CAddonDll<DllVisualisation, Visualisation, VIS_PROPS>(props) {}
  virtual ~CVisualisation() {};
  virtual void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample);
  virtual void OnAudioData(const unsigned char* pAudioData, int iAudioDataLength);
  bool Create(int x, int y, int w, int h);
  void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const CStdString strSongName);
  void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
  void Render();
  void Stop();
  void GetInfo(VIS_INFO *info);
  bool OnAction(VIS_ACTION action, void *param = NULL);
  bool UpdateTrack();
  void GetSettings(std::vector<VisSetting> **vecSettings);
  void UpdateSetting(int num, std::vector<VisSetting> **vecSettings);
  void GetCurrentPreset(char **pPreset, bool *locked);
  int  GetSubModules(std::map<std::string, std::string>& subModules);
  bool IsLocked();
  char *GetPreset();
  bool GetPresetList(std::vector<CStdString>& vecpresets);

  // some helper functions
  static CStdString GetFriendlyName(const char* strVisz, const char* strSubModule);
  static CStdString GetFriendlyName(const char* combinedName);
  static CStdString GetCombinedName(const char* strVisz, const char* strSubModule);
  static CStdString GetCombinedName(const char* friendlyName);

  CStdString m_strVisualisationName;
  CStdString m_strSubModuleName;

private:
  void CreateBuffers();
  void ClearBuffers();

  bool GetPresets();

  // attributes of the viewport we render to
  int m_xPos;
  int m_yPos;
  int m_width;
  int m_height;

  // cached preset list
  std::vector<CStdString> m_presets;

  // audio properties
  int m_iChannels;
  int m_iSamplesPerSec;
  int m_iBitsPerSample;
  std::list<CAudioBuffer*> m_vecBuffers;
  int m_iNumBuffers;        // Number of Audio buffers
  bool m_bWantsFreq;
  float m_fFreq[2*AUDIO_BUFFER_SIZE];         // Frequency data
  bool m_bCalculate_Freq;       // True if the vis wants freq data

  // track information
  CStdString m_AlbumThumb;

  CCriticalSection m_critSection;
};

