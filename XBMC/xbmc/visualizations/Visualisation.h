// Visualisation.h: interface for the CVisualisation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
#define AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "../../guilib/Key.h"
#include "DllVisualisation.h"

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
                    VIS_ACTION_RATE_PRESET_MINUS,
                    VIS_ACTION_UPDATE_ALBUMART};
  CVisualisation(struct Visualisation* pVisz, DllVisualisation* pDll, const CStdString& strVisualisationName);
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
  auto_ptr<DllVisualisation> m_pDll;
  CStdString m_strVisualisationName;
};


#endif // !defined(AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
