// Spectrum.cpp: implementation of the CSpectrum class.
//
//////////////////////////////////////////////////////////////////////

#include <xtl.h>
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

VIS_INFO		  vInfo;
static LPDIRECT3DDEVICE8 m_pd3dDevice;
extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice)
{
}

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
}


extern "C" void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
}

extern "C" void Render()
{
}


extern "C" void Stop()
{
}


extern "C" void GetInfo(VIS_INFO* pInfo)
{
	memcpy(pInfo,&vInfo,sizeof(struct VIS_INFO ) );
}
extern "C" 
{
	struct Visualisation
	{
	public:
		void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice);
		void (__cdecl* Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample);
		void (__cdecl* AudioData)(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
		void (__cdecl* Render) ();
		void (__cdecl* Stop)();
		void (__cdecl* GetInfo)(VIS_INFO* pInfo);
	};

	void __declspec(dllexport) get_module(struct Visualisation* pVisz)
	{
		pVisz->Create = Create;
		pVisz->Start = Start;
		pVisz->AudioData = AudioData;
		pVisz->Render = Render;
		pVisz->Stop = Stop;
		pVisz->GetInfo = GetInfo;
	}
};