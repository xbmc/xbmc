// Boxalizer Vis Plugin for XBMC
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <math.h>
#include "fft.h"
#include "Settings.h"
#include "Boxalizer.h"
#include "xbmc_vis.h"

#pragma comment (lib, "lib/xbox_dx8.lib" )

/*extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);
extern "C" void d3dGetRenderState(DWORD dwY, DWORD* dwZ);
extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
*/
#define	FREQ_DATA_SIZE 1024			// size of frequency data wanted
#define MIN_LEVEL 0					// allowable level range
#define MAX_LEVEL 96

VIS_INFO		vInfo;
CBoxalizer		myVis;

// Arrays to store frequency data
float	m_pFreq[MAX_BARS*2];			// Frequency data
int		m_iSampleRate;
float	fCamPos;

static  char		m_szVisName[1024];
LPDIRECT3DDEVICE8	m_pd3dDevice;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName)
{
	strcpy(m_szVisName, szVisualisationName);
	m_pd3dDevice = pd3dDevice;
	g_settings.LoadSettings();

	vInfo.bWantsFreq = true;
	vInfo.iSyncDelay = g_stSettings.m_iSyncDelay;

	if(!myVis.Init(m_pd3dDevice))
		return;

	fCamPos = 10.0f;
}

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
	m_iSampleRate = iSamplesPerSec;
  g_settings.LoadSettings();
  printf("loading preset %i",g_settings.m_nCurrentPreset);
  g_settings.LoadPreset(g_settings.m_nCurrentPreset);
}

extern "C" void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
	if (iFreqDataLength > FREQ_DATA_SIZE)
		iFreqDataLength = FREQ_DATA_SIZE;

	// Group data into frequency bins by averaging (Ignore the constant term)
	int jmin=2;
	int jmax;
	// FIXME:  Roll conditionals out of loop
	for (int i=0, iBin=0; i < g_stSettings.m_iBars; i++, iBin+=2)
	{
		m_pFreq[iBin]=0.000001f;	// almost zero to avoid taking log of zero later
		m_pFreq[iBin+1]=0.000001f;
		if(g_stSettings.m_bLogScale)
			jmax = (int) (g_stSettings.m_fMinFreq*pow(g_stSettings.m_fMaxFreq/g_stSettings.m_fMinFreq,(float)i/g_stSettings.m_iBars)/m_iSampleRate*iFreqDataLength + 0.5f);
		else
			jmax = (int) ((g_stSettings.m_fMinFreq + (g_stSettings.m_fMaxFreq-g_stSettings.m_fMinFreq)*i/g_stSettings.m_iBars)/m_iSampleRate*iFreqDataLength + 0.5f);
		// Round up to nearest multiple of 2 and check that jmin is not jmax
		jmax<<=1;
		if(jmax > iFreqDataLength) jmax = iFreqDataLength;
		if(jmax == jmin)jmin -= 2;
		for (int j=jmin; j<jmax; j+=2)
		{
			if(g_stSettings.m_bMixChannels)
			{
				if(g_stSettings.m_bAverageLevels)
					m_pFreq[iBin] += pFreqData[j] + pFreqData[j+1];
				else 
				{
					if(pFreqData[j] > m_pFreq[iBin])
						m_pFreq[iBin] = pFreqData[j];
					if(pFreqData[j+1] > m_pFreq[iBin])
						m_pFreq[iBin] = pFreqData[j+1];
				}
			}
			else
			{
				if(g_stSettings.m_bAverageLevels)
				{
					m_pFreq[iBin] += pFreqData[j];
					m_pFreq[iBin+1] += pFreqData[j+1];
				}
				else
				{
					if(pFreqData[j] > m_pFreq[iBin])
						m_pFreq[iBin] = pFreqData[j];
					if(pFreqData[j+1] > m_pFreq[iBin+1])
						m_pFreq[iBin+1] = pFreqData[j+1];
				}
			}
		}
		if(g_stSettings.m_bAverageLevels)
		{
			if(g_stSettings.m_bMixChannels)
				m_pFreq[iBin] /=(jmax-jmin);
			else
			{
				m_pFreq[iBin] /= (jmax-jmin)/2;
				m_pFreq[iBin+1] /= (jmax-jmin)/2;
			}
		}
		jmin = jmax;
	}

	// Transform data to dB scale, 0 (Quietest possible) to 96 (Loudest)
	for (int i=0; i < g_stSettings.m_iBars*2; i++)
	{
		m_pFreq[i] = 10*log10(m_pFreq[i]);
		if (m_pFreq[i] > MAX_LEVEL)
			m_pFreq[i] = MAX_LEVEL;
		if (m_pFreq[i] < MIN_LEVEL)
			m_pFreq[i] = MIN_LEVEL;
	}
}

void SetupCamera(float fPosZ)
{
	D3DXMATRIX matView;
    D3DXMatrixLookAtLH(&matView, &D3DXVECTOR3(g_stSettings.m_fCamX, g_stSettings.m_fCamY, fPosZ), //Camera Position
                                 &D3DXVECTOR3(g_stSettings.m_fCamLookX, g_stSettings.m_fCamLookY, fPosZ + 30.0f), //Look At Position
                                 &D3DXVECTOR3(0.0f, 1.0f, 0.0f)); //Up Direction

    m_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);
}

extern "C" void Render()
{
	if(m_pd3dDevice == NULL)
		return;

	if(g_stSettings.m_bCamStatic != true)
	{
		float fThisRowZ = myVis.GetNextZ() - 30.0f - g_stSettings.m_fBarDepth;
		float fCurRowTime = (float)(timeGetTime() - myVis.GetLastRowTime());	//time spent in curr row
		
		fCamPos = fThisRowZ + ((fCurRowTime / (float)g_stSettings.m_iLingerTime) * g_stSettings.m_fBarDepth);

	}
	SetupCamera(fCamPos);

	////Here we will rotate our view around the x, y and z axis.
	D3DXMATRIX matView2, matRot;
	m_pd3dDevice->GetTransform(D3DTS_VIEW, &matView2);
	D3DXMatrixRotationYawPitchRoll(&matRot, 0.0f, 0.0f, 0.0f); 
	D3DXMatrixMultiply(&matView2, &matView2, &matRot);
	m_pd3dDevice->SetTransform(D3DTS_VIEW, &matView2);	

    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, 1.0f, 1.0f, 500.0f);
    m_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

	d3dSetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	d3dSetRenderState(D3DRS_LIGHTING, FALSE);
	d3dSetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	
	d3dSetRenderState(D3DRS_ALPHABLENDENABLE, true);
	d3dSetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	d3dSetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	d3dSetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	d3dSetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

	myVis.Set(m_pFreq);
	myVis.Render(fCamPos);
}

extern "C" void Stop()
{
	myVis.CleanUp();
}

  // function to export the above structure to XBMC
extern "C"
{
  void __declspec(dllexport) get_module(struct Visualisation* pVisz)
	{
		pVisz->Create = Create;
		pVisz->Start = Start;
		pVisz->AudioData = AudioData;
		pVisz->Render = Render;
		pVisz->Stop = Stop;
		pVisz->GetInfo = GetInfo;
    pVisz->OnAction = OnAction;
    pVisz->GetSettings = GetSettings;
    pVisz->UpdateSetting = UpdateSetting;
    pVisz->GetPresets = GetPresets;
	};
}