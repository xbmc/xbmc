#pragma once
#include "../DynamicDll.h"

struct VIS_INFO
{
  bool bWantsFreq;
  int iSyncDelay;
  //  int iAudioDataLength;
  //  int iFreqDataLength;
};

// The VisSetting class for GUI settings for vis.
class VisSetting
{
public:
  enum SETTING_TYPE { NONE=0, CHECK, SPIN };
  VisSetting()
  {
    name = NULL;
    current = 0;
    type = NONE;
  };
  ~VisSetting()
  {
    if (name)
      delete[] name;
    name = NULL;
  }
  SETTING_TYPE type;
  char *name;
  int  current;
  std::vector<const char *> entry;
};

struct Visualisation
{
public:
#ifndef HAS_SDL
  void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisation, float pixelRatio);
#else // TODO LINUX this obviously doesn't work but it will have to do for now
  void (__cdecl* Create)(void* pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisation, float pixelRatio);
#endif
  void (__cdecl* Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
  void (__cdecl* AudioData)(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
  void (__cdecl* Render) ();
  void (__cdecl* Stop)();
  void (__cdecl* GetInfo)(VIS_INFO *info);
  bool (__cdecl* OnAction)(long flags, void *param);
  void (__cdecl *GetSettings)(std::vector<VisSetting> **vecSettings);
  void (__cdecl *UpdateSetting)(int num);
  void (__cdecl *GetPresets)(char ***pPresets, int *currentPreset, int *numPresets, bool *locked);
} ;

class DllVisualisationInterface
{
public:
  void GetModule(struct Visualisation* pScr);
};

class DllVisualisation : public DllDynamic, DllVisualisationInterface
{
  DECLARE_DLL_WRAPPER_TEMPLATE(DllVisualisation)
  DEFINE_METHOD1(void, GetModule, (struct Visualisation* p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(get_module,GetModule)
  END_METHOD_RESOLVE()
};
