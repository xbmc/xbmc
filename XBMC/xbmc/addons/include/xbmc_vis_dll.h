#ifndef __XBMC_VIS_H__
#define __XBMC_VIS_H__

#ifdef HAS_XBOX_HARDWARE
    #include <xtl.h>
#elif _LINUX
#define __cdecl
#define __declspec(x)
#elif __APPLE__
#define __cdecl
#define __declspec(x)
#else
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "xbmc_addon_dll.h"               /* Dll related functions available to all AddOn's */
#include "xbmc_vis_types.h"

extern "C"
{
  // the action commands ( see Visualisation.h )
  #define VIS_ACTION_NEXT_PRESET       1
  #define VIS_ACTION_PREV_PRESET       2
  #define VIS_ACTION_LOAD_PRESET       3
  #define VIS_ACTION_RANDOM_PRESET     4
  #define VIS_ACTION_LOCK_PRESET       5
  #define VIS_ACTION_RATE_PRESET_PLUS  6
  #define VIS_ACTION_RATE_PRESET_MINUS 7
  #define VIS_ACTION_UPDATE_ALBUMART   8
  #define VIS_ACTION_UPDATE_TRACK      9

  #define VIS_ACTION_USER 100

  // Functions that your visualisation must implement
  void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
  void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
  void Render();
  void Stop();
  bool OnAction(long action, void *param);
  void GetInfo(VIS_INFO* pInfo);
  void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct Visualisation* pVisz)
  {
    pVisz->Start = Start;
    pVisz->AudioData = AudioData;
    pVisz->Render = Render;
    pVisz->Stop = Stop;
    pVisz->OnAction = OnAction;
    pVisz->GetInfo = GetInfo;
    pVisz->GetPresets = GetPresets;
  };
};

#endif
