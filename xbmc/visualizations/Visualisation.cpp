// Visualisation.cpp: implementation of the CVisualisation class.
//
//////////////////////////////////////////////////////////////////////

#include "Visualisation.h"
#include "GraphicContext.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVisualisation::CVisualisation(struct Visualisation* pVisz,DllLoader* pLoader)
:m_pLoader(pLoader)
,m_pVisz(pVisz)
{	
}

CVisualisation::~CVisualisation()
{
}

void CVisualisation::Create()
{
  int iWidth = g_graphicsContext.GetWidth();
  int iHeight= g_graphicsContext.GetHeight();
  char szTmp[129];
  sprintf(szTmp,"create:%ix%i\n", iWidth,iHeight);
  OutputDebugString(szTmp);
  m_pVisz->Create (g_graphicsContext.Get3DDevice(),iWidth,iHeight );
}

void CVisualisation::Start(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
	m_pVisz->Start(iChannels, iSamplesPerSec, iBitsPerSample);
}

void CVisualisation::AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
	m_pVisz->AudioData(const_cast<short*>(pAudioData), iAudioDataLength, pFreqData, iFreqDataLength);
}

void CVisualisation::Render()
{
	m_pVisz->Render();
}

void CVisualisation::Stop()
{
	m_pVisz->Stop();
}


void CVisualisation::GetInfo(VIS_INFO *info)
{
	m_pVisz->GetInfo(info);
}