// Visualisation.cpp: implementation of the CVisualisation class.
//
//////////////////////////////////////////////////////////////////////

#include "Visualisation.h"
#include "GraphicContext.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVisualisation::CVisualisation(struct Visualisation* pVisz)
{
	m_pVisz=pVisz;
}

CVisualisation::~CVisualisation()
{
	delete m_pVisz;
}

void CVisualisation::Create()
{
	if (!m_pVisz) return;
	m_pVisz->Create (g_graphicsContext.Get3DDevice() );
}

void CVisualisation::Start(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
	if (!m_pVisz) return;
	m_pVisz->Start(iChannels, iSamplesPerSec, iBitsPerSample);
}

void CVisualisation::AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
	if (!m_pVisz) return;
	m_pVisz->AudioData(pAudioData, iAudioDataLength, pFreqData, iFreqDataLength);
}

void CVisualisation::Render()
{
	if (!m_pVisz) return;
	m_pVisz->Render();
}

void CVisualisation::Stop()
{
	if (!m_pVisz) return;
	m_pVisz->Stop();
}


void CVisualisation::GetInfo(VIS_INFO *info)
{
	if (!m_pVisz) return;
	m_pVisz->GetInfo(info);
}