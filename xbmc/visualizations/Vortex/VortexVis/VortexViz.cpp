/*
 *      Copyright (C) 2010 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>
#include <xtl.h>
#include "Vortex.h"

using namespace std;

#pragma comment (lib, "lib/xbox_dx8.lib" )

extern "C" 
{
	struct VIS_INFO {
		bool bWantsFreq;
		int iSyncDelay;
		//		int iAudioDataLength;
		//		int iFreqDataLength;
	};
};

Vortex_c vortex;
char g_xmlFile[1024];

extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName, float fPixelRatio)
{
	strcpy(g_xmlFile, "P:\\visualisations\\");
	strcat(g_xmlFile, szVisualisationName);
	strcat(g_xmlFile, ".xml");

	vortex.LoadSettings(g_xmlFile);
	vortex.Init(pd3dDevice, iPosX, iPosY, iWidth, iHeight, fPixelRatio, g_xmlFile);
}

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{

}

extern "C" void Stop()
{
	vortex.SaveSettings(g_xmlFile);
	vortex.Stop();
}

unsigned char waves[2][576];

extern "C" void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
	vortex.AudioData(pAudioData, iAudioDataLength, pFreqData, iFreqDataLength);

}

extern "C" void Render()
{
	//  OutputDebugString("Vortex INFO: Enter Render\n");
	vortex.Render();
	//  OutputDebugString("Vortex INFO: Leave Render\n");
}

extern "C" void GetInfo(VIS_INFO* pInfo)
{
	pInfo->bWantsFreq = false;
	pInfo->iSyncDelay = 0;
}

extern "C" void GetSettings(vector<VisSetting> **vecSettings)
{
	if (!vecSettings) return;
	// load in our settings
	*vecSettings = vortex.GetSettings();
	return;
}

extern "C" void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{
	vortex.GetPresets(pPresets, currentPreset, numPresets, locked);
}

extern "C" bool OnAction(long flags, void *param)
{
	return vortex.OnAction(flags, param);
}

extern "C" void UpdateSetting(int num)
{
	vortex.UpdateSettings(num);
}

extern "C" 
{

	struct Visualisation
	{
	public:
		void (__cdecl *Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName, float fPixelRatio);
		void (__cdecl *Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
		void (__cdecl *AudioData)(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
		void (__cdecl *Render)();
		void (__cdecl *Stop)();
		void (__cdecl *GetInfo)(VIS_INFO* pInfo);
		bool (__cdecl *OnAction)(long action, void *param);
		void (__cdecl *GetSettings)(vector<VisSetting> **vecSettings);
		void (__cdecl *UpdateSetting)(int num);
		void (__cdecl *GetPresets)(char ***pPresets, int *currentPreset, int *numPresets, bool *locked);

	};

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

	}
};