#ifndef __XBMC_VIS_H__
#define __XBMC_VIS_H__

#include "xbmc_addon_dll.h"
#include "xbmc_vis_types.h"

extern "C"
{
  // Functions that your visualisation must implement
  void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
  void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
  void Render();
  bool OnAction(long action, const void *param);
  void GetInfo(VIS_INFO* pInfo);
  unsigned int GetPresets(char ***presets);
  unsigned GetPreset();
  unsigned int GetSubModules(char ***presets);
  bool IsLocked();

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct Visualisation* pVisz)
  {
    pVisz->Start = Start;
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

#endif
