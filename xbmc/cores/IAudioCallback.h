// IAudioCallback.h: interface for the IAudioCallback class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IAUDIOCALLBACK_H__5A6AC7CF_C60E_45B9_8113_599F036FBBF8__INCLUDED_)
#define AFX_IAUDIOCALLBACK_H__5A6AC7CF_C60E_45B9_8113_599F036FBBF8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class IAudioCallback  
{
public:
	IAudioCallback() {};
	virtual ~IAudioCallback() {};
	virtual void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample)=0;
	virtual void OnAudioData(const unsigned char* pAudioData, int iAudioDataLength)=0;
	
};

#endif // !defined(AFX_IAUDIOCALLBACK_H__5A6AC7CF_C60E_45B9_8113_599F036FBBF8__INCLUDED_)
