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
#include "stdstring.h"
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
	void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szVisualisation);
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
	CVisualisation(struct Visualisation* pVisz,DllLoader* pLoader, const CStdString& strVisualisationName);
	virtual ~CVisualisation();

	// Things that MUST be supplied by the child classes
	virtual void Create();
	virtual void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const CStdString strSongName);
	virtual void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
	virtual void Render();
	virtual void Stop();
	virtual void GetInfo(VIS_INFO *info);
protected:
	auto_ptr<struct Visualisation> m_pVisz;
	auto_ptr<DllLoader> m_pLoader;
  CStdString m_strVisualisationName;
};


#endif // !defined(AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
