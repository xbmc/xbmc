#include "BoxLine.h"
#include "Settings.h"
#include <stdio.h>

void CBoxLine::CleanUp()
{
	for(int i=0; i<g_stSettings.m_iBars; i++)
		m_pCubes[i].CleanUp();

	free(m_pCubes);
}

bool CBoxLine::Init(LPDIRECT3DDEVICE8 pD3DDevice, float fLineZ)
{
	m_pd3dDevice = pD3DDevice;

	m_fBaseHeight = 0.1f;
	m_fBarWidth = 24.0f / g_stSettings.m_iBars;
	m_fBaseZ = fLineZ;

	m_pCubes = (CD3DCube *)malloc(sizeof(CD3DCube) * g_stSettings.m_iBars);
	if(!m_pCubes)
		return false;

	for(int i=0; i<g_stSettings.m_iBars; i++)
	{
		float nextLeft = -12.0f + (i * m_fBarWidth);
		m_pCubes[i].Init(m_pd3dDevice, nextLeft, -10.0f, fLineZ, m_fBaseHeight, m_fBarWidth, g_stSettings.m_fBarDepth);
	}

	return true;
}

void CBoxLine::Render(LPDIRECT3DTEXTURE8 pTexture)
{
	for(int i=0; i<g_stSettings.m_iBars; i++)
	{
		m_pCubes[i].Render(pTexture);
	}
}

bool CBoxLine::Set(float *pFreqData)
{
	for(int i=0; i<g_stSettings.m_iBars; i++)
	{
		if(!m_pCubes[i].Set(m_fBaseHeight + (pFreqData[i*2] / 10.0f)))
			return false;
	}

	return true;
}

//Is this row in font of the current cam position?
bool CBoxLine::CheckVisible(float fCamPos)
{
	if(m_fBaseZ + g_stSettings.m_fBarDepth > fCamPos)
		return true;
	else
		return false;
}