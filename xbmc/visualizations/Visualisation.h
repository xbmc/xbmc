// Visualisation.h: interface for the CVisualisation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
#define AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <xtl.h>
#include "../cores/DllLoader/dll.h"
#include <memory>
using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

struct VIS_INFO 
{
	bool bWantsFreq;
	int iSyncDelay;
	//		int iAudioDataLength;
	//		int iFreqDataLength;
};

struct Visualisation
{
public:
	void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice);
	void (__cdecl* Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample);
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
	CVisualisation(struct Visualisation* pVisz,DllLoader* pLoader);
	virtual ~CVisualisation();

	// Things that MUST be supplied by the child classes
	void Create();
	void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample);
	void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
	void Render();
	void Stop();
	void GetInfo(VIS_INFO *info);
private:
	auto_ptr<struct Visualisation> m_pVisz;
	auto_ptr<DllLoader> m_pLoader;
};


#endif // !defined(AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
