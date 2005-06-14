// Visualisation.h: interface for the CVisualisation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
#define AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "../cores/DllLoader/dll.h"
#include "../../guilib/key.h"

#ifdef __cplusplus
extern "C"
{
#endif

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
    vector<const char *> entry;
  };

  struct Visualisation
  {
public:
    void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisation, float pixelRatio);
    void (__cdecl* Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
    void (__cdecl* AudioData)(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
    void (__cdecl* Render) ();
    void (__cdecl* Stop)();
    void (__cdecl* GetInfo)(VIS_INFO *info);
    bool (__cdecl* OnAction)(long flags, void *param);
    void (__cdecl *GetSettings)(vector<VisSetting> **vecSettings);
    void (__cdecl *UpdateSetting)(int num);
    void (__cdecl *GetPresets)(char ***pPresets, int *currentPreset, int *numPresets, bool *locked);
  } ;

#ifdef __cplusplus
};
#endif

class CVisualisation
{
public:
  enum VIS_ACTION { VIS_ACTION_NONE = 0,
                    VIS_ACTION_NEXT_PRESET,
                    VIS_ACTION_PREV_PRESET,
                    VIS_ACTION_LOAD_PRESET,
                    VIS_ACTION_RANDOM_PRESET,
                    VIS_ACTION_LOCK_PRESET,
                    VIS_ACTION_RATE_PRESET_PLUS,
                    VIS_ACTION_RATE_PRESET_MINUS };
  CVisualisation(struct Visualisation* pVisz, DllLoader* pLoader, const CStdString& strVisualisationName);
  ~CVisualisation();

  void Create(int posx, int posy, int width, int height);
  void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const CStdString strSongName);
  void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
  void Render();
  void Stop();
  void GetInfo(VIS_INFO *info);
  bool OnAction(VIS_ACTION action, void *param = NULL);
  void GetSettings(vector<VisSetting> **vecSettings);
  void UpdateSetting(int num);
  void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked);
  void GetCurrentPreset(char **pPreset, bool *locked);
  bool IsLocked();
  char *GetPreset();

protected:
  auto_ptr<struct Visualisation> m_pVisz;
  auto_ptr<DllLoader> m_pLoader;
  CStdString m_strVisualisationName;
};


#endif // !defined(AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
