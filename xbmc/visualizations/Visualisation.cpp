#include "stdafx.h" 
// Visualisation.cpp: implementation of the CVisualisation class.
//
//////////////////////////////////////////////////////////////////////

#include "../Application.h"
#include "Visualisation.h" 


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVisualisation::CVisualisation(struct Visualisation* pVisz, DllVisualisation* pDll, const CStdString& strVisualisationName)
    : m_pVisz(pVisz)
    , m_pDll(pDll)
    , m_strVisualisationName(strVisualisationName)
{}

CVisualisation::~CVisualisation()
{
}

void CVisualisation::Create(int posx, int posy, int width, int height)
{
  // allow vis. to create internal things needed
  // pass it the location,width,height
  // and the name of the visualisation.
  char szTmp[129];
  sprintf(szTmp, "create:%ix%i at %ix%i %s\n", width, height, posx, posy, m_strVisualisationName.c_str());
  OutputDebugString(szTmp);
  
  float pixelRatio = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].fPixelRatio;
#ifndef HAS_SDL  
  // TODO LINUX this is obviously not good, but until we have visualization sorted out, this will have to do
  m_pVisz->Create (g_graphicsContext.Get3DDevice(), posx, posy, width, height, m_strVisualisationName.c_str(), pixelRatio);
#else
  m_pVisz->Create (0, posx, posy, width, height, m_strVisualisationName.c_str(), pixelRatio);
#endif
}

void CVisualisation::Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const CStdString strSongName)
{
  // notify visz. that new song has been started
  // pass it the nr of audio channels, sample rate, bits/sample and offcourse the songname
  m_pVisz->Start(iChannels, iSamplesPerSec, iBitsPerSample, strSongName.c_str());
}

void CVisualisation::AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  // pass audio data to visz.
  // audio data: is short audiodata [channel][iAudioDataLength] containing the raw audio data
  // iAudioDataLength = length of audiodata array
  // pFreqData = fft-ed audio data
  // iFreqDataLength = length of pFreqData
  m_pVisz->AudioData(const_cast<short*>(pAudioData), iAudioDataLength, pFreqData, iFreqDataLength);
}

void CVisualisation::Render()
{
  // ask visz. to render itself
  g_graphicsContext.BeginPaint();
  m_pVisz->Render();
  g_graphicsContext.EndPaint();

}

void CVisualisation::Stop()
{
  // ask visz. to cleanup
  m_pVisz->Stop();
}


void CVisualisation::GetInfo(VIS_INFO *info)
{
  // get info from vis
  m_pVisz->GetInfo(info);
}

bool CVisualisation::OnAction(VIS_ACTION action, void *param) 
{
  // see if vis wants to handle the input
  // returns false if vis doesnt want the input
  // returns true if vis handled the input
  if (action != VIS_ACTION_NONE && m_pVisz->OnAction)
    return m_pVisz->OnAction((int)action, param);
  return false;
}

void CVisualisation::GetSettings(vector<VisSetting> **vecSettings)
{
  if (vecSettings) *vecSettings = NULL;
  if (m_pVisz->GetSettings)
    m_pVisz->GetSettings(vecSettings);
}

void CVisualisation::UpdateSetting(int num)
{
  if (m_pVisz->UpdateSetting)
    m_pVisz->UpdateSetting(num);
}

void CVisualisation::GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{
  if (m_pVisz->GetPresets)
    m_pVisz->GetPresets(pPresets, currentPreset, numPresets, locked);
}

void CVisualisation::GetCurrentPreset(char **pPreset, bool *locked)
{
  if (pPreset && locked && m_pVisz->GetPresets)
  {
    char **presets = NULL;
    int currentPreset = 0;
    int numPresets = 0;
    *locked = false;
    m_pVisz->GetPresets(&presets, &currentPreset, &numPresets, locked);
    if (presets && currentPreset < numPresets)
      *pPreset = presets[currentPreset];
  }
}

bool CVisualisation::IsLocked()
{
  char *preset;
  bool locked = false;
  GetCurrentPreset(&preset, &locked);
  return locked;
}

char *CVisualisation::GetPreset()
{
  char *preset = NULL;
  bool locked = false;
  GetCurrentPreset(&preset, &locked);
  return preset;
}
