// Visualisation.h: interface for the CVisualisation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
#define AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "../cores/DllLoader/dll.h"

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

  struct Visualisation
  {
public:
    void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisation);
    void (__cdecl* Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
    void (__cdecl* AudioData)(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
    void (__cdecl* Render) ();
    void (__cdecl* Stop)();
    void (__cdecl* GetInfo)(VIS_INFO *info);
  } ;

#ifdef __cplusplus
};
#endif

class CVisualisation
{
public:
  CVisualisation(struct Visualisation* pVisz, DllLoader* pLoader, const CStdString& strVisualisationName);
  ~CVisualisation();

  // Things that MUST be supplied by the child classes
  void Create(int posx, int posy, int width, int height);
  void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const CStdString strSongName);
  void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
  void Render();
  void Stop();
  void GetInfo(VIS_INFO *info);
protected:
  auto_ptr<struct Visualisation> m_pVisz;
  auto_ptr<DllLoader> m_pLoader;
  CStdString m_strVisualisationName;

  // position on screen
  int m_posX;
  int m_posY;
  int m_width;
  int m_height;
};


#endif // !defined(AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
