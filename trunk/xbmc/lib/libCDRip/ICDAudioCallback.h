// IAudioCallback.h: interface for the IAudioCallback class.
//
//////////////////////////////////////////////////////////////////////


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class ICDAudioCallback  
{
public:
	ICDAudioCallback() {};
	virtual ~ICDAudioCallback() {};
	virtual void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample)=0;
	virtual void OnAudioData(const unsigned char* pAudioData, int iAudioDataLength)=0;
	
};

