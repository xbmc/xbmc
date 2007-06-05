
#include "stdafx.h"
#include "PCMAmplifier.h"

CPCMAmplifier::CPCMAmplifier() : m_nVolume(VOLUME_NATIVE), m_dFactor(1.0)
{
}

CPCMAmplifier::~CPCMAmplifier() {
}

void CPCMAmplifier::SetVolume(int nVolume) {
	m_nVolume = nVolume;
	if (nVolume > VOLUME_MAXIMUM)
		nVolume = VOLUME_MAXIMUM;
	
	if (nVolume < VOLUME_MINIMUM)
		nVolume = VOLUME_MINIMUM;

	m_dFactor = fabs(((float)(nVolume - VOLUME_MINIMUM) / (float)VOLUME_NATIVE)) ;
		
}

int  CPCMAmplifier::GetVolume() {
	return m_nVolume;
}

   	// only works on 16bit samples
void CPCMAmplifier::Amplify(short *pcm, int nSamples) {
	if (m_dFactor > 0.98 && m_dFactor < 1.02) {
		// no amp - native volume 
		return;
	}
	
	for (int nSample=0; nSample<nSamples; nSample++) {
		int nSampleValue = pcm[nSample]; // must be int. so that we can check over/under flow 
		nSampleValue = (int)((double)nSampleValue * m_dFactor);
		
		// must keep the volume in the right range 
		if (nSampleValue < -32768)
			nSampleValue = -32768;
		else if (nSampleValue > 32767)
			nSampleValue = 32767;		
		
		pcm[nSample] = (short)nSampleValue;
	}
}
