
#include "stdafx.h"
#include "PCMAmplifier.h"

CPCMAmplifier::CPCMAmplifier() : m_nVolume(VOLUME_MAXIMUM), m_dFactor(0)
{
}

CPCMAmplifier::~CPCMAmplifier()
{
}

void CPCMAmplifier::SetVolume(int nVolume) 
{
  m_nVolume = nVolume;
  if (nVolume > VOLUME_MAXIMUM)
    nVolume = VOLUME_MAXIMUM;
  
  if (nVolume < VOLUME_MINIMUM)
    nVolume = VOLUME_MINIMUM;

  m_dFactor = 1.0 - fabs((float)nVolume / (float)(VOLUME_MAXIMUM - VOLUME_MINIMUM));
}

int  CPCMAmplifier::GetVolume()
{
  return m_nVolume;
}

     // only works on 16bit samples
void CPCMAmplifier::DeAmplify(short *pcm, int nSamples)
{
  if (m_dFactor >= 1.0) 
  {
    // no process required. using >= to make sure no amp is ever done (only de-amp) 
    return;
  }
  
  for (int nSample=0; nSample<nSamples; nSample++) 
  {
    int nSampleValue = pcm[nSample]; // must be int. so that we can check over/under flow 
    nSampleValue = (int)((double)nSampleValue * m_dFactor);
    
    pcm[nSample] = (short)nSampleValue;
  }
}
