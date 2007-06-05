#ifndef __PCM_AMPLIFY__H__
#define __PCM_AMPLIFY__H__

#include "../Settings.h"

// VOLUME_NATIVE is the volume to which no amplification is done (middle). 
#define VOLUME_NATIVE (VOLUME_MINIMUM+((VOLUME_MAXIMUM-VOLUME_MINIMUM)/2))

class CPCMAmplifier {
public:
	CPCMAmplifier();
	virtual ~CPCMAmplifier();

	void SetVolume(int nVolume);
	int  GetVolume();

   	// only works on 16bit samples
	void Amplify(short *pcm, int nSamples);

protected:
	int m_nVolume;
	double m_dFactor;

};

#endif

