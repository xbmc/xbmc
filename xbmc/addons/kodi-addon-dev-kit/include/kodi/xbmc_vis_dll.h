#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "xbmc_addon_dll.h"
#include "xbmc_vis_types.h"

extern "C"
{
  // Functions that your visualisation must implement
  bool Start(void* addonInstance, int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
  void Stop(void* addonInstance);
  void AudioData(void* addonInstance, const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
  void Render(void* addonInstance);
  bool OnAction(void* addonInstance, long action, const void *param);
  void GetInfo(void* addonInstance, VIS_INFO* pInfo);
  unsigned int GetPresets(void* addonInstance, char ***presets);
  unsigned GetPreset(void* addonInstance);
  unsigned int GetSubModules(void* addonInstance, char ***presets);
  bool IsLocked(void* addonInstance);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(void* ptr)
  {
    KodiToAddonFuncTable_Visualisation* pVisz = static_cast<KodiToAddonFuncTable_Visualisation*>(ptr);

    pVisz->Start = Start;
    pVisz->Stop = Stop;
    pVisz->AudioData = AudioData;
    pVisz->Render = Render;
    pVisz->OnAction = OnAction;
    pVisz->GetInfo = GetInfo;
    pVisz->GetPresets = GetPresets;
    pVisz->GetPreset = GetPreset;
    pVisz->GetSubModules = GetSubModules;
    pVisz->IsLocked = IsLocked;
  };
};

