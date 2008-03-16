#ifndef __PCM_AMPLIFY__H__
#define __PCM_AMPLIFY__H__

#include "../Settings.h"

class CPCMAmplifier {
public:
  CPCMAmplifier();
  virtual ~CPCMAmplifier();

  void SetVolume(int nVolume);
  int  GetVolume();

  // only works on 16bit samples
  void DeAmplify(short *pcm, int nSamples);

protected:
  int m_nVolume;
  double m_dFactor;

};

#endif

